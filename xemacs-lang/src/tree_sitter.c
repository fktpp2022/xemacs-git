#include "xemacs-lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ts_init(ts_context_t *ctx, const char *language_name) {
    ctx->parser = ts_parser_new();
    ts_parser_set_language(ctx->parser, ctx->language);
    return 0;
}

int ts_parse(ts_context_t *ctx, const char *source, size_t length, TSTree **tree) {
    *tree = ts_parser_parse_string(ctx->parser, NULL, source, length);
    return (*tree != NULL) ? 0 : -1;
}

void ts_query(ts_context_t *ctx, TSTree *tree, const char *query_string) {
}

void ts_cleanup(ts_context_t *ctx) {
    ts_parser_delete(ctx->parser);
}