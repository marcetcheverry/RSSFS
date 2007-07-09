#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

struct MemoryStruct {
    unsigned char * memory;
    size_t size;
};

void *myrealloc(void *ptr, size_t size) {
    /* There might be a realloc() out there that doesn't like reallocing
       NULL pointers, so we take care of it here */
    if(ptr)
        return realloc(ptr, size);
    else
        return malloc(size);
}

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    // The number of bytes, times the number of elements its sending. (i.e, 4 chunks of 10 bytes)
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = (unsigned char *)myrealloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}


// Fetch a URL, returns the file size in int, or NULL if there is an error. Pass a char pointer reference to get the data.
long int fetch_url(char *url, char **fileBuf) {
    CURL *curl_handle;

    struct MemoryStruct chunk;

    chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // Cookie test
    curl_easy_setopt(curl_handle, CURLOPT_COOKIE, "uid=xxxx; pass=xxx;");

    /* get it! */
    curl_easy_perform(curl_handle);

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);


    *fileBuf = (char *)chunk.memory;
/*--------------------------------------------------
*     int i;
*     for (i = 0; i < chunk.size; i++) {
*         printf("%c", chunk.memory[i]);
*     }
*--------------------------------------------------*/

    if(chunk.memory) {
        free(chunk.memory);
        return (long int)chunk.size;
    } else {
        return -1;
    }
}

