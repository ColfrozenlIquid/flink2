#include "../../include/source/source.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>

ssize_t fd_read(void* ctx, void* dst, size_t cap, int timeout_ms) {
    int fd = ((fd_source_t*)ctx)->fd;

    // Poll the file descriptor for new data. Avoids a busy loop
    if (timeout_ms >= 0) {
        struct pollfd p = {.fd = fd, .events = POLLIN};
        int rc = poll(&p, 1, timeout_ms);
        if (rc == 0) {
            // No data, try again
            return SRC_AGAIN;
        }
        if (rc < 0) {
            // EINTR: A signal occurred before any requested event -> try again.
            // Else return error
            return errno == EINTR ? SRC_AGAIN : SRC_ERR;
        }
    }
    ssize_t bytes = recv(fd, dst, cap, 0);
    if (bytes < 0) {
        // No bytes received. Try again
        return errno == EINTR ? SRC_AGAIN : SRC_ERR;
    }

    return bytes;
}

ssize_t mem_read(void* ctx, void* dst, size_t cap, int timeout_ms) {
    (void)timeout_ms;
    mem_source_t* m = ctx;
    // End of file reached
    if (m->pos == m->len) {
        return SRC_EOF;
    }
    // How many Bytes are left
    size_t bytes = m->len - m->pos;
    if (bytes > cap) {
        bytes = cap;
    }
    // Check max chunk size
    if (m->max_chunk && bytes > m->max_chunk) {
        bytes = m->max_chunk;
    }
    memcpy(dst, m->data + m->pos, bytes);
    m->pos += bytes;
    return (ssize_t)bytes;
}
