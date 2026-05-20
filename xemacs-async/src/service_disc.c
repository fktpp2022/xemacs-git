#include "xemacs-async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static service_info_t services[MAX_SERVICES];
static int service_count = 0;

int service_register(const char *name, const char *socket_path) {
    if (service_count >= MAX_SERVICES) return -1;
    strncpy(services[service_count].name, name, sizeof(services[service_count].name)-1);
    strncpy(services[service_count].socket_path, socket_path, sizeof(services[service_count].socket_path)-1);
    services[service_count].pid = getpid();
    service_count++;
    return 0;
}

int service_unregister(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            memmove(&services[i], &services[i+1], (service_count-i-1)*sizeof(service_info_t));
            service_count--;
            return 0;
        }
    }
    return -1;
}

const service_info_t *service_find(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return &services[i];
        }
    }
    return NULL;
}

int service_list(service_info_t *services_out, int max_count) {
    int count = (service_count < max_count) ? service_count : max_count;
    memcpy(services_out, services, count * sizeof(service_info_t));
    return count;
}