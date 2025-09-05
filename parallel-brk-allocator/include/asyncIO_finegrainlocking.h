#ifndef __ASYNCIO_H__
#define __ASYNCIO_H__

#include <aio.h>
#include <stdint.h>
#include <threads.h>
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
};

// Thread-local queue for I/O requests
typedef struct {
    struct ioRequest requests[MAX_REQS];  // Local queue of requests
    int count;                            // Number of requests in the queue
} ThreadLocalQueue;

// Global queue for completed requests
extern struct ioRequest global_completed_queue[MAX_REQS];
extern int global_completed_count;
extern mtx_t global_queue_lock;  // Lock for the global queue

// Initialize the global queue
void req_init(void);

// Add a new I/O request to the thread-local queue
void req_add(int fd, char *data, size_t size, uint64_t offset);

// Check if all I/O requests in the thread-local queue have been completed
int is_all_req_completed(void);

// Merge completed requests from the thread-local queue into the global queue
void merge_completed_requests(ThreadLocalQueue *local_queue);

#ifdef __cplusplus
}
#endif

#endif // __ASYNCIO_H__
