#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <cstring>
#include <regex>
#include "piholeLogAnalyserDefs.h"
#include "dbInterface.h"
#include "grok.h"
#include "DNSQuery.h"

using namespace std;

int appendFoundFilesInDirectoryToList(const filesystem::directory_entry file, regex rx, list<filesystem::path> * fileList)
{
    if (regex_match(file.path().filename().c_str(), rx))
    {
        fileList->push_front(file.path());   
        return 1;
    }
    else
        return 0;
}

int searchDirectoryForMatchingFilesAndAppendToList(cliArgs* args, list<filesystem::path> * fileList, ofstream * errorLogger)
{
    filesystem::path logPath(args->searchDirName);
    if (filesystem::is_directory(logPath))
    {
        int foundFiles = 0;
        filesystem::path logFile(args->searchDirName);

        if (args->recurse)
            for (const auto& file : filesystem::recursive_directory_iterator(logPath))
                foundFiles += appendFoundFilesInDirectoryToList(file, args->rx, fileList);
        else
            for (const auto& file : filesystem::directory_iterator(logPath))
                foundFiles += appendFoundFilesInDirectoryToList(file, args->rx, fileList);

        return foundFiles;
    }
    else
    {
        (*errorLogger) << logPath << " is not a directory" << endl;
        return 0;
    }
}

void checkCommandLineArgs(cliArgs* args, string errorOutputFile)
{
    ofstream errorLogger(errorOutputFile.c_str());// check if path to errorOutputFile exits
    dbInterface tempDb(args->dryRun, &errorLogger);

    if (tempDb.ckeckDbServer(args))
        cout << "The database is good" << endl;
    else
        cout << "Could not open the database (see " << errorOutputFile << ")" << endl;

    try
    {
        list<filesystem::path> filenameList;
        if (searchDirectoryForMatchingFilesAndAppendToList(args, &filenameList, &errorLogger) == 0)
            cout << "There are no matching files in " << args->searchDirName << endl;
        else
        {
            unsigned long int i = filenameList.size();

            cout << "Found "<< i << " files:" << endl;
            
            for (std::list<filesystem::path>::iterator filenameInList = filenameList.begin(); filenameInList != filenameList.end(); ++filenameInList)
            {
                cout << "Matched: " << (*filenameInList) << endl;
            }
        }
    }
    catch (filesystem::filesystem_error& fse)
    {
        cout << "Could not find custom search directory " + args->searchDirName + " (error: " << fse.what() << ")" << endl;
    }

    //// check that the grok custom pattern file is available
    try
    {
        filesystem::path customPatternFile(args->customPatternFilename);
        if (filesystem::is_empty(customPatternFile))
            cout << "The custom pattern file exists but is empty." << endl;
        else
            cout << "The custom pattern file looks good." << endl;

    }
    catch (filesystem::filesystem_error& fse)
    {
        cout << "Error for " + args->customPatternFilename + " (error: " << fse.what() << ")" << endl;
    }

    if (tempDb.willExecuteDbCommands())
        cout << "The system will update the database (NOT a dry run)." << endl;
    else
        cout << "The system will NOT update the database (a dry run)." << endl;

    errorLogger.close();
    cout << "Exiting after check..." << endl;

}

void readArgs(int argc, char** argv, cliArgs * args)
{
    bool checkOnCompletion = false;

    args->dryRun = false;
    args->recurse = false;
    args->searchDirName = "/var/log/pihole";
    args->fileSpec = "pihole.log.1";
    args->rxFileSpec = "pihole\\.log\\.1";
    args->rx = regex(args->rxFileSpec);
    args->errorPathFilename = "./loadError.txt";
    args->maxExecTimeInMilliseconds = 0;
    args->customPatternFilename = "./grokCustom.txt";
    args->dbUserName = "logUser";
    args->dbUserPwd = "oppen20:Log";
    args->serverIPAddress = "192.168.1.110";
    args->serverPortNumber = "3306";
    args->databaseName = "dbPiholeLog";

    string arg;

    for (int i = 1; i < argc; i++)  // argv[0] is the executable name
    {
        arg = argv[i];
        if (arg.compare("-r") == 0)
            args->recurse = true;

        else if (arg.compare("-d") == 0)
            args->searchDirName = string(argv[++i]);

        else if (arg.compare("-f") == 0)
        {
            args->fileSpec = string(argv[++i]);

            regex rxMeta("\\[|\\]|\\.|\\:|\\+|\\,|\\^|\\{|\\}|\\~");            // all regex special characters that can appear in a linux filename
            args->rxFileSpec = regex_replace(args->fileSpec, rxMeta, R"(\$&)");

            rxMeta.assign("\\?");                                               // replace ? with a regex .
            args->rxFileSpec = regex_replace(args->rxFileSpec, rxMeta, R"(.)");
            rxMeta.assign("\\*");                                               // replace * with regex any number of valid file characters
            args->rxFileSpec = regex_replace(args->rxFileSpec, rxMeta, R"(([0-9]|[a-z]|[A-Z]|_|\[|\]|\.|\:|\+|\,|\^|\{|\}|\~)*)");    

            args->rx = regex(args->rxFileSpec);      
        }

        else if (arg.compare("-p") == 0)
            args->dryRun = true;

        else if (arg.compare("-t") == 0)
            args->maxExecTimeInMilliseconds = stol(string(argv[++i]));

        else if (arg.compare("-g") == 0)
            args->customPatternFilename = string(argv[++i]);

        else if (arg.compare("-e") == 0)
            args->errorPathFilename = string(argv[++i]);

        else if (arg.compare("-check") == 0)
            checkOnCompletion = true;

        else if (arg.compare("-user") == 0)
            args->dbUserName = string(argv[++i]);

        else if (arg.compare("-pwd") == 0)
            args->dbUserPwd = string(argv[++i]);

        else if (arg.compare("-ip") == 0)
            args->serverIPAddress = string(argv[++i]);

        else if (arg.compare("-port") == 0)
            args->serverPortNumber = string(argv[++i]);

        else if (arg.compare("-db") == 0)
            args->databaseName = string(argv[++i]);

        else if (arg.compare("-test") == 0)
            args->testRxString = string(argv[++i]);

        else
        {
            cout << "Invalid argument: " << arg << endl;
            cout << endl;
            cout << "-d Directory : directory to be scanned (default: /var/log/pihole)" << endl;
            cout << "-r : scan recursively (recurse == false)" << endl;
            cout << "-f 'Filename pattern' : load a specific log file with wildcards ? and * (default: pihole.log.1)" << endl;
            cout << "-p : Pretend to add to the database (default: false)" << endl;
            cout << "-t milliseconds : max exec time for regex (default == 0 or no time limit)" << endl;
            cout << "-g File name : specify the location of the grok pattern file" << endl;
            cout << "-e Full path to the load error file" << endl;
            cout << "-user username : log in to the database with the given username" << endl;
            cout << "-pwd pwd : log into the database with the given password" << endl;
            cout << "-ip IP Address : look for the database at the given IP address or url" << endl;
            cout << "-port portNum : connect to the database on the given port" << endl;
            cout << "-db dbName : connect to the database with the given name" << endl;
            cout << "-check : checks the database, directory and file spec for log files the grok custom pattern file and then exits (output in -e load error file)" << endl;
            cout << "-h or -help : print this text" << endl;
            exit(1);
        }
    }
    if (checkOnCompletion)
    {
        checkCommandLineArgs(args, args->errorPathFilename);
        exit(0);
    }
}

clock_t processLogFile(string pathFileName, grok* grk, dbInterface * db, ofstream* errorLogger, int * linesInserted, unsigned long timeLimitInMilliseconds)
{
    clock_t ticksForThisFile = 0;
    ifstream logFile(pathFileName);

    if (logFile.is_open())
    {
        dnsQuery currentQuery(db, errorLogger);
        bool dnsServerReplied = false;
        string logLine;
        string action = "";
        string actionTruncated = "";
        tm timeOfCall;
        memset(&timeOfCall, 0, sizeof(tm));
        string domainFrom = "";
        string callerIP = "";
        string valueAsString = "";
        char timeFormat[] = "%b %d %H:%M:%S";
        grokResult* grkRes;

        while (!logFile.eof())
        {
            getline(logFile, logLine, '\n');
            grkRes = grk->parse(logLine, timeLimitInMilliseconds);
            if (grkRes->timedOut)
            {
                (*errorLogger) << "Time out: >" << logLine << "<" << endl;
            }
            else if (!grkRes->matched)
            {
                (*errorLogger) << "Line not matched: >" << logLine << "<" << endl;
            }
            else
            {
                action = (*grkRes)["ActionFrom"].valueAsString() + (*grkRes)["ActionTo"].valueAsString() + (*grkRes)["ActionIs"].valueAsString();
                actionTruncated = action.substr(0, 5);

                if (actionTruncated.compare("query") == 0)      /// covers all varieties
                {
                    if ((*grkRes)["Timestamp"].value(&timeOfCall, &valueAsString, timeFormat))   // piholeLogDateTimeFormat))
                    {
                        if ((*grkRes)["DomainFrom"].value(&domainFrom))
                        {
                            if ((*grkRes)["EndOfLineFrom"].value(&callerIP))
                                currentQuery.setRequest(timeOfCall, domainFrom, callerIP);
                            else
                                (*errorLogger) << "Error at EndOfLineFrom: " << callerIP << endl;
                        }
                        else
                            (*errorLogger) << "Error at DomainFrom: " << domainFrom << endl;
                    }
                    else
                        (*errorLogger) << "Error at TimeStamp: " << valueAsString << endl;

                }
                else if ((actionTruncated.compare("forwa") == 0) ||
                    (actionTruncated.compare("cache") == 0) ||
                    (actionTruncated.compare("exact") == 0) ||
                    (actionTruncated.compare("gravi") == 0) ||
                    (actionTruncated.compare("regex") == 0))
                {
                    currentQuery.blockerAction(action);
                    if (dnsServerReplied)
                    {
                        ticksForThisFile += currentQuery.insertIntoDb(linesInserted);
                    }
                }
                else if (actionTruncated.compare("reply") == 0)
                {
                    if (!dnsServerReplied)      // store the first log entry that recieved a replay and then store from then on
                    {
                        ticksForThisFile += currentQuery.insertIntoDb(linesInserted);
                        dnsServerReplied = true;
                    }
                }
                else if ((actionTruncated.compare("confi") == 0) ||
                    (actionTruncated.compare("speci") == 0) ||
                    (actionTruncated.compare("Apple") == 0) ||
                    (actionTruncated.compare("Rate-") == 0))
                {
                    ;   // Ignore
                }
                else
                {
                    (*errorLogger) << "Unknown: " << endl;
                }
            }
        }
    }
    else
        (*errorLogger) << "File: " << pathFileName << " failed to open." << endl;

    return ticksForThisFile;
}

int main(int argc, char** argv)
{
    int exitCode = 1;
    cliArgs args;
    readArgs(argc, argv, &args);

    time_t now = time(0);
    string strNow = asctime(localtime(&now));

    ifstream customPatterns(args.customPatternFilename);
    ofstream errorLogger;
    errorLogger.open(args.errorPathFilename.c_str());
    errorLogger << "Opened: " << strNow;    // asctime adds a \n

    if (customPatterns.is_open())
    {
        list<filesystem::path> fileList;

        if (searchDirectoryForMatchingFilesAndAppendToList(&args, &fileList, &errorLogger) > 0)
        {
            now = time(0);
            strNow = asctime(localtime(&now));
            strNow.erase(remove_if(strNow.begin(), strNow.end(), [](auto ch) { return (ch == '\n'); }), strNow.end());
            cout << strNow << " : " << fileList.size() << " found" << endl;

            try
            {
                dbInterface db(args.dryRun, &errorLogger);

                if (db.open(&args))
                {
                    stringstream msg;
                    tm lastDate;
                    string grkPattern = "%{LOGTIME:Timestamp:datetime} %{LOGPROG:Prog}: ((%{LOGACTIONFROM:ActionFrom} %{LOGDOMAIN:DomainFrom} %{LOGDIRECTIONFROM:DirectionFrom} %{LOGEOLFROM:EndOfLineFrom})|(%{LOGACTIONTO:ActionTo} %{LOGDOMAIN:DomainTo} %{LOGDIRECTIONTO:DirectionTo} %{LOGEOLTO:EndOfLineTo})|(%{LOGACTIONIS:ActionIs} %{LOGDOMAIN:DomainIs} %{LOGDIRECTIONIS:DirectionIs} %{LOGEOLIS:EndOfLineIs}))";
                    grok grk(grkPattern, &customPatterns);

                    /*errorLogger <<*/ grk.parseGrokString();// << endl;

                    clock_t fileProcessingTimeInTicks;
                    clock_t totalProcessingTimeInTicks = 0;
                    int linesInserted = 0;
                    int filesProcessed = 0;
                    std::filesystem::file_time_type fileLastWriteTime;
                    unsigned long int fileSize;
                    string readLogKey;

                    db.maxDate(&lastDate);

                    totalProcessingTimeInTicks = clock();
                    for (list<filesystem::path>::iterator fileIter = fileList.begin(); fileIter != fileList.end(); fileIter++)
                    {
                        fileLastWriteTime = filesystem::last_write_time(*fileIter);
                        fileSize = filesystem::file_size(*fileIter);
                        if (db.recordLogFile(fileSize, fileLastWriteTime, &readLogKey))
                        {
                            fileProcessingTimeInTicks = processLogFile(*fileIter, &grk, &db, &errorLogger, &linesInserted, args.maxExecTimeInMilliseconds);

                            msg << strNow << " File: " << fileIter->filename() << " insert time: " << (fileProcessingTimeInTicks / CLOCKS_PER_SEC) << " seconds";
                            db.updateLogFileRecord(&readLogKey, linesInserted, msg.str());

                            filesProcessed++;
                        }
                        else
                        {
                            msg << "It looks like " << fileIter->filename() << " is a duplicate.";
                        }
                        cout << msg.str() << " for " << linesInserted << " lines" << endl;
                        msg.str("");
                    }

                    db.updateTblCommon();
                    db.updateLevelOfInterestFromDate(lastDate);

                    totalProcessingTimeInTicks = clock() - totalProcessingTimeInTicks;
                    cout << strNow << " Files processed: " << filesProcessed << " in " << (totalProcessingTimeInTicks / CLOCKS_PER_SEC) << " seconds." << endl;

                    db.close();
                    exitCode = 0;
                }
            }
            catch (grokException& gEx)
            {
                cout << gEx.what() << endl;
            }
            catch (exception& ex)
            {
                cout << ex.what() << endl;
                errorLogger << ex.what() << endl;
            }
            catch (int e)
            {
                cout << e << endl;
                errorLogger << "Error: " << e << endl;
            }
            catch (...)
            {
                cout << "Yeah... dunno!" << endl;
                errorLogger << "Unknown Exception" << endl;
            }
        }
        else
            cout << "No files found in: " << args.searchDirName << endl;

        customPatterns.close();
    }
    else
        cout << "grokCustom.txt not open (or not found)." << endl;

    errorLogger << "About to close" << endl;
    errorLogger.close();

    return exitCode;
}