/*
 * rssfs
 *
 * Copyright 2007 Marc E.
 * http://www.jardinpresente.com.ar/wiki/index.php/Main_Page
 *
 * rssfs global variables and includes
 * $Id$
 */

#define VERSION "0.1"

// Extension to use in the file names (this appends to the rss <title> tag, remove it if you dont want one.
#define RSSEXT ".torrent"

// Whether to print LOG_DEBUG messages to syslog
#define DEBUG 1

#ifdef DEBUG
#include <syslog.h>
#endif
