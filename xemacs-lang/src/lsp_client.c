#include "xemacs-lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lsp_start_server(const char *language, const char *command) {
    uv_process_options_t options = {0};
    options.file = command;
    
    uv_process_t *process = malloc(sizeof(uv_process_t));
    uv_process_init(uv_default_loop(), process);
    return uv_spawn(uv_default_loop(), process, &options);
}

int lsp_send_request(lsp_server_t *server, const char *method, msgpack_object params) {
    return 0;
}

void lsp_stop_server(lsp_server_t *server) {
    uv_process_kill(&server->process, SIGTERM);
    free(server);
}