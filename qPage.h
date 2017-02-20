#ifndef _Q_PAGE_H_
#define _Q_PAGE_H_
#include <gumbo.h>
#include "qURL.h"

int page_parse(char* htmlPtr, parseURL_t* pUrl);

void gumbo_extractURL(GumboNode* node, parseURL_t* pUrl);

int gumbo_findTitle(GumboNode* node, char *title);

#endif
