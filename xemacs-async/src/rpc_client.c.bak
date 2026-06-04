#include "xemacs-async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

int rpc_client_connect(rpc_client_t *client, const char *name, const char *socket_path) {
    client->name = strdup(name);
    uv_tcp_init(async_core.loop, &client->handle);
    // For now, just return success since we don't need full connect implementation
    return 0;
}

int rpc_client_send(rpc_client_t *client, const char *method, msgpack_object params) {
    msgpack_sbuffer_clear(async_core.sbuffer);
    msgpack_packer pk;
    msgpack_packer_init(&pk, async_core.sbuffer, msgpack_sbuffer_write);
    
    msgpack_pack_map(&pk, 3);
    msgpack_pack_str(&pk, 4); msgpack_pack_str_body(&pk, "type", 4);
    msgpack_pack_str(&pk, 7); msgpack_pack_str_body(&pk, "request", 7);
    msgpack_pack_str(&pk, 6); msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, strlen(method)); msgpack_pack_str_body(&pk, method, strlen(method));
    msgpack_pack_str(&pk, 6); msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_object(&pk, params);
    
    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(async_core.sbuffer->data, async_core.sbuffer->size);
    return uv_write(req, (uv_stream_t *)&client->handle, &buf, 1, NULL);
}

void rpc_client_disconnect(rpc_client_t *client) {
    uv_close((uv_handle_t *)&client->handle, NULL);
    free(client->name);
}