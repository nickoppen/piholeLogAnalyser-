#pragma once

#include <string>
#include <iostream>


using namespace std;

class grokNamedSubExpression
{
private:
	string _name;
	string _value;
	enum desiredResultType { StringType, IntType, TimestampType, FloatType} _desiredType;

public:
	grokNamedSubExpression()
	{
		_desiredType = StringType;
	}

	grokNamedSubExpression(string expressionName)
	{
		_name = expressionName;
		_desiredType = StringType;
	}

	grokNamedSubExpression(string expressionName, string typeAsString) : grokNamedSubExpression(expressionName)
	{
		if (typeAsString.compare("int") == 0)
			_desiredType = IntType;
		else if (typeAsString.compare("float") == 0)
			_desiredType = FloatType;
		else if (typeAsString.compare("datetime") == 0)
			_desiredType = TimestampType;
		else
			cout << "Unsupported type: " << typeAsString << endl;
	}

	void setValue(string valueAsString)
	{
		_value = valueAsString;
	}

	bool value(int * valueAsInt, string * valueAsString = nullptr)
	{
		if (_desiredType != IntType)
			return false;

		if (valueAsString != nullptr)
			(*valueAsString) = _value;

		if (_desiredType == IntType)
		{
			(*valueAsInt) = std::stoi(_value);
			return true;
		}
		return false;  
	}

	bool value(float * valueAsFloat, string * valueAsString = nullptr)
	{
		if (_desiredType != FloatType)
			return false;

		if (valueAsString != nullptr)
			(*valueAsString) = _value;

		if (_desiredType == IntType)
		{
			(*valueAsFloat) = std::stof(_value);
			return true;
		}
		else
			return false;  
	}

	bool value(string* valueAsString)
	{
		(*valueAsString) = _value;
		return true;
	}

	bool value(tm* timeOfCall, string* valueAsString, char * dateTimeFormat)
	{
		if (_desiredType != TimestampType)
			return false;

		(*valueAsString) = _value;
		return (strptime(_value.c_str(), dateTimeFormat, timeOfCall) != NULL);
	}

	string valueAsString()
	{
		return _value;
	}

	string name()
	{
		return _name;
	}

};
