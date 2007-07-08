/*
 * rssfs
 * Copyright Marc E. 2007
 * $Id$
 */

#define FUSE_USE_VERSION  26 
#include <fuse.h>
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <string.h>
#include <asm/errno.h>
#include <fcntl.h>
#include "http_fetcher.h"
#include "rss_parser.h"

// The linked list struct for our data
RssData * datalist;

static long long int file_size;


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
        file_size;

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
    int res;

    syslog(LOG_INFO, "Opening file path: %s", path+1);

    // Check if its in our data list
    if (findRecordByTitle(datalist, path+1) == -1) {
        syslog(LOG_ERR, "No entity!");
        return -ENOENT;
    }

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int rssfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char *file_content;
    (void) fi;

    syslog(LOG_INFO, "Reading file path: %s", path+1);

    // Check the path
    if (findRecordByTitle(datalist, path+1) == -1) {
        return -ENOENT;
    }

    // Get the torrent

    file_content = fetch_url(getRecordUrlByTitle(datalist, path+1));
    file_size = sizeof(file_content)/sizeof(char) -1;

    syslog(LOG_INFO, file_content);


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

int main(int argc, char *argv[]) {

    if (argc != 2)
        return(1);
    //char * surl = "http://www.btarg.com.ar/tracker/rss.php?feed=dl&cat=6,1,2,7,11,12,9,10,15,13,14,3,4,8,5&passkey=f53897bb25ff0d053d5cd9535ef9e21a";
    char * surl = "http://www.supertorrents.org/rss.php?passkey=38f15d1430602e32593bc3fc3292620d&dd=1";

    printf("Loading RSS feed...\n");
    // Fetch our RSS data
    datalist = loadRSS(surl);

    syslog(LOG_INFO, "Init");

    return fuse_main(argc, argv, &rssfs_oper);
}
