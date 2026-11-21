#pragma once
#include <stdint.h>
#include <stdio.h>

#define SRC_EOF 0    // clean end of stream
#define SRC_ERR -1   // stream error
#define SRC_AGAIN -2 // no data right now, try again

typedef struct {
    ssize_t (*read)(void* ctx, void* dst, size_t cap, int timeout_ms);
    void* ctx;
} source_t;

typedef struct {
    int fd;
} fd_source_t;

ssize_t fd_read(void* ctx, void* dst, size_t cap, int timeout_ms);

typedef struct {
    const uint8_t* data;
    size_t len;
    size_t pos;
    size_t
        max_chunk; // cap on bytes per read operation. (0 = unlimited). Simulate TCP fragmentation
} mem_source_t;

ssize_t mem_read(void* ctx, void* dst, size_t cap, int timeout_ms);
