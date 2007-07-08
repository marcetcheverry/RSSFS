#include <stdio.h>
#include <stdlib.h>
#include "rss_parser.h"
#include "http_fetcher.h"

// Extension to use in the file names (this appends to the rss <title> tag, remove it if you dont want one.
#define RSSEXT ".torrent"

// Adds a record to a RssData struct
RssData * addRecord(RssData *datalist, int counter, const xmlChar *title, const xmlChar *link, long int size) {
    //printf("Add %d: %s - %s!\n", counter, (char *)title, (char *)link);
    RssData * new = malloc(sizeof(RssData));
    new->number = counter;
    #ifdef RSSEXT
    sprintf(new->title, "%s%s", (char *)title, RSSEXT);
    #else
    sprintf(new->title, "%s", (char *)title);
    #endif
    sprintf(new->link, "%s", (char *)link);
    new->size = size;
    new->next = datalist;
    datalist = new;
    return datalist;
}

void printRecord(RssData *datalist) {
    printf("Number: %d\n", datalist->number);
    printf("Title: %s\n", datalist->title);
    printf("link: %s\n", datalist->link);
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
}

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
                        file_content = fetch_url((char *)link);
                        size = sizeof(file_content);
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
        printf("Error: could not parse file %s\n", url);
    }
    root_element = xmlDocGetRootElement(doc);

    xmlNodePtr node;

    RssData * datalist = iterate_xml(root_element);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return datalist;
}

#else
    fprintf(stderr, "Tree support not compiled in\n");
    exit(1);
#endif
