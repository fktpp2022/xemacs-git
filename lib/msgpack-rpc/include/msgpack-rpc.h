#ifndef MSGPACK_RPC_H
#define MSGPACK_RPC_H

#include <msgpack.h>

int msgpack_rpc_pack_request(msgpack_packer *pk, int id, const char *method, msgpack_object params);
int msgpack_rpc_pack_response(msgpack_packer *pk, int id, msgpack_object result);
int msgpack_rpc_pack_notification(msgpack_packer *pk, const char *method, msgpack_object params);
int msgpack_rpc_unpack_request(msgpack_unpacker *up, int *id, char **method, msgpack_object *params);

#endif