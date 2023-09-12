#pragma once

#include <memory>
#include <string>
//#include <cstring>
#include <regex>
#include <thread>
#include <map>
#include <list>
#include <iostream>
#include <fstream>

#include <thread>
#include <chrono>
#include <future>

#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX

#include "grokResult.h"
#include "grokException.h"
#include "grokNamedSubExpression.h"

using namespace std;

class grok
{
private:
        regex _compiledRegex; // the working regex object with all subcomponents expanded out

        regex _grokNamedExpression;     // used to extract the expressions with names e.g. %{LOGTIME:TimeOfCall:datetime} and %{LOGENDOFLINE:EndOfLine}
        regex _grokUnnamedExpression;   // used to extract unnamed expressions e.g. %{LOGIPv4} which is named at a higer level 

        regex _grokExpressionWithType;  // used to extract the explicitly typed expressions (%\{(\w+):(\w+):(\w+)\})*? e.g. %{LOGTIME:TimeOfCall:datetime}

        string _grokPattern;
        map<string, string> _patterns;
        map<string, string> _typeMaps;
        list<string> _groupNames;
        map<string, int> _nameIndexMap;

        grokResult grkRes;

public:
    grok(string grkPattern)
    {
        _grokNamedExpression.assign(R"(%\{(\w+):(\w+)(:\w+)?\})");
        _grokUnnamedExpression.assign(R"(%\{(\w+)\})");
        _grokExpressionWithType.assign(R"((%\{(\w+):(\w+):(string|datetime|int|float)\}))");
        _grokPattern = grkPattern;
        LoadPatterns();
    }

    ~grok()
    {
    }

    grok(string grokPattern, ifstream * customPatternFile) : grok(grokPattern)
    {
        LoadPatterns(customPatternFile);
    }

    grok(string grokPattern, map<string, string>customPatterns) : grok(grokPattern)
    {
        AddPatterns(customPatterns);
    }

    grokResult * Parse(string text, unsigned long milliSeconds = 0)
    {
        grkRes.reset();
        if (milliSeconds == 0)        // Not timed so call synchronously
        {
            matchText(text);
            grkRes.timedOut = false;
        }
        else
        {
            std::future<bool> matchThread = std::async(std::launch::async, [&] { return matchText(text); });

            if (matchThread.wait_for(std::chrono::milliseconds(milliSeconds)) == std::future_status::timeout)
                grkRes.timedOut = true;
            else
                grkRes.timedOut = false;
        }

        return &grkRes;
    }

    void ParseGrokString(string grokPattern = "")
    {
        string pattern;
        string replacedString = "";

        if (grokPattern != "")
        {
            pattern = grokPattern;
            _grokPattern = grokPattern;
        }
        else
            pattern = _grokPattern;

        bool substitutionsComplete = false;

        replacedString = pattern;
        do
        {
            string searchString = replacedString;
            for (smatch sm; regex_search(searchString, sm, _grokExpressionWithType);)
            {
                grkRes.addItemWithType(sm.str(3), sm.str(4));
                searchString = sm.suffix().str();
            }

            //// replace the named components with the expanded version 
            map<string, string> replacements;
            string expandedExpression;
            string rxReplacePattern;
            regex rxReplace;
            regex rxDollarSign("\\$");
            searchString = replacedString;
            for (smatch smat; regex_search(searchString, smat, _grokNamedExpression);)
            {
                rxReplacePattern = "%\\{" + smat.str(1) + ":" + smat.str(2) + ".*?\\}";
                if (replacements.find(rxReplacePattern) == replacements.end())
                {
                    expandedExpression = regex_replace(replaceWithName(smat), rxDollarSign, "$$$$");
                    replacements.insert(make_pair(rxReplacePattern, expandedExpression));
                }
                searchString = smat.suffix().str();
            }

            // do the same for the unnamed components
            searchString = replacedString;
            for (smatch smat; regex_search(searchString, smat, _grokUnnamedExpression);)
            {
                rxReplacePattern = "%\\{" + smat.str(1) + "\\}";
                if (replacements.find(rxReplacePattern) == replacements.end())
                {
                    expandedExpression = regex_replace(replaceWithoutName(smat), rxDollarSign, "$$$$");
                    replacements.insert(make_pair(rxReplacePattern, expandedExpression));
                }
                searchString = smat.suffix().str();
            }

            for (auto it = replacements.begin(); it != replacements.end(); it++)
            {
                rxReplace.assign(it->first);
                replacedString = std::regex_replace(replacedString, rxReplace, it->second);
            }
            replacements.clear();


            if (replacedString == pattern)     // i.e. there were no replacements in the last call
                substitutionsComplete = true;

            pattern = replacedString;

        } while (!substitutionsComplete);

        pattern = createNameIndexRemovingNames(pattern);        /// remove when std::regex has named groups
        _compiledRegex.assign(pattern, std::regex::optimize);
    }

private:
    bool matchText(string text)
    {
        smatch sm;

        if (regex_match(text, sm, _compiledRegex))
        {
            grkRes.matched = true;
            for (auto nameIndexPair = _nameIndexMap.begin(); nameIndexPair != _nameIndexMap.end(); nameIndexPair++)
            {
                grkRes.addItemResult(nameIndexPair->first, sm.str(nameIndexPair->second));
            }
            for (size_t i = 0; i < sm.size(); i++)
                grkRes.addItemResult((unsigned)i, sm.str(i));
            return true;
        }
        else
        {
            grkRes.matched = false;
            return false;
        }
    }

    void loadCustomPatterns(ifstream* customPatternFile)
    {
        LoadPatterns(customPatternFile);
    }

    void AddPatterns(map<string, string>customPatterns)
    {
        for (std::map<string, string>::iterator it = customPatterns.begin(); it != customPatterns.end(); ++it)
            AddPatternIfNotExists(it->first, it->second);
    }

    void AddPatternIfNotExists(string key, string value)
    {
        EnsurePatternIsValid(value);
        _patterns.insert(make_pair(key, value));        // Does nothing if key is a duplicate
        //            if ()
    }

    string createNameIndexRemovingNames(string pattern)
    {
        int groupNumber = 0;
        unsigned letterPosition;
        bool copyNameTag = false;
        bool skipLetter = false;        // true when a letter is not to be copied to the output string
        string deNamedPattern = "";
        string newName = "";
        char currentLetter = ' ', lastLetter = ' ';
        for (letterPosition = 0; letterPosition < pattern.size(); letterPosition++)
        {
            currentLetter = pattern[letterPosition];

            if ((currentLetter == '(') && (lastLetter != '\\')) // unescaped open indicates a new group
            {
                groupNumber++;
            }
            else if ((currentLetter == '<') && (lastLetter == '?'))  // start of a name
            {
                copyNameTag = true;
                skipLetter = true;      // don't copy the <
                deNamedPattern = deNamedPattern.substr(0, deNamedPattern.size() - 1);   // remove the ? from the output string
            }
            else if ((currentLetter == '>') && (lastLetter != '\\')) // end of a name
            {
                copyNameTag = false;
                skipLetter = true;      // don't copy the >
                _nameIndexMap.insert(make_pair(newName, groupNumber));
                newName = "";
            }
            else if ((currentLetter == ':') && (lastLetter == '?'))
                groupNumber--; // non capturing group

            if (!skipLetter)
            {
                if (copyNameTag)
                    newName.push_back(currentLetter);
                else
                    deNamedPattern.push_back(currentLetter);
            }
            else
                skipLetter = false;

            lastLetter = currentLetter;
        }
        return deNamedPattern;
    }

    void LoadPatterns()
    {
        // Load default patterns
        string oneLine;
        char result[PATH_MAX];
        memset(result, '\0', PATH_MAX);

        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        const char* path;
        if (count != -1) {
            path = dirname(result);
        }

        string executableLocation(path);
        ifstream patterns(executableLocation += "/grok-patterns");

        if (patterns.is_open())
        {
            while (!patterns.eof())
            {
                getline(patterns, oneLine, '\n');
                ProcessPatternLine(oneLine);
            }
            patterns.close();
        }
        else
            throw grokException("Standard Pattern file not found in the executable's directory.");
    }

    void LoadPatterns(ifstream * customPatterns)
    {
        string oneLine;
        while (!customPatterns->eof())
        {
            getline((*customPatterns), oneLine, '\n');
            ProcessPatternLine(oneLine);
        }
    }

    void ProcessPatternLine(string line)
    {
        if (line.length() == 0) /// empty line
            return;

        if (line[0] == '#')      /// Comment line
            return;

        auto spacePos = line.find_first_of(" ");

        if (spacePos == line.npos)
            throw grokException("Pattern line was not in a correct form (two strings split by space)");

        string key(line.substr(0, spacePos));
        string value(line.substr(spacePos + 1));

        AddPatternIfNotExists(key, value);
    }

    void EnsurePatternIsValid(string pattern)
    {
        try
        {
            static const auto r = regex(pattern, std::regex::optimize); // retest      /// Seems to think that all expressions are valid e.g. ^(Jan"|Feb)$
        }
        catch (const regex_error& e)
        {
            string msg = string("Invalid regular expression: ") + pattern + " " + e.what();
            throw grokException(msg);
        }

    }

    string replaceWithName(smatch match)
    {
        string rxExpansion;
        map<string, string>::iterator foundKey = _patterns.find(match.str(1));

        if (foundKey == _patterns.end())
        {
            cout << "Named expansion: " << match.str(1) << " was not found.";
            return "(?<" + match.str(2) + ">)";
        }
        
        rxExpansion = foundKey->second;
        _groupNames.push_back(match.str(2));    // record the names as we go

        return "(?<" + match.str(2) + ">"+rxExpansion+")";  // c++11 does not have named capture groups
        //return "(" + rxExpansion + ")";

    }

    string replaceWithoutName(smatch match)
    {

        string rxExpansion;
        map<string, string>::iterator foundKey = _patterns.find(match.str(1));

        if (foundKey == _patterns.end())
        {
            cout << "Unnamed expansion: " << match.str(1) << " was not found.";
            return "()";
        }

        return "(" + foundKey->second + ")";
    }
};