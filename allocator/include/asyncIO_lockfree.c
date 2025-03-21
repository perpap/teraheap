#ifndef __ASYNCIO_H__
#define __ASYNCIO_H__

#include <aio.h>
#include <stdint.h>
#include <stdatomic.h>
#include "../include/sharedDefines.h"

#ifdef __cplusplus
extern "C" {
#endif

// Application-defined structure for tracking I/O requests
struct ioRequest {    
    int            state;      // Status of request
    char *         buffer;     // Internal buffer
    struct aiocb   aiocbp;     // Asynchronous I/O control block
#if !MALLOC_ON
    size_t         size;
#endif
    struct ioRequest *next;    // Pointer to the next request in the lock-free queue
};

// Lock-free queue structure
typedef struct {
    _Atomic(struct ioRequest *) head;  // Head of the queue
    _Atomic(struct ioRequest *) tail;  // Tail of the queue
} LockFreeQueue;

// Global lock-free queues
extern LockFreeQueue request_queue;    // Queue for pending I/O requests
extern LockFreeQueue completed_queue;  // Queue for completed I/O requests

// Initialize the lock-free queues
void req_init(void);

// Add a new I/O request to the lock-free queue
void req_add(int fd, char *data, size_t size, uint64_t offset);

// Check if all I/O requests have been completed
int is_all_req_completed(void);

// Process completed requests from the lock-free queue
void process_completed_requests(void);

#ifdef __cplusplus
}
#endif

#endif // __ASYNCIO_H__
