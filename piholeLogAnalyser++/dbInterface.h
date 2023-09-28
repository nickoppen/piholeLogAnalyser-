#pragma once
#include "piholeLogAnalyserDefs.h"

#include <mariadb/conncpp.hpp>

#include <ctime>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace sql;
using namespace std::chrono_literals;

// The standard template library does time very badly.
// This is the best way around the lack of functionality relating to file date/time that I've found
template <typename TP>
std::time_t to_time_t(TP tp)
{
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
	return system_clock::to_time_t(sctp);
}

class dbInterface
{
private:
	unique_ptr<Connection> _conn;
	unique_ptr<PreparedStatement> _insertStatement;
	ofstream* _errorLogger;
	bool _isADryRun;
	stringstream _ss;

public:
	dbInterface(bool dryRun, ofstream * errLogger)
	{
		_isADryRun = dryRun;
		_errorLogger = errLogger;
	}

	~dbInterface()
	{

	}

	bool ckeckDbServer(cliArgs* args)		/// add database name, ip address and port
	{
		if (open(args))
		{
			close();
			(*_errorLogger) << "Check succeeded." << endl;
			return  true;
		}

		(*_errorLogger) << "Check failed." << endl;
		return false; // failed to open
	}

	bool open(cliArgs *args)		/// add database name, ip address and port
	{
		// there is a 256 byte memory leak here somewhere
		//Do not delete driver - a null pointer error occurs
		Driver* driver = mariadb::get_driver_instance();

		// Configure Connection
		SQLString url("jdbc:mariadb://"+args->serverIPAddress+":"+args->serverPortNumber+"/"+args->databaseName);
		Properties properties({
								{"user", (args->dbUserName).c_str()},
								{"password", (args->dbUserPwd).c_str()}
							});

		// Establish Connection
		try
		{
			unique_ptr<sql::Connection> connectShon(driver->connect(url, properties));
			_conn = move(connectShon);
		}
		catch (exception & ex)
		{
			(*_errorLogger) << ex.what() << endl;
			return false;
		}

		// Prepare the addLogEntry statement for repeated use
		string sql = "CALL addLogEntry(?, ?, ?, ?)";
		unique_ptr<PreparedStatement> statement(_conn->prepareStatement(sql));
		_insertStatement = move(statement);
		
		return true;
	}

	void close()
	{
		//if (!stmnt->isClosed())
		//	stmnt->close();
		if (!_conn->isClosed())
			_conn->close();
	}

	void maxDate(tm * maxD)
	{
		string sql = "CALL maxDateYYYYMMDDHourMinSec;";
		unique_ptr<sql::PreparedStatement> stmnt(_conn->prepareStatement("CALL maxDateYYYYMMDDHourMinSec;"));		

		try
		{
			sql::ResultSet* res = stmnt->executeQuery();
			res->first();
			while (!res->isAfterLast())
			{
				maxD->tm_year = res->getInt(1) - 1900;
				maxD->tm_mon = res->getInt(2) - 1; // to make it consistent with struct tm
				maxD->tm_mday = res->getInt(3);
				maxD->tm_hour = res->getInt(4);
				maxD->tm_min = res->getInt(5);
				maxD->tm_sec = res->getInt(6);
				res->next();	// should move past EOF
			}
			stmnt->getMoreResults();	// flush the rest of the buffer
			res->close();
			delete res;
		}
		catch (exception & ex)
		{
			(*_errorLogger) << "Error on executeQuery in maxDate :" << ex.what() << endl;
		}

		return;
	}

	bool updateTblCommon()
	{
		string sql = "CALL appendToCommon;";
		unique_ptr<sql::PreparedStatement> stmnt(_conn->prepareStatement(sql));

		try
		{
			if (!_isADryRun)
			{
				ResultSet * res = stmnt->executeQuery();
				res->close();
				delete res;		// to prevent a memory leak
			}
			else
				cout << sql << endl;
		}
		catch (exception& ex)
		{
			(*_errorLogger) << "Error on executeQuery in updateTblCommon :" << ex.what() << endl;
			return false;
		}
		return true;
	}

	bool updateLevelOfInterestFromDate(tm todaysStartDate)
	{
		string sql = "CALL updateAllLOIFromDate('" + formatDateTime(todaysStartDate) + "');";
		unique_ptr<sql::PreparedStatement> stmnt(_conn->prepareStatement(sql));

		try
		{
			if (!_isADryRun)
			{
				ResultSet* res = stmnt->executeQuery();
				res->close();
				delete res;		// to prevent a memory leak
			}
			else
				cout << sql << endl;
		}
		catch (exception& ex)
		{
			(*_errorLogger) << "Error on executeQuery in updateAllLOIFromDate :" << ex.what() << endl;
			return false;
		}
		return true;
	}

	bool insertLogEntry(tm logDateTime, string domain, int status, int callerSubNetID)
	{
		string formatedDateTime = formatDateTime(logDateTime);
		//string sql = "CALL addLogEntry('" + domain + "', " + std::to_string(callerSubNetID) + ", " + std::to_string(status) + ", '" + formatedDateTime + "')";
		
		try
		{
			_insertStatement->setString(1, domain);
			_insertStatement->setInt(2, callerSubNetID);
			_insertStatement->setInt(3, status);
			_insertStatement->setString(4, formatDateTime(logDateTime));

			if (!_isADryRun)  // for testing
			{
				sql::ResultSet * res = _insertStatement->executeQuery();
				res->close();
				delete res;		// to prevent a memory leak
			}
			else
				cout << "addLogEntry: " << domain << " at: " << formatedDateTime << " status: " << status << " from: " << callerSubNetID << endl;
		}
		catch (exception& ex)
		{
			(*_errorLogger) << "Error on executeQuery in insertLogEntry :" << ex.what() << endl;
			return false;
		}

		return true;
	}

	bool recordLogFile(long fileSize, filesystem::file_time_type fileDateTime, string * logKey)
	{
		(*logKey) = std::to_string(fileSize) + ", '" + formatDateTime(fileDateTime);
		string sql = "CALL recordLogFile(" + *logKey + "')";
		unique_ptr<sql::PreparedStatement> stmnt(_conn->prepareStatement(sql));

		try
		{
			if (!_isADryRun)
			{
				ResultSet* res = stmnt->executeQuery();
				res->close();
				delete res;		// to prevent a memory leak
			}
			else
				cout << sql << endl;
			return true;
		}
		catch (exception &ex)
		{
			(*_errorLogger) << "Record Log File Error (insert): " << ex.what() << endl;
			// return the sql error code for main to distinguish duplicates (to skip the file) from other errors (abort)
			return false;
		}
	}

	bool updateLogFileRecord(string * logKey, int logRowsProcessed, string comment)//long fileSize, filesystem::file_time_type fileDateTime, int logRowsProcessed, string comment)
	{
		// UPDATE will not fail if fileSize or fileDateTime has changed since RecordLogFile was called
		//unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("CALL updateLogFileRecord(" + std::to_string(fileSize) + ", '" + formatDateTime(fileDateTime) + "', " + std::to_string(logRowsProcessed) + ", '" + comment + "')"));
		string sql = "CALL updateLogFileRecord(" + *logKey + "', " + std::to_string(logRowsProcessed) + ", '" + comment + "')";
		unique_ptr<PreparedStatement> stmnt(_conn->prepareStatement(sql));

		try
		{
			if (!_isADryRun)
			{
				ResultSet* res = stmnt->executeQuery();
				res->close();
				delete res;		// to prevent a memory leak
			}
			else
				cout << sql << endl;
			return true;
		}
		catch (exception &ex)
		{
			(*_errorLogger) << "Record Log File Error (update): " << ex.what() << endl;
			return false;
		}
	}

	const bool willExecuteDbCommands()
	{
		return !_isADryRun;
	}

private:
	string formatDateTime(tm dt)
	{
		//stringstream _ss;	// now a member variable
		_ss.str("");
		_ss << put_time(&dt, "%Y-%m-%d %T");
		return  _ss.str();
	}

	string formatDateTime(filesystem::file_time_type fileDateTime)
	{
		std::time_t cftime = to_time_t(fileDateTime);				// see above
		tm local = *(std::localtime(&cftime));
		return formatDateTime(local);
	}
};

