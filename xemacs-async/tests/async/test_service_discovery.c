#include "xemacs-async.h"
#include <assert.h>
#include <string.h>

int main() {
    assert(async_init() == 0);
    
    // Test service registration
    assert(service_register("test-service", "/tmp/test-socket") == 0);
    
    // Test service lookup
    const service_info_t *info = service_find("test-service");
    assert(info != NULL);
    assert(strcmp(info->name, "test-service") == 0);
    assert(strcmp(info->socket_path, "/tmp/test-socket") == 0);
    
    // Test listing services
    service_info_t services[MAX_SERVICES];
    int count = service_list(services, MAX_SERVICES);
    assert(count == 1);
    assert(strcmp(services[0].name, "test-service") == 0);
    
    // Test service unregistration
    assert(service_unregister("test-service") == 0);
    
    // Verify service is gone
    info = service_find("test-service");
    assert(info == NULL);
    
    async_stop();
    return 0;
}
