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
        regex _compiledRegex; // the working regex object with all expression generated from the grok string expanded out

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
        loadPatterns();
    }

    ~grok()
    {
    }

    grok(string grokPattern, ifstream * customPatternFile) : grok(grokPattern)
    {
        loadPatterns(customPatternFile);
    }

    grok(string grokPattern, map<string, string>customPatterns) : grok(grokPattern)
    {
        addPatterns(customPatterns);
    }

    /// <summary>
    /// Parse the give text based on the regular expression generated from the Grok pattern
    /// Return a pointer to the member variable grkRes which is an instance of grokResult 
    /// </summary>
    /// <param name="test">The text to be parsed.</param>
    /// <param name="milliseconds">The time limit (in milliseconds) for regex to be allowed to run</param>
    grokResult * parse(string text, unsigned long milliSeconds = 0)
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
                grkRes.timedOut = true;     // matchText returned within the time limit
            else
                grkRes.timedOut = false;    // matchText did not return
        }

        return &grkRes;
    }

    /// <summary>
    /// Convert a Grok pattern into a regular expression
    /// The arguement allows for the Grok expression to change over the life of the objcect
    /// </summary>
    /// <param name="grokPattern">A new patter (default is the one passed on creation).</param>
    string parseGrokString(string grokPattern = "")
    {
        static string pattern;
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
            for (smatch match; regex_search(searchString, match, _grokExpressionWithType);)
            {
                grkRes.addItemWithType(match.str(3), match.str(4));
                searchString = match.suffix().str();
            }

            //// replace the named components with the expanded version 
            // collect all of the components that need to be replace along with what they need to be replace with
            map<string, string> replacements;   // the map of rx strings and expanded expressions that will replace the grok components
            regex rxReplace;                    // the regex object that does the replacement
            regex rxDollarSign("\\$");          // replace $ with $$ because $ is a special char in rxReplace

            std::function<string(string, string)> generateReplacePatternForNamedExpression = [](string part1, string part2) -> string { string pattern("%\\{" + part1 + ":" + part2 + ".*?\\}"); return pattern; };
            findGrokSubExpressionReplacements(replacedString, _grokNamedExpression, generateReplacePatternForNamedExpression, &replacements);

            //// do the same for the unnamed components
            std::function<string(string, string)> generateReplacePatternForUnnamedExpression = [](string part1, string part2) -> string { string pattern("%\\{" + part1 + "\\}"); return pattern; };
            findGrokSubExpressionReplacements(replacedString, _grokUnnamedExpression, generateReplacePatternForUnnamedExpression , &replacements);

            for (auto replacementPair = replacements.begin(); replacementPair != replacements.end(); replacementPair++)
            {
                rxReplace.assign(replacementPair->first);
                replacedString = std::regex_replace(replacedString, rxReplace, replacementPair->second);
            }
            replacements.clear();


            if (replacedString == pattern)     // i.e. there were no replacements in the last call
                substitutionsComplete = true;

            pattern = replacedString;

        } while (!substitutionsComplete);

        pattern = createNameIndexRemovingNames(pattern);        /// remove when std::regex has named groups
        _compiledRegex.assign(pattern, std::regex::optimize);
        return pattern;

    }

private:
    bool matchText(string text)
    {
        smatch match;

        if (regex_match(text, match, _compiledRegex))
        {
            grkRes.matched = true;
            for (auto nameIndexPair = _nameIndexMap.begin(); nameIndexPair != _nameIndexMap.end(); nameIndexPair++)
            {
                grkRes.addItemResult(nameIndexPair->first, match.str(nameIndexPair->second));
            }
            for (size_t i = 0; i < match.size(); i++)
                grkRes.addItemResult((unsigned)i, match.str(i));
            return true;
        }
        else
        {
            grkRes.matched = false;
            return false;
        }
    }

    string findGrokSubExpressionReplacements(string searchString, regex rxSubExpression, std::function<string(string, string)> generateRxPattern, map<string, string> * replacements)
    {
        // replace std::function<string(string, string)> generateRxPattern with String::format() when using c++20

        string rxReplacePattern;    // the rx string of all grok sub expressions that need to be replace
        string expandedExpression;  // the expanded subexpression that may include further grok components that need replacing
        regex rxDollarSign("\\$");

        for (smatch match; regex_search(searchString, match, rxSubExpression);)
        {
            rxReplacePattern = generateRxPattern(match.str(1), match.str(2));
            if (replacements->find(rxReplacePattern) == replacements->end())
            {
                expandedExpression = regex_replace(replaceWithName(match), rxDollarSign, "$$$$");
                replacements->insert(make_pair(rxReplacePattern, expandedExpression));
            }
            searchString = match.suffix().str();
        }
        return expandedExpression;
    }

    void loadCustomPatterns(ifstream* customPatternFile)
    {
        loadPatterns(customPatternFile);
    }

    void addPatterns(map<string, string>customPatterns)
    {
        for (std::map<string, string>::iterator patternPair = customPatterns.begin(); patternPair != customPatterns.end(); ++patternPair)
            addPatternIfNotExists(patternPair->first, patternPair->second);
    }

    void addPatternIfNotExists(string key, string value)
    {
        ensurePatternIsValid(value);
        _patterns.insert(make_pair(key, value));        // Does nothing if key is a duplicate
    }

    /// <summary>
    /// Because c++ regex does not yet support named expression I extract the names and 
    /// store them in grokResult to be associated with the result when it becomes available
    /// and return the string without the names
    /// </summary>
    /// <param name="patter">The regex string with names.</param>
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

    void loadPatterns()
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
                processPatternLine(oneLine);
            }
            patterns.close();
        }
        else
            throw grokException("Standard Pattern file not found in the executable's directory.");
    }

    void loadPatterns(ifstream * customPatterns)
    {
        string oneLine;
        while (!customPatterns->eof())
        {
            getline((*customPatterns), oneLine, '\n');
            processPatternLine(oneLine);
        }
    }

    void processPatternLine(string line)
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

        addPatternIfNotExists(key, value);
    }

    void ensurePatternIsValid(string pattern)
    {
        try
        {
            static const auto r = regex(pattern, std::regex::optimize); // retest  before ::optimize ^(Jan"|Feb)$ was valid
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
    }

    string replaceWithoutName(smatch match)
    {
        map<string, string>::iterator foundKey = _patterns.find(match.str(1));

        if (foundKey == _patterns.end())
        {
            cout << "Unnamed expansion: " << match.str(1) << " was not found.";
            return "()";
        }

        return "(" + foundKey->second + ")";
    }
};
