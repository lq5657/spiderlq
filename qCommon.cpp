#include "qCommon.h"

#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

extern vector<string> vSeeds;
extern vector<string> vKwords;
extern int maxDeepth;

int load_conf(const char* confFile)
{
    ifstream fin(confFile);
    char line[128] = {0};
    char *p = NULL;
    string s;
    while(fin.getline(line, sizeof(line))){
        p = strchr(line, ':');
        if(p != NULL){
            p = p + 1;
            s.assign(p);
        }
        if(strstr(line, "SEED") != NULL){
            /* is seed url */
            vSeeds.push_back(s);
        } else if(strstr(line, "KEYWORDS") != NULL) {
            /* is keyword */
            vKwords.push_back(s);
        } else if(strstr(line, "MAXDEEPTH") != NULL) {
            maxDeepth = atoi(p);
        }
    }
    return 0;
}

