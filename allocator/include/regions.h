#ifndef __REGIONS_H__
#define __REGIONS_H__

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "segments.h"

#if defined (__ia64__) || defined (__x86_64__)
#define INT_PTR unsigned long

#else
#define INT_PTR unsigned int
#endif

struct _mem_pool{
  char *mmap_start;			// Memory mapped allocation start addresss
  char* start_address;			// Aligned start address of TeraCache
  char* cur_alloc_ptr;			// Current allocation pointer of TeraCache
  char* stop_address;			// Last address of TeraCache
  uint64_t size;			// Current allocated bytes in TeraCache
};
    
struct region_list{
  char *start;
  char *end;
  struct region_list *next;
};

extern struct _mem_pool tc_mem_pool;	// Allocator pool
extern int fd;				// File descriptor for the opended file
extern int num_reqs;			// Number of asynchronous write requests

uint64_t total_h2_regions();

// Initialize allocator with start address 'heap_end + 1'. The end of the
// heap. The path indicates the path to create the file for H2 and its size.
//void       init(uint64_t alignment, const char* path, uint64_t size);
void       init(uint64_t alignment, const char* path, uint64_t size, char* heap_end);
// Return the unaligned address of the mmaped region
char*      start_mmap_region(void);
// Return the start address of the memory allocation pool
char*      start_addr_mem_pool(void);	
// Return the size of the memory allocation pool
size_t     mem_pool_size(void);

// Allocate a new object with `size` and return the `start allocation
// address`.
char *     allocate(size_t size, uint64_t rdd_id, uint64_t partition_id);

// Return the last address of the memory allocation pool
char*      stop_addr_mem_pool(void);

// Return the current allocation pointer
char*      cur_alloc_ptr(void);

// Return 'true' if the allocator is empty, 'false' otherwise.
int        r_is_empty(void);

// Close allocator and unmap pages
void	   r_shutdown(void);

// Give advise to kernel to expect page references in sequential order.  (Hence,
// pages in the given range can be aggressively read ahead, and may be freed
// soon after they are accessed.)
void	   r_enable_seq(void);

// Give advise to kernel to expect page references in random order (Hence, read
// ahead may be less useful than normally.)
void	   r_enable_rand(void);

// Explicit write 'data' with 'size' in certain 'offset' using system call
// without memcpy.
void	   r_write(char *data, char *offset, size_t size);

// Explicit asynchronous write 'data' with 'size' in certain 'offset' using
// system call without memcpy.
void	   r_awrite(char *data, char *offset, size_t size);

// Check if all the asynchronous requestes have been completed
// Return 1 on succesfull, and 0 otherwise
int		   r_areq_completed(void);

// We need to ensure that all the writes will be flushed from buffer
// cur_alloc_ptrcur_alloc_ptrhe and they will be written to the device.
void		r_fsync(void);

// This function if for the FastMap hybrid version. Give advise to kernel to
// serve all the pagefault using regular pages.
void	   r_enable_regular_flts(void);

// This function if for the FastMap hybrid version. Give advise to kernel to
// serve all the pagefault using huge pages.
void	   r_enable_huge_flts(void);

// This function is to get the start address of the mmaped space
// for H2
unsigned long	 r_get_mmaped_start(void);
#if 1//perpap
void* aligned_mmap(size_t size, size_t alignment, void *address, int prot, int flags, int fd, off_t offset);
bool is_aligned(char *p, uintptr_t N);
uintptr_t available_virtual_space(void *address1, void *address2);
int is_address_mapped(pid_t pid, uintptr_t address); 
//void* align_ptr_up(void* ptr, size_t alignment);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __REGIONS_H__ */
