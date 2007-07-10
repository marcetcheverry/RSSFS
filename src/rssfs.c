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

// WARNING SHOULD BE LONG LONG
static long int file_size;
static char *url;

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
        if ((stbuf->st_size = getRecordFileSizeByTitle(datalist, path+1)) == -1) {
#ifdef DEBUG
            syslog(LOG_INFO, "Could not find file size!");
#endif
            stbuf->st_size = 0;
        }
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
                filler(buf, current->title, NULL, 0);
                current = current->next;
            }
        }
    }
    return 0;
}

static int rssfs_open(const char *path, struct fuse_file_info *fi) {

#ifdef DEBUG
    syslog(LOG_INFO, "Opening file path: %s, url: %s", path+1, getRecordUrlByTitle(datalist, path+1));
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

    // Get the data
    file_size = fetch_url(getRecordUrlByTitle(datalist, path+1), &file_content);

    if (file_size == -1) {
        fprintf(stderr, "Could not fetch '%s' from server", path+1);
        return -ENOENT;
    }

    if (offset >= file_size) { /* Trying to read past the end of file. */
#ifdef DEBUG
        syslog(LOG_INFO, "Trying to read past the end of file.");
#endif
        return 0;
    }

    if (offset + size > file_size) { /* Trim the read to the file size. */
        size = file_size - offset;
    }

    memcpy(buf, file_content + offset, size); /* Provide the content. */

    return size;
}

static void * rssfs_init(void) {
#ifdef DEBUG
    syslog(LOG_INFO, "Mounted");
    syslog(LOG_INFO, "Loading RSS feed '%s", url);
#endif
    // Fetch our RSS data
    datalist = loadRSS(url);

    if (datalist == NULL) {
        fprintf(stderr, "Could not open or parse: '%s'\n", url);
        exit(EXIT_FAILURE);
    } else {
#ifdef DEBUG
    syslog(LOG_INFO, "RSS feed '%s' loaded", url);
#endif 
    }
    return NULL;
}

static void rssfs_destroy(void *arg) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "Unmounted");
#endif
}

static struct fuse_operations rssfs_oper = {
    .getattr    = rssfs_getattr,
    .readdir    = rssfs_readdir,
    .open       = rssfs_open,
    .read       = rssfs_read,
    .init       = rssfs_init,
    .destroy    = rssfs_destroy,
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
    
    url = argv[1];

    // Don't send the URL data to fuse.
    fusev[0] = argv[0];
    fusev[1] = argv[2];
    return fuse_main(argc-1, fusev, &rssfs_oper);
}
