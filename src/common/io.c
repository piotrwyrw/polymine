#include "io.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

_Bool input_read(const char *path, struct input_handle *handle)
{
        if (handle->length > 0 || handle->path || handle->buffer) {
                input_free(handle);
        }

        FILE *f = fopen(path, "r");

        if (!f) {
                return false;
        }

        fseek(f, 0, SEEK_END);
        long length = ftell(f);
        rewind(f);

        char *buffer = malloc(sizeof(char) * length);

        fread(buffer, sizeof(char), length, f);

        if (ferror(f) != 0) {
                free(buffer);
                fclose(f);
                return false;
        }

        fclose(f);

        handle->path = strdup(path);
        handle->buffer = buffer;
        handle->length = length;

        return true;
}

void input_free(struct input_handle *handle)
{
        if (handle->path) {
                free(handle->path);
                handle->path = NULL;
        }

        if (handle->buffer) {
                free(handle->buffer);
                handle->buffer = NULL;
        }

        handle->length = 0;
}

