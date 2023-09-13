# Grok++
A C++ implementation of Roman Marusyk's [Grok.Net](https://github.com/Marusyk/grok.net). All credit for the design goes to him.

## To Compile
- include -pthread as a dependency in linker options

## To Run
- Copy grok-patterns (pre-made custom patterns) to the executable's directory

# PiholeLogAnalyser++
An interpreter/loader for pihole log files. Output is stored in a MariaDB database.

Written in C++ and developed on the pi.

## Prerequisites:
- An instance of MariaDB 
- MariaDB_connector_cpp available from https://mariadb.com/docs/skysql-previous-release/connect/programming-languages/cpp/install/ (this can be tricky to install)

## To Compile
- in linker directory options include -lmariadb once libmariadb.so (linked to libmariadb.so.3) is in /usr/lib/mariadb
- include stdc++fs as a linker dependency

## To Run
- Set up the database as per the database schema
- Copy your grok custom pattern file to an accessible location (default: ./grokCustom.txt) Note: if you have scheduled execution using cron then . (current directory) is the users home directory. 
- Commandline options are:

            -d Directory : directory to be scanned (default: /var/log/pihole)
            -r : scan recursively (recurse == false)
            -f 'Filename pattern' : load a specific log file with wildcards ? and \* (default: pihole.log.1) Note: '' to ensure that the wildcards are not expanded by the shell
            -p : Pretend to add to the database (default: false)
            -t milliseconds : max exec time for regex match function
            -g File name : specify the location of the grok custom pattern file (default: ./grokCustom.txt)
            -e Full path to the load error file (default: ./loadError.txt)
            -user username : log in to the database with the given username
            -pwd pwd : log into the database with the given password
            -ip IP Address : look for the database at the given IP address or url
            -port portNum : connect to the database on the given port
            -db dbName : connect to the database with the given name
            -check : checks the database, directory and file spec for log files the grok custom pattern file and then exits (output in load error file)
            -h or -help : print this text
