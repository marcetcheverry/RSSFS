/*
 * rssfs
 *
 * Copyright 2007 Marc E.
 * http://www.jardinpresente.com.ar/wiki/index.php/Main_Page
 *
 * RSS parser functions and storage headers
 * $Id$
 */

#include <libxml/parser.h>
#include <libxml/tree.h>

// This linked list holds the RSS data
typedef struct RssData {
    int number;
    char title[255];
    char link[255];
    long int size;
    struct RssData *next;
} RssData;

RssData * addRecord(RssData *datalist, int counter, const xmlChar *title, const xmlChar *link, long int size);
void printRecord(RssData *datalist);
void printAllRecords(RssData *datalist);
int findRecordByTitle(RssData *datalist, const char *title);
char * getRecordUrlByTitle(RssData *datalist, const char *title);
long int getRecordFileSizeByTitle(RssData *datalist, const char *title);
RssData * loadRSS(char *url);

