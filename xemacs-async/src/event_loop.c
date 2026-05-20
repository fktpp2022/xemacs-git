#include "xemacs-async.h"
#include <stdio.h>

async_core_t async_core;

int async_init(void) {
    async_core.loop = uv_default_loop();
    if (!async_core.loop) {
        fprintf(stderr, "Failed to create event loop\n");
        return -1;
    }
    
    async_core.sbuffer = msgpack_sbuffer_new();
    async_core.unpacker = msgpack_unpacker_new(0);
    if (!async_core.sbuffer || !async_core.unpacker) {
        fprintf(stderr, "Failed to initialize msgpack\n");
        return -1;
    }
    
    return 0;
}

int async_run(void) {
    return uv_run(async_core.loop, UV_RUN_DEFAULT);
}

void async_stop(void) {
    uv_stop(async_core.loop);
    msgpack_sbuffer_destroy(async_core.sbuffer);
    msgpack_unpacker_destroy(async_core.unpacker);
}