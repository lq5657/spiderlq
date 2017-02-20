#include "qPage.h"

#include <cassert>
#include <cstring>
#include <cstdlib>

int page_parse(char* htmlPtr, parseURL_t* pUrl)
{
    GumboOutput* output = gumbo_parse(htmlPtr);
    if(output == NULL)
        return 1;
    gumbo_extractURL(output->root, pUrl);

    /* find title */
    char *title = NULL;
    int ret =  gumbo_findTitle(output->root, title);
    
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return 0;
}

void gumbo_extractURL(GumboNode* node, parseURL_t* pUrl)
{
    if(node->type != GUMBO_NODE_ELEMENT) {
        return;
    }

    GumboAttribute* gumboAttr;
    if(node->v.element.tag == GUMBO_TAG_A){
        if((gumboAttr = gumbo_get_attribute(&node->v.element.attributes, "href")) != NULL) {
            /* find link url in page html */
            char *link = strdup(gumboAttr->value);            /* free link in attach_domain */
            if(is_bin_url(link)){                          /* exclude .jpg.jpeg.gif.png.ico.bmp.swf */
                free(link);
            } else {
                char *url = attach_domain(link, pUrl->domain);    /* free link and alloc memory for url */
                if(url != NULL){
                    sourURL_t *sUrl = (sourURL_t*)malloc(sizeof(sUrl)); 
                    sUrl->level = pUrl->level + 1;
                    sUrl->type = TYPE_HTML;
                    /* normalize url, remove http(s) */
                    if((sUrl->url = url_normalized(url)) == NULL){    /* url_normalized alloc memory for sUrl->url and free url */
                        /* url is unnormal */
                        free(sUrl);
                    } else {
                        if(is_crawled(sUrl->url)){   /* url already be crawled before */
                            free(sUrl->url);
                            free(sUrl);
                        } else{
                            push_sourURL_queue(sUrl);
                        }
                    }
                }
            }
        }    
    }

    GumboVector* children = &node->v.element.children;
    for(unsigned int i = 0; i < children->length; ++i){
        gumbo_extractURL(static_cast<GumboNode*>(children->data[i]), pUrl);
    }
}
/*
 * @root html's root node
 * @title return the page's title
 * if return is 0, then find title success, else if return is 1, then find empty title, else not find ,then return -1.
 */
int gumbo_findTitle(GumboNode* root, char *title)
{

    assert(root->type == GUMBO_NODE_ELEMENT);
    assert(root->v.element.children.length >= 2);

    const GumboVector* root_children = &root->v.element.children;
    GumboNode* head = NULL;
    for (int i = 0; i < (int)root_children->length; ++i) {
        GumboNode* child = (GumboNode*)root_children->data[i];
        if (child->type == GUMBO_NODE_ELEMENT &&
            child->v.element.tag == GUMBO_TAG_HEAD) {
              head = child;
              break;
        }
    }
    assert(head != NULL);

    GumboVector* head_children = &head->v.element.children;
    for (int i = 0; i < (int)head_children->length; ++i) {
        GumboNode* child = (GumboNode*)head_children->data[i];
        if (child->type == GUMBO_NODE_ELEMENT &&
            child->v.element.tag == GUMBO_TAG_TITLE) {
              if (child->v.element.children.length != 1) {
                    return 1;       //empty title
              }
              GumboNode* title_text = (GumboNode*)child->v.element.children.data[0];
              assert(title_text->type == GUMBO_NODE_TEXT || title_text->type == GUMBO_NODE_WHITESPACE);
              title = strdup(title_text->v.text.text);
              return 0;
        }
    }
    return -1;           //"<no title found>"
}
