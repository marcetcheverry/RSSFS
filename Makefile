CC = gcc
FUSE_CFLAGS ?= `pkg-config --cflags --libs fuse`
XML_CFLAGS ?= `xml2-config --cflags --libs`

rssfs: 
	@$(CC) $(FUSE_CFLAGS) $(XML_CFLAGS) src/http_error_codes.c src/http_fetcher.c src/rss_parser.c src/rssfs.c -o src/rssfs

all: rssfs
clean:
	@rm -f src/rssfs
