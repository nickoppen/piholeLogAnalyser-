#pragma once

#include <memory>
#include <list>
#include "grokNamedSubExpression.h"

using namespace std;

class grokResult
{

private:
	map<string, grokNamedSubExpression> _items;
	map<unsigned, string> _indexedValues;


public:
	bool matched;
	bool timedOut;

public:
	grokResult()
	{
		matched = false;
		timedOut = false;
	}

	~grokResult()
	{
		//for (list<grokNamedSubExpression>::iterator listI = items.begin(); listI != items.end(); listI++)
		//{
		//	listI.erase();
		//}
	}

	void reset()
	{
		_indexedValues.clear();
		matched = false;
		timedOut = false;
	}

	void addItemResult(string expressionName, string expValue)
	{
		if (_items.find(expressionName) != _items.end())
		{
			_items[expressionName].setValue(expValue);
			return;
		}

		grokNamedSubExpression newItem(expressionName);
		newItem.setValue(expValue);

		_items.insert(make_pair(expressionName, newItem));
	}

	void addItemResult(unsigned index, string expressionValue)
	{
		_indexedValues.insert(make_pair(index, expressionValue));
	}

	void addItemWithType(string expressionName, string expressionType)
	{
		grokNamedSubExpression newItem(expressionName, expressionType);

		_items.insert(make_pair(expressionName, newItem));

	}

	grokNamedSubExpression operator[](string expressionName)
	{
		return _items[expressionName];
	}

	string operator[](unsigned i)
	{
		return _indexedValues[i];
	}
};

