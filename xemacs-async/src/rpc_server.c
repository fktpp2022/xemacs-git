#include "xemacs-async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

static void on_new_connection(uv_stream_t *server, int status);
static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

static uv_tcp_t server;

int rpc_server_init(const char *socket_path) {
    struct sockaddr_in addr;
    unlink(socket_path);
    uv_tcp_init(async_core.loop, &server);
    uv_ip4_addr("127.0.0.1", 8888, &addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
    return uv_listen((uv_stream_t *)&server, 128, on_new_connection);
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(async_core.loop, client);
    uv_accept(server, (uv_stream_t *)client);
    uv_read_start((uv_stream_t *)client, alloc_buffer, on_read);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        uv_close((uv_handle_t *)client, NULL);
        free(client);
        free(buf->base);
        return;
    }
    if (nread > 0) {
        msgpack_unpacker_reserve_buffer(async_core.unpacker, nread);
        memcpy(msgpack_unpacker_buffer(async_core.unpacker), buf->base, nread);
        msgpack_unpacker_buffer_consumed(async_core.unpacker, nread);
    }
    free(buf->base);
}

void rpc_server_shutdown(void) {
    uv_close((uv_handle_t *)&server, NULL);
}