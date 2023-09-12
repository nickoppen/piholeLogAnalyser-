#pragma once
#include "dbInterface.h"
#include <string>
#include <ctime>
#include "grok.h"

class dnsQuery
{
public:
	dnsQuery(dbInterface * databaseInterface, ofstream* errorFile)
	{
		time_t rightNow = time(0);
		tm * tmNow = localtime(&rightNow);
		currentYearString = std::to_string(1900 + tmNow->tm_year);
		currentMonth = tmNow->tm_mon;
		currentYear = tmNow->tm_year;

		db = databaseInterface;
		errorLogger = errorFile;
	}

	void setRequest(tm logTimeOfCall, string targetDomain, string callerIPAddr)
	{
		timeOfQuery = logTimeOfCall;

		// There is no year part to the time stamp on the log record so
		// start by assuming that the log record is in the current year
		timeOfQuery.tm_year = currentYear;

		// check if the calculated time is less than now() i.e. it happened in the past
		// The log records loaded on 1 Jan are mostly from Dec last year so assuming the current year is the year of the log line that would put Dec records in the future
		if (timeOfQuery.tm_mon > currentMonth)
		{
			timeOfQuery.tm_year -= 1;
		}

		domain = targetDomain;
		callerIP = callerIPAddr;
		callerSubNetID = subNetIDFromIPAddr(callerIPAddr);
		isInserted = false;
	}

	void setRequest(string logTimeOfCall, string targetDomain, string callerIPAddr)
	{
		strptime(logTimeOfCall.c_str(), "%b %d %H:%M:%S", &timeOfQuery);
		setRequest(timeOfQuery, targetDomain, callerIPAddr);
	}

	void blockerAction(string blockerAction)
	{
//		this->action = (blockerAction);
		if (blockerAction.compare("forwarded") == 0)
			status = 0;
		else if (blockerAction.compare("gravity blocked") == 0)
			status = 1;
		else if (blockerAction.compare("cached") == 0)
			status = 2;
		else if ((blockerAction.compare("regex blacklisted") == 0) || (blockerAction.compare("exactly blacklisted") == 0))
			status = 3;
		else
		{
			status = 127;
			(*errorLogger) << "Unknown blocker action: >" << blockerAction << "<" << endl;
		}
	}

	clock_t insertIntoDb(int * linesInserted)
	{
		if (isInserted)
			return 0;
		else
		{
			clock_t begin = clock();
			db->insertLogEntry(timeOfQuery, domain, status, callerSubNetID);
			clock_t end = clock();
			(*linesInserted)++;	
			isInserted = true;
			return end - begin;
		}
	}

private:
	int subNetIDFromIPAddr(string ip)
	{
		// return the number after the third .
		size_t dotPos = 0;
		dotPos = ip.find(".", dotPos + 1);
		dotPos = ip.find(".", dotPos + 1);
		dotPos = ip.find(".", dotPos + 1);
		return std::stoi(ip.substr(dotPos + 1));
	}

public:
	tm timeOfQuery;
	string queryType;
	string domain = "";
	string callerIP = "";
//	string action = "";
	bool isInserted = false;

private:
	int callerSubNetID;
	int status;
	dbInterface * db;
	ofstream* errorLogger;

	string currentYearString;
	int currentMonth;
	int currentYear;
};

