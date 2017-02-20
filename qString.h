#ifndef _Q_STRING_H_
#define _Q_STRING_H_

/* Trim the string. This function will NOT allocate new memory */
extern char * strim(char *str);

/* Split string.
 * count: size of splitted strings
 * limit: how many times to split string 
 */
extern char ** strsplit(char *line, char delimeter, int *count, int limit);

#endif
