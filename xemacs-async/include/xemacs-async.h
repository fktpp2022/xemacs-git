#ifndef XEMACS_ASYNC_H
#define XEMACS_ASYNC_H

#include <uv.h>
#include <msgpack.h>

typedef struct {
    uv_loop_t *loop;
    uv_tcp_t rpc_server;
    msgpack_sbuffer *sbuffer;
    msgpack_unpacker *unpacker;
} async_core_t;

extern async_core_t async_core;

int async_init(void);
int async_run(void);
void async_stop(void);

int rpc_server_init(const char *socket_path);
void rpc_server_shutdown(void);

typedef struct {
    uv_tcp_t handle;
    char *name;
} rpc_client_t;

int rpc_client_connect(rpc_client_t *client, const char *name, const char *socket_path);
int rpc_client_send(rpc_client_t *client, const char *method, msgpack_object params);
void rpc_client_disconnect(rpc_client_t *client);

typedef void (*file_watch_callback)(const char *path, int events);

int file_watch_add(const char *path, file_watch_callback callback);
int file_watch_remove(const char *path);

#define MAX_SERVICES 16

typedef struct {
    char name[64];
    char socket_path[256];
    int pid;
} service_info_t;

int service_register(const char *name, const char *socket_path);
int service_unregister(const char *name);
const service_info_t *service_find(const char *name);
int service_list(service_info_t *services, int max_count);

#endif