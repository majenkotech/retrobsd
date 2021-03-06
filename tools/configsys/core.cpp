#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "util.h"
#include "core.h"
#include "cluster.h"

using namespace std;

bool core::load(const char *filename)
{
    ifstream f;
    char line[1024];

    f.open(filename,ios::in);
    if(!f.is_open())
    {
        cout << "Unable to open core " << filename << endl;
        return false;
    }

    while(f.getline(line,1023))
    {
        stringstream temp(line);
        string in;
        temp >> in;
        uc(in);
        if(in.substr(0,1) == "#")
            continue;

        if(in == "MACHINE")
        {
            this->data.read(f);
        }

        if(in == "ALWAYS")
        {
            this->always.read(f);
        }
        if(in == "OPTION")
        {
            temp >> in;
            uc(in);
            this->options[in].read(f);
        }
    }

    f.close();
    return true;
}

