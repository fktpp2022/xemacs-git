#include "xemacs-async.h"
#include <assert.h>

int main() {
    assert(async_init() == 0);
    async_stop();
    return 0;
}