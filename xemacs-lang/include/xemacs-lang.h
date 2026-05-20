#ifndef XEMACS_LANG_H
#define XEMACS_LANG_H

#include <uv.h>
#include <msgpack.h>
#include <tree_sitter/api.h>

typedef struct {
    char *language;
    char *command;
    uv_process_t process;
} lsp_server_t;

int lsp_start_server(const char *language, const char *command);
int lsp_send_request(lsp_server_t *server, const char *method, msgpack_object params);
void lsp_stop_server(lsp_server_t *server);

typedef struct {
    TSParser *parser;
    TSLanguage *language;
} ts_context_t;

int ts_init(ts_context_t *ctx, const char *language_name);
int ts_parse(ts_context_t *ctx, const char *source, size_t length, TSTree **tree);
void ts_query(ts_context_t *ctx, TSTree *tree, const char *query_string);
void ts_cleanup(ts_context_t *ctx);

#endif