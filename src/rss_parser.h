/*
 * RSSFS - RSS in userspace.
 * http://www.jardinpresente.com.ar/wiki/index.php/RSSFS
 *
 * Copyright (C) 2007 Marc E. <santusmarc@users.sourceforge.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

extern char invalid_char[10];

RssData * addRecord(RssData *datalist, int counter, const xmlChar *title, const xmlChar *link, long int size);
void printRecord(RssData *datalist);
void printAllRecords(RssData *datalist);
int findRecordByTitle(RssData *datalist, const char *title);
char * getRecordUrlByTitle(RssData *datalist, const char *title);
long int getRecordFileSizeByTitle(RssData *datalist, const char *title);
RssData * loadRSS(char *url);

