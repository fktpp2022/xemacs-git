#include "xemacs-async.h"
#include <stdio.h>
#include <assert.h>

int main() {
    assert(async_init() == 0);
    
    assert(rpc_server_init("/tmp/xemacs-async.sock") == 0);
    
    assert(service_register("elisp", "/tmp/xemacs-elisp.sock") == 0);
    
    assert(file_watch_add("/tmp", NULL) >= 0);
    
    printf("All integration tests passed!\n");
    return 0;
}