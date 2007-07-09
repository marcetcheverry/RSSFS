/*
 * rssfs
 *
 * Copyright 2007 Marc E.
 * http://www.jardinpresente.com.ar/wiki/index.php/RSSFS
 *
 * RSS parser functions and storage handlers
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rssfs.h"
#include "rss_parser.h"
#include "http_fetcher.h"

// Invalid characters in filenames
char invalid_char[10] = {'/', '\\', '?', '%', '*', ':', '|', '"', '<', '>'};


// Checks filename, and replaces invalid chars the invalid_char character array.
char *checkFilename(char *filename) {
    if (strcspn(filename, invalid_char) != strlen(filename)) {
        int i;
        char * stringptr;
        // Needed
        char * filenamecopy = strdup(filename);

        for (i = 0; i < sizeof(invalid_char); i++) {
            if ((stringptr = strchr(filenamecopy, invalid_char[i])) != NULL) {
                *stringptr = '.';
            } 
        }
        return filenamecopy;
    } else {
        return filename;
    }
}

// Adds a record to a RssData struct
RssData * addRecord(RssData *datalist, int counter, const xmlChar *title, const xmlChar *link, long int size) {
    //printf("Add %d: %s - %s!\n", counter, (char *)title, (char *)link);

    // Clean out invalid chars
    char *titleclean = checkFilename((char *)title);

    RssData * new = malloc(sizeof(RssData));
    if (new == NULL) {
        fprintf(stderr, "Could not allocate memory\n");
    }
    new->number = counter;
#ifdef RSSEXT
    sprintf(new->title, "%s%s", titleclean, RSSEXT);
#else
    sprintf(new->title, "%s", titleclean);
#endif
    sprintf(new->link, "%s", link);
    new->size = size;
    new->next = datalist;
    datalist = new;
    return datalist;
}

void printRecord(RssData *datalist) {
    printf("Number: %d\n", datalist->number);
    printf("Title: %s\n", datalist->title);
    printf("Link: %s\n", datalist->link);
}

void printAllRecords(RssData *datalist) {
    if (datalist != NULL) {
        while (datalist != NULL) {
            printRecord(datalist);
            datalist = datalist->next;
        }
    } else {
        printf("Error: No records have been entered yet!\n");
    }
}

// Returns -1 for not found, and 0 for found.
int findRecordByTitle(RssData *datalist, const char *title) {
    int found = 0;
    RssData *current = datalist;
    while ((current != NULL) && !found) {
        if (!strcmp(current->title, title)) {
            found = 1;
            return 0;
        } else {
            current = current->next;
        }
    }
    return -1;
}

// Returns the url by title, or (char *)-1 if it cant be found.
char * getRecordUrlByTitle(RssData *datalist, const char *title) {
    int found = 0;
    RssData *current = datalist;
    while ((current != NULL) && !found) {
        if (!strcmp(current->title, title)) {
            found = 1;
            return (char *)current->link;
        } else {
            current = current->next;
        }
    }
    // FIXME
    return (char *)-1;
}

// Returns the file size as a long int, or -1 if it can't be found
long int getRecordFileSizeByTitle(RssData *datalist, const char *title) {
    int found = 0;
    RssData *current = datalist;
    while ((current != NULL) && !found) {
        if (!strcmp(current->title, title)) {
            found = 1;
            return current->size;
        } else {
            current = current->next;
        }
    }
    return -1;
}

static RssData * iterate_xml(xmlNode *root_node) {
    xmlNode * root_node_children;
    xmlNode * channel_node_children;
    xmlNode * item_node_children;
    xmlChar * link;
    xmlChar * title;
    RssData * datalist = NULL;
    int counter = 0;
    char * file_content;
    long int size;

    // Top level (<rss>)
    for (; root_node; root_node = root_node->next) {
        if(root_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        // Sub level (<channel>)
        for (root_node_children = root_node->children; root_node_children; root_node_children = root_node_children->next) {
            if (root_node_children->type != XML_ELEMENT_NODE) {
                continue;
            }

            // Channel sub level (<title>, <item>, etc)
            for (channel_node_children = root_node_children->children; channel_node_children; channel_node_children = channel_node_children->next) {
                // We only care about <item>
                if ((channel_node_children->type != XML_ELEMENT_NODE) || (strcmp((const char *)channel_node_children->name, "item") != 0)) {
                    continue;
                }

                // Each <item> is a recordset counted
                counter++;

                // Item sub level (<title>, <link>, <description>)
                for (item_node_children = channel_node_children->children; item_node_children; item_node_children = item_node_children->next) {

                    // We don't care about description
                    if ((item_node_children->type != XML_ELEMENT_NODE) || (strcmp((const char *)item_node_children->name, "description") == 0)) {
                        continue;
                    }

                    // Set our link and titles
                    if (strcmp((const char *)item_node_children->name, "link") == 0) {
                        link = item_node_children->children->content;
                        // Download and calculate file size
                        size = fetch_url((char *)link, &file_content);
                        if (size == -1) {
                            size = 0;
                        }
                    } else if (strcmp((const char *)item_node_children->name, "title") == 0) {
                        title = item_node_children->children->content;
                    }
                }
                // Add the item's data to a linked list
                datalist = addRecord(datalist, counter, title, link, size);
                //printf("%d %s %s\n", counter, link, title);
            }
        }
    }
    return datalist;
}

#ifdef LIBXML_TREE_ENABLED
RssData * loadRSS(char *url) {
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    LIBXML_TEST_VERSION

        doc = xmlReadFile(url, NULL, 0);

    if (doc == NULL) {
        return NULL;
    }
    root_element = xmlDocGetRootElement(doc);


    RssData * datalist = iterate_xml(root_element);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return datalist;
}

#else
fprintf(stderr, "Tree support not compiled in\n");
exit(1);
#endif
