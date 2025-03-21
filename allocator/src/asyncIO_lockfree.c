#include <asm-generic/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <aio.h>
#include <unistd.h>
#include "../include/asyncIO.h"
#include <threads.h>

// Global lock-free queues
LockFreeQueue request_queue = { .head = NULL, .tail = NULL };
LockFreeQueue completed_queue = { .head = NULL, .tail = NULL };

// Initialize the lock-free queues
void req_init() {
    // No initialization needed for atomic pointers
}

// Add a new I/O request to the lock-free queue
void req_add(int fd, char *data, size_t size, uint64_t offset) {
    // Allocate a new I/O request
    struct ioRequest *new_request = malloc(sizeof(struct ioRequest));
    if (!new_request) {
        #if DEBUG_PRINT
        fprintf(allocator_log_fp, "[%s|%s|%d] Failed to allocate memory for I/O request\n", __FILE__, __func__, __LINE__);
        #endif
        exit(EXIT_FAILURE);
    }

    // Initialize the I/O request
    new_request->state = EINPROGRESS;
    new_request->aiocbp = (struct aiocb){0};
    new_request->aiocbp.aio_fildes = fd;
    new_request->aiocbp.aio_offset = offset;

#if MALLOC_ON
    new_request->buffer = malloc(size * sizeof(char));
#else
    new_request->buffer = malloc(size * sizeof(char));
    new_request->size = size;
#endif

    memcpy(new_request->buffer, data, size);
    new_request->aiocbp.aio_buf = new_request->buffer;
    new_request->aiocbp.aio_nbytes = size;
    new_request->next = NULL;

    // Add the request to the lock-free queue
    struct ioRequest *tail = atomic_load(&request_queue.tail);
    while (1) {
        if (atomic_compare_exchange_weak(&request_queue.tail, &tail, new_request)) {
            if (tail) {
                tail->next = new_request;
            } else {
                atomic_store(&request_queue.head, new_request);
            }
            break;
        }
    }

    // Submit the I/O request
#ifdef ASSERT
    int check = aio_write(&new_request->aiocbp);
    assertf(check == 0, "Write failed");
#else
    aio_write(&new_request->aiocbp);
#endif
}

// Process completed requests from the lock-free queue
void process_completed_requests() {
    struct ioRequest *head = atomic_load(&completed_queue.head);
    while (head) {
        // Check if the request is completed
        if (head->state == 0) {
            // Free the buffer if necessary
#if MALLOC_ON
            if (head->buffer) {
                free(head->buffer);
                head->buffer = NULL;
            }
#endif
            // Move to the next request
            struct ioRequest *next = head->next;
            free(head);
            head = next;
        } else {
            break;
        }
    }
    atomic_store(&completed_queue.head, head);
}

// Check if all I/O requests have been completed
int is_all_req_completed() {
    struct ioRequest *head = atomic_load(&request_queue.head);
    while (head) {
        if (head->state == EINPROGRESS) {
            head->state = aio_error(&head->aiocbp);
            if (head->state == 0) {
                // Move the completed request to the completed queue
                struct ioRequest *next = head->next;
                head->next = NULL;

                struct ioRequest *tail = atomic_load(&completed_queue.tail);
                while (1) {
                    if (atomic_compare_exchange_weak(&completed_queue.tail, &tail, head)) {
                        if (tail) {
                            tail->next = head;
                        } else {
                            atomic_store(&completed_queue.head, head);
                        }
                        break;
                    }
                }

                head = next;
            } else {
                return 0;  // Not all requests are completed
            }
        } else {
            head = head->next;
        }
    }
    return 1;  // All requests are completed
}
