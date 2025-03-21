#include <asm-generic/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <aio.h>
#include <unistd.h>
#include "../include/asyncIO.h"
#include <threads.h>

// Global queue for completed requests
struct ioRequest global_completed_queue[MAX_REQS];
int global_completed_count = 0;
mtx_t global_queue_lock;

// Thread-local storage for I/O requests
thread_local ThreadLocalQueue local_queue = { .count = 0 };

// Initialize the global queue
void req_init() {
    if (mtx_init(&global_queue_lock, mtx_plain) != thrd_success) {
        #if DEBUG_PRINT
        fprintf(allocator_log_fp, "[%s|%s|%d] Global queue mutex initialization failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
        #endif
        exit(EXIT_FAILURE);
    }
}

// Add a new I/O request to the thread-local queue
void req_add(int fd, char *data, size_t size, uint64_t offset) {
    // No lock needed for thread-local queue

    if (local_queue.count >= MAX_REQS) {
        // If the local queue is full, merge completed requests into the global queue
        merge_completed_requests(&local_queue);
    }

    // Find an available slot in the local queue
    int slot = local_queue.count;
    local_queue.count++;

    // Initialize the I/O request
    struct aiocb obj = {0};
    obj.aio_fildes = fd;
    obj.aio_offset = offset;

#if MALLOC_ON
    // Allocate memory for the buffer
    local_queue.requests[slot].buffer = malloc(size);
    if (!local_queue.requests[slot].buffer) {
        #if DEBUG_PRINT
        fprintf(allocator_log_fp, "[%s|%s|%d] Failed to allocate memory for I/O request buffer\n", __FILE__, __func__, __LINE__);
        #endif
        exit(EXIT_FAILURE);
    }
#else
    // Reallocate memory if the size is larger than the current buffer size
    if (size > local_queue.requests[slot].size) {
        char *ptr_new = realloc(local_queue.requests[slot].buffer, size);
        if (!ptr_new) {
            #if DEBUG_PRINT
            fprintf(allocator_log_fp, "[%s|%s|%d] Failed to reallocate memory for I/O request buffer\n", __FILE__, __func__, __LINE__);
            #endif
            exit(EXIT_FAILURE);
        }
        local_queue.requests[slot].buffer = ptr_new;
        local_queue.requests[slot].size = size;
    }
#endif

    // Copy data into the buffer
    memcpy(local_queue.requests[slot].buffer, data, size);
    obj.aio_buf = local_queue.requests[slot].buffer;
    obj.aio_nbytes = size;

    local_queue.requests[slot].state = EINPROGRESS;
    local_queue.requests[slot].aiocbp = obj;

    // Submit the I/O request
#ifdef ASSERT
    int check = aio_write(&local_queue.requests[slot].aiocbp);
    assertf(check == 0, "Write failed");
#else
    aio_write(&local_queue.requests[slot].aiocbp);
#endif
}

// Merge completed requests from the thread-local queue into the global queue
void merge_completed_requests(ThreadLocalQueue *local_queue) {
    // No lock needed for thread-local queue

    for (int i = 0; i < local_queue->count; i++) {
        if (local_queue->requests[i].state == EINPROGRESS) {
            local_queue->requests[i].state = aio_error(&local_queue->requests[i].aiocbp);

            if (local_queue->requests[i].state == 0) {
                // Request completed, move it to the global queue
                mtx_lock(&global_queue_lock);  // Lock the global queue

                if (global_completed_count < MAX_REQS) {
                    global_completed_queue[global_completed_count] = local_queue->requests[i];
                    global_completed_count++;
                }

                mtx_unlock(&global_queue_lock);  // Unlock the global queue

#if MALLOC_ON
                // Free the buffer if necessary
                if (local_queue->requests[i].buffer) {
                    free(local_queue->requests[i].buffer);
                    local_queue->requests[i].buffer = NULL;
                }
#endif
                // Mark the slot as available
                local_queue->requests[i].state = 0;
            }
        }
    }
}

// Check if all I/O requests in the thread-local queue have been completed
int is_all_req_completed() {
    // No lock needed for thread-local queue

    for (int i = 0; i < local_queue.count; i++) {
        if (local_queue.requests[i].state == EINPROGRESS) {
            local_queue.requests[i].state = aio_error(&local_queue.requests[i].aiocbp);

            if (local_queue.requests[i].state == EINPROGRESS) {
                return 0;  // Not all requests are completed
            }
        }
    }

    return 1;  // All requests are completed
}
