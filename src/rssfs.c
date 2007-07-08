/*
 * rssfs
 *
 * Copyright 2007 Marc E.
 * http://www.jardinpresente.com.ar/wiki/index.php/RSSFS
 *
 * FUSE module
 * $Id$
 */

#define FUSE_USE_VERSION  26 
#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <string.h>
#include <asm/errno.h>
#include <fcntl.h>

#include "rssfs.h"
#include "http_fetcher.h"
#include "rss_parser.h"

// The linked list struct for our data
RssData * datalist;

static long long int file_size;

// Replaces characters in a string
char * str_replace(const char *str, const char *character, const char *replace) {
    char* var;
    char* tmp_pos;
    char* needle_pos;

    int count;
    int len;
    if( strlen (character) < strlen (replace) ){

        count = 0;
        tmp_pos = (char*)str;
        while( needle_pos = (char*) strcasestr( tmp_pos, character ) ){

            tmp_pos = needle_pos + strlen (character);
            count++;

        }
        len = strlen(str) + (strlen(replace) - strlen(character)) * count;
        var = (char*) malloc( sizeof(char) * (len + 1) );

    }	
    else {

        len = strlen(str);
        var = (char*) malloc( sizeof(char) * (len+1) );
        memset( var, 0, ( sizeof(char) * (len+1) ) );

    }
    tmp_pos = (char*) str;
    while( needle_pos = (char*)strcasestr( tmp_pos, character ) ){

        len = needle_pos - tmp_pos;

        strncat( var, tmp_pos, len );
        strcat( var, replace);

        tmp_pos = needle_pos + strlen (character);

    }
    strcat( var, tmp_pos );
    return var;
}

static int rssfs_getattr(const char *path, struct stat *stbuf) {
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(findRecordByTitle(datalist, path+1) == 0) {
        stbuf->st_mode = S_IFREG | 0555;
        stbuf->st_nlink = 1;
        stbuf->st_size = getRecordFileSizeByTitle(datalist, path+1);
    }
    else
        res = -ENOENT;

    return res;
}

static int rssfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    // No directories
    if(strcmp(path, "/") != 0) {
        return -ENOENT;
    } else {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        
        // Fill from the data
        RssData *current = datalist;
        if (current != NULL) {
            while (current != NULL) {
                // TODO: Filter out invalid characters
                filler(buf, str_replace(current->title, "/", "-"), NULL, 0);
                current = current->next;
            }
        }
    }
    return 0;
}

static int rssfs_open(const char *path, struct fuse_file_info *fi) {
    int res;

    #ifdef DEBUG
    syslog(LOG_INFO, "Opening file path: %s", path+1);
    #endif

    // Check if its in our data list
    if (findRecordByTitle(datalist, path+1) == -1) {
        #ifdef DEBUG
        syslog(LOG_ERR, "No entity!");
        #endif
        return -ENOENT;
    }

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int rssfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char *file_content;
    (void) fi;

    #ifdef DEBUG
    syslog(LOG_INFO, "Reading file path: %s", path+1);
    #endif

    // Check the path
    if (findRecordByTitle(datalist, path+1) == -1) {
        return -ENOENT;
    }

    // Get the torrent

    file_content = fetch_url(getRecordUrlByTitle(datalist, path+1));
    file_size = sizeof(file_content)/sizeof(char) -1;

    #ifdef DEBUG
    syslog(LOG_INFO, file_content);
    #endif


    if (offset >= file_size) { /* Trying to read past the end of file. */
        return 0;
    }

    if (offset + size > file_size) { /* Trim the read to the file size. */
        size = file_size - offset;
    }

    memcpy(buf, file_content + offset, size); /* Provide the content. */

    return size;
}

static int rssfs_write(const char *path) {
}

static struct fuse_operations rssfs_oper = {
    .getattr    = rssfs_getattr,
    .readdir    = rssfs_readdir,
    .open       = rssfs_open,
    .read       = rssfs_read,
};

/* Print usage information on stderr */
void usage(char * argve) {
    fprintf(stderr, "Usage: %s url mount-point\n", argve);
    fprintf(stderr, "Version: %s\n", VERSION);
}

/* Main function */
int main(int argc, char *argv[]) {
    char *fusev[2];

    if (argc != 3) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Loading RSS feed...\n");

    // Fetch our RSS data
    datalist = loadRSS(argv[1]);

    if (datalist == NULL) {
        fprintf(stderr, "Could not open or parse: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
        
    #ifdef DEBUG
    syslog(LOG_DEBUG, "Init rssfs");
    #endif

    // Don't send the URL data to fuse.
    fusev[0] = argv[0];
    fusev[1] = argv[2];
    return fuse_main(argc-1, fusev, &rssfs_oper);
}
