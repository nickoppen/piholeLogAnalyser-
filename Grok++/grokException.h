#pragma once

#include <string>
#include <exception>

using namespace std;

class grokException : public exception
{
public:
    explicit grokException(string msg) : exception()
    {
        message = msg;
    }

    virtual const char* what()  const noexcept
    {
        return message.c_str();
    }

private:
    string message;

};

