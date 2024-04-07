#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <stdbool.h>

static struct input_handle {
        char *path;
        char *buffer;
        size_t length;
} empty_input_handle = {.path = NULL, .buffer = NULL, .length = 0};

/**
 * Read the given file into an input_handle object. This will free the previous
 * instance if signs of previous use are detected, i.e. if legnth != 0,
 * path != NULL or buffer != NULL
 **/
_Bool input_read(const char *, struct input_handle *);

void input_free(struct input_handle *);

#endif
