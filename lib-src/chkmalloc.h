#ifndef CHKMALLOC_H
#define CHKMALLOC_H

#include <stdlib.h>

static inline void *
trace_malloc(const char *file, int line, size_t size)
{
  return malloc(size);
}

static inline void *
trace_realloc(const char *file, int line, void *ptr, size_t size)
{
  return realloc(ptr, size);
}

#endif
