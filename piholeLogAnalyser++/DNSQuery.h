#pragma once
#include "dbInterface.h"
#include <string>
#include <ctime>
#include "grok.h"

class dnsQuery
{
public:
	bool isInserted = false;

private:
	tm _timeOfQuery;
	//string _queryType;
	string _domain = "";
	string _callerIP = "";
	int _callerSubNetID;
	int _status;
	dbInterface* _db;
	ofstream* _errorLogger;
	string _currentYearString;
	int _currentMonth;
	int _currentYear;

public:
	dnsQuery(dbInterface * databaseInterface, ofstream* errorFile)
	{
		time_t rightNow = time(0);
		tm * tmNow = localtime(&rightNow);
		_currentYearString = std::to_string(1900 + tmNow->tm_year);
		_currentMonth = tmNow->tm_mon;
		_currentYear = tmNow->tm_year;

		_db = databaseInterface;
		_errorLogger = errorFile;
	}

	void setRequest(tm logTimeOfCall, string targetDomain, string callerIPAddr)
	{
		_timeOfQuery = logTimeOfCall;

		// There is no year part to the time stamp on the log record so
		// start by assuming that the log record is in the current year
		_timeOfQuery.tm_year = _currentYear;

		// check if the calculated time is less than now() i.e. it happened in the past
		// The log records loaded on 1 Jan are mostly from Dec last year so assuming the current year is the year of the log line that would put Dec records in the future
		if (_timeOfQuery.tm_mon > _currentMonth)
		{
			_timeOfQuery.tm_year -= 1;
		}

		_domain = targetDomain;
		_callerIP = callerIPAddr;
		_callerSubNetID = subNetIDFromIPAddr(callerIPAddr);
		isInserted = false;
	}

	void setRequest(string logTimeOfCall, string targetDomain, string callerIPAddr)
	{
		strptime(logTimeOfCall.c_str(), "%b %d %H:%M:%S", &_timeOfQuery);
		setRequest(_timeOfQuery, targetDomain, callerIPAddr);
	}

	void blockerAction(string blockerAction)
	{
//		this->action = (blockerAction);
		if (blockerAction.compare("forwarded") == 0)
			_status = 0;
		else if (blockerAction.compare("gravity blocked") == 0)
			_status = 1;
		else if (blockerAction.compare("cached") == 0)
			_status = 2;
		else if ((blockerAction.compare("regex blacklisted") == 0) || (blockerAction.compare("exactly blacklisted") == 0))
			_status = 3;
		else
		{
			_status = 127;
			(*_errorLogger) << "Unknown blocker action: >" << blockerAction << "<" << endl;
		}
	}

	clock_t insertIntoDb(int * linesInserted)
	{
		if (isInserted)
			return 0;
		else
		{
			clock_t begin = clock();
			_db->insertLogEntry(_timeOfQuery, _domain, _status, _callerSubNetID);
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

};

