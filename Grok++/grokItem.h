#pragma once
#include <string>
#include <iostream>

using namespace std;

class grokItem
{
public:
    grokItem(string Key, string Value)
    {
        key = Key;
        value = Value;
    }
    ~grokItem()
    {
        //cout << "item: " << Key << " " << Value << " is being destroyed." << endl;
    }

private:
    string key;
    string value;

};

