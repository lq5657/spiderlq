#include "qString.h"

#include <cstring>
#include <cstdlib>
#include <ctype.h>

char * strim(char *str)
{
    char *end, *sp, *ep;
    size_t len;

    sp = str;
    end = ep = str+strlen(str)-1;
    while(sp <= end && isspace(*sp)) sp++;
    while(ep >= sp && isspace(*ep)) ep--;
    len = (ep < sp) ? 0 : (ep-sp)+1;
    sp[len] = '\0';
    return sp;
}

char ** strsplit(char *line, char delimeter, int *count, int limit)
{
    char *ptr = NULL, *str = line;
    char **vector = NULL;

    *count = 0;
    while((ptr = strchr(str, delimeter))) {
        *ptr = '\0';
        vector = (char **)realloc(vector,((*count)+1)*sizeof(char *));
        vector[*count] = strim(str);
        str = ptr+1;
        (*count)++;	
        if (--limit == 0) break;
    }
    if (*str != '\0') {
        vector = (char **)realloc(vector,((*count)+1)*sizeof(char *));
        vector[*count] = strim(str);
        (*count)++;
    }
    return vector;
}

