#include "xemacs-async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/inotify.h>
#endif

int file_watch_add(const char *path, file_watch_callback callback) {
#ifdef _WIN32
    HANDLE hDir = CreateFileA(path, FILE_LIST_DIRECTORY, 
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) return -1;
    
    OVERLAPPED ov = {0};
    char buf[1024];
    ReadDirectoryChangesW(hDir, buf, sizeof(buf), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, &ov, NULL);
    return 0;
#else
    static int inotify_fd = -1;
    if (inotify_fd < 0) {
        inotify_fd = inotify_init1(IN_NONBLOCK);
        if (inotify_fd < 0) return -1;
    }
    return inotify_add_watch(inotify_fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
#endif
}

int file_watch_remove(const char *path) {
    return 0;
}