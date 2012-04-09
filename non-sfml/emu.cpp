#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "dcpu.h"

bool valid_number_string(char *str)
{
    for(int c = 0; str[c]; c++)
        if(!isdigit(str[c]))
            return false;

    return true;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        cerr << "usage: dcpu <flags> <filename>\n";
        return 1;
    }

    bool debug = false;
    bool fast = false;
    int time_to_kill_ms = 0;

    // -d runs in debug mode, prints out memory dump at end
    // -t <value> is the time in ms until the program exits (not very useful anymore)
    // -f will not perform cycle checks, i.e. ignore the wait(cycles) command (recomended because the timing is WAY off)

    for(int a = 1; a < argc - 1; a++)
    {
        if(!strcmp(argv[a], "-d"))
        {
            debug = true;
        }else if(!strcmp(argv[a], "-f"))
        {
            fast = true;
        }else
        {
            cerr << "invalid command <" << argv[a] << ">. ignoring\n";
        }
    }

    ifstream inFile(argv[argc - 1], ios::in);
    if(!inFile.good())
    {
        cerr << "Could not open <" << argv[argc - 1] << ">\n";
        return 1;
    }

    Dcpu cpu(inFile, debug, fast);

    cpu.run();
    
    return 0;
}
