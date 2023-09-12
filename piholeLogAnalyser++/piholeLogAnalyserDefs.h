#pragma once

#include <string>

struct cliArgs
{
    bool dryRun;
    std::string fileSpec;
    std::string rxFileSpec;
    std::regex rx;
    std::string searchDirName;
    bool recurse;
    std::string errorPathFilename;
    unsigned long maxExecTimeInMilliseconds;
    std::string customPatternFilename;
    std::string dbUserName;
    std::string dbUserPwd;
    std::string serverIPAddress;
    std::string serverPortNumber;
    std::string databaseName;

    std::string testRxString;   // testing
};
