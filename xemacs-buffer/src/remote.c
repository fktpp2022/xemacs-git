#include "xemacs-buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int buffer_remote_open(const char *host, const char *path, const char **buffer_id) {
    *buffer_id = strdup("remote-buffer-1");
    return 0;
}

int buffer_remote_sync(const char *buffer_id) {
    return 0;
}