#pragma once

#include <memory>
#include <list>
#include "grokNamedSubExpression.h"

using namespace std;

class grokResult
{

private:
	//list<unique_ptr<grokNamedSubExpression>> items;
	map<string, grokNamedSubExpression> items;
	map<unsigned, string> indexedValues;


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
		indexedValues.clear();
		matched = false;
		timedOut = false;
	}

	void addItemResult(string expressionName, string expValue)
	{
		if (items.find(expressionName) != items.end())
		{
			items[expressionName].setValue(expValue);
			return;
		}

		//unique_ptr<grokNamedSubExpression> newItem = make_unique<grokNamedSubExpression>(expName);
		grokNamedSubExpression newItem(expressionName);
		newItem.setValue(expValue);

		items.insert(make_pair(expressionName, newItem));
	}

	void addItemResult(unsigned index, string expressionValue)
	{
		indexedValues.insert(make_pair(index, expressionValue));
	}

	void addItemWithType(string expressionName, string expressionType)
	{
		//unique_ptr<grokNamedSubExpression> newItem = make_unique<grokNamedSubExpression>(expressionName, expressionType);
		grokNamedSubExpression newItem(expressionName, expressionType);

		items.insert(make_pair(expressionName, newItem));

	}

	grokNamedSubExpression operator[](string expressionName)
	{
		return items[expressionName];
	}

	string operator[](unsigned i)
	{
		return indexedValues[i];
	}
};

