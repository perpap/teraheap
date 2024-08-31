#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <aio.h>

#include "../include/regions.h"
#include "../include/asyncIO.h"
#include "../include/segments.h"
#include "../include/sharedDefines.h"

#define HEAPWORD (8)                       // In the JVM the heap is aligned to 8 words
#define HEADER_SIZE (32)                   // Header size of the Dummy object	

#define align_size_up_(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))
static inline char* align_ptr_up(char* ptr, uintptr_t alignment) {
  return (char *) ( ((uintptr_t)ptr + alignment - 1) & ~(alignment - 1) );
}
static void check_address(pid_t pid, uintptr_t address);

char dev[150] = {'\0'};
uint64_t dev_size = 0;
static uint64_t mmap_size = 0;
uint64_t region_array_size = 0;
uint64_t max_rdd_id = 0;
uint64_t group_array_size = 0;
struct _mem_pool tc_mem_pool;
int fd;

FILE *allocator_log_fp = NULL;

static void calculate_h2_region_array_size(){
  mmap_size = (uintptr_t)tc_mem_pool.stop_address - (uintptr_t)tc_mem_pool.start_address;
  region_array_size = /*dev_size*/mmap_size / REGION_SIZE;
  max_rdd_id = region_array_size / MAX_PARTITIONS;
  group_array_size = region_array_size / 2; // deprecated
  assertf(region_array_size >= MAX_PARTITIONS,
          "Device size should be larger, because region_array_size is "
          "calculated to be smaller than MAX_PARTITIONS!");
}

// Function to check if the address is already mapped
int is_address_mapped(pid_t pid, uintptr_t target_addr) {
    char maps_file[256];
    snprintf(maps_file, sizeof(maps_file), "/proc/%d/maps", pid);

    FILE *file = fopen(maps_file, "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[256];
    uintptr_t start, end;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            if (target_addr >= start && target_addr < end) {
                fclose(file);
                return 1; // Address is already mapped
            }
        }
    }

    fclose(file);
    return 0; // Address is not mapped
}

void check_h2_addresses(){
  pid_t pid = getpid();
  char *h2_addresses_msg[] = {"unaligned H2 base address", "aligned H2 base address", "H2 end address", NULL};
  char *h2_addresses[] = {tc_mem_pool.mmap_start, tc_mem_pool.start_address, tc_mem_pool.stop_address, NULL};
  int i = 0;
  for(char *address = *h2_addresses, *msg = *h2_addresses_msg; address != NULL; address = h2_addresses[i], msg = h2_addresses_msg[i]){
    printf("Checking %-30s: ", msg);
    check_address(pid, (uintptr_t)address);
    ++i;
  }
}

void check_address(pid_t pid, uintptr_t address){ 
  if (is_address_mapped(pid, address)) {
      printf("Address %16p is already mapped.\n", (char *)address);
  } else {
      printf("Address %16p is not mapped.\n", (char *)address);
  }
}

uintptr_t available_virtual_space(void *address1, void *address2){
  assertf((uintptr_t)address2 >= (uintptr_t)address1, "There is no available virtual space");
  return CONVERT_TO_GB(((uintptr_t)address2 - (uintptr_t)address1));
}

bool is_aligned(char *p, uintptr_t N)
{
    return ((uintptr_t)p & (uintptr_t)(N - 1)) == 0;
}

void create_file(const char* path, uint64_t size) {
  assertf(size >= 1024*1024*1024LU, "Size should be grater than 1GB");
  size_t max_len = strnlen(path, 150);

  strncpy(dev, path, max_len + 1);

  if (dev[max_len + 1] != '\0') {
    perror("[Error] - Strncpy failed");
    exit(EXIT_FAILURE);
  }

	strcat(dev,".XXXXXX");
  dev_size = size;
  fd = mkstemp(dev);
  unlink(dev);
  assertf(fd >= 1, "tempfile error.");
  int status = posix_fallocate(fd, 0, /*dev_size*/size);
  if (status != 0) {
    fprintf(stderr, "[%s|%s|%d]Fallocate error\n",__FILE__,__func__,__LINE__);
  }
}

// Initialize allocator
void init(uint64_t align, const char* path, uint64_t size, char* h1_end) {
    fd = -1;
    assertf((allocator_log_fp = fopen("allocator_log", "w")) != NULL, "Allocator logger failed!");

    create_file(path, size);
    // Memory-mapped a file over a storage device
    tc_mem_pool.mmap_start = mmap(0, /*dev_size*/size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    assertf(tc_mem_pool.mmap_start != MAP_FAILED, "Mapping Failed");
    // Card table in JVM needs the start address of TeraCache to be align up
    tc_mem_pool.start_address = align_ptr_up(tc_mem_pool.mmap_start, align); 
    tc_mem_pool.cur_alloc_ptr = tc_mem_pool.start_address;
    tc_mem_pool.size = 0;
    tc_mem_pool.stop_address = tc_mem_pool.mmap_start + /*dev_size*/size;
#ifdef ASSERT
    static const char *border = "-----------------------------------------------------------";
    fprintf(allocator_log_fp, "%s\n[%s|%s|%d]Use memory-mapped IO for H2 using a file of %zd GB...\n%-30s = %p\n%-30s = %p\n%-30s = %p\n%s\n", border, __FILE__, __func__, __LINE__, CONVERT_TO_GB(size), "tc_mem_pool.mmap_start", tc_mem_pool.mmap_start, "tc_mem_pool.start_address", tc_mem_pool.start_address, "tc_mem_pool.stop_address", tc_mem_pool.stop_address, border);
#endif
    calculate_h2_region_array_size();
    init_regions();
    req_init();
#ifdef ASSERT
    fclose(allocator_log_fp);
#endif
}

void* aligned_mmap(size_t size, size_t alignment, void *address, int prot, int flags, int fd, off_t offset){
    // Request extra memory to ensure we can align the allocation
    size_t map_size = size + alignment;
    errno = 0;
    void* map = tc_mem_pool.mmap_start = mmap(address, map_size, prot, flags, fd, offset);
    //void* map = mmap(address, map_size, prot, flags, fd, offset);
    if (map == MAP_FAILED) {
        fprintf(stderr, "[%s|%s|%d]mmap failed with errno:%d\n", __FILE__,__func__,__LINE__,errno);
        return NULL;
    }

    // Align the base address
    uintptr_t base = (uintptr_t)map;
    uintptr_t aligned_base = (base + alignment - 1) & ~(alignment - 1);

    // Calculate the required unmapping sizes
    size_t pre_size = aligned_base - base;
    size_t post_size = (base + map_size) - (aligned_base + size);

    // Unmap any unused regions
    if (pre_size > 0) {
        munmap(map, pre_size);
    }
    if (post_size > 0) {
        munmap((void*)(aligned_base + size), post_size);
    }

    return (void*)aligned_base;
}

char* start_mmap_region(void){
  return tc_mem_pool.mmap_start;
}

// Return the start address of the memory allocation pool
char* start_addr_mem_pool() {
	assertf(tc_mem_pool.start_address != NULL, "Start address is NULL");
	return tc_mem_pool.start_address;
}

// Return the last address of the memory allocation pool
char* stop_addr_mem_pool() {
	assertf(tc_mem_pool.stop_address != NULL, "Stop address is NULL");
	return tc_mem_pool.stop_address;
}

// Return the `size` of the memory allocation pool
size_t mem_pool_size() {
	assertf(tc_mem_pool.start_address != NULL, "Start address is NULL");
	return dev_size;
}

char* allocate(size_t size, uint64_t rdd_id, uint64_t partition_id) {
	char* alloc_ptr = tc_mem_pool.cur_alloc_ptr;
	char* prev_alloc_ptr = tc_mem_pool.cur_alloc_ptr;

	assertf(size > 0, "Object size should be > 0");

  alloc_ptr = allocate_to_region(size * HEAPWORD, rdd_id, partition_id);

  if (alloc_ptr == NULL) {
    perror("[Error] - H2 Allocator is full");
    exit(EXIT_FAILURE);
  }

    tc_mem_pool.size += size;
    tc_mem_pool.cur_alloc_ptr = (char *) (((uint64_t) alloc_ptr) + size * HEAPWORD);

	if (prev_alloc_ptr > tc_mem_pool.cur_alloc_ptr)
		tc_mem_pool.cur_alloc_ptr = prev_alloc_ptr;

	assertf(prev_alloc_ptr <= tc_mem_pool.cur_alloc_ptr, 
			"Error alloc ptr: Prev = %p, Cur = %p", prev_alloc_ptr, tc_mem_pool.cur_alloc_ptr);

	// Alighn to 8 words the pointer (TODO: CHANGE TO ASSERTION)
	if ((uint64_t) tc_mem_pool.cur_alloc_ptr % HEAPWORD != 0)
		tc_mem_pool.cur_alloc_ptr = (char *)((((uint64_t)tc_mem_pool.cur_alloc_ptr) + (HEAPWORD - 1)) & -HEAPWORD);

	return alloc_ptr;
}

// Return the current allocation pointer
char* cur_alloc_ptr() {
	assertf(tc_mem_pool.cur_alloc_ptr >= tc_mem_pool.start_address
			&& tc_mem_pool.cur_alloc_ptr < tc_mem_pool.stop_address,
			"Allocation pointer out-of-bound")

	return tc_mem_pool.cur_alloc_ptr;
}

// Return 'true' if the allocator is empty, 'false' otherwise.
// Invariant: Initialize allocator
int r_is_empty() {
	assertf(tc_mem_pool.start_address != NULL, "Allocator should be initialized");

	return tc_mem_pool.size == 0;
}

// Close allocator and unmap pages
void r_shutdown(void) {
	printf("CALL HERE");
	munmap(tc_mem_pool.mmap_start, dev_size);
}

// Give advise to kernel to expect page references in sequential order.  (Hence,
// pages in the given range can be aggressively read ahead, and may be freed
// soon after they are accessed.)
void r_enable_seq() {
	madvise(tc_mem_pool.mmap_start, dev_size, MADV_SEQUENTIAL);
}

// Give advise to kernel to expect page references in random order (Hence, read
// ahead may be less useful than normally.)
void r_enable_rand() {
	madvise(tc_mem_pool.mmap_start, dev_size, MADV_NORMAL);
}

// Explicit write 'data' with 'size' in certain 'offset' using system call
// without memcpy.
void r_write(char *data, char *offset, size_t size) {
#ifdef ASSERT
	ssize_t s_check = 0;
	uint64_t diff = offset - tc_mem_pool.mmap_start;

	s_check = pwrite(fd, data, size * HEAPWORD, diff);
	assertf(s_check == size * HEAPWORD, "Sanity check: s_check = %ld", s_check);
#else
	uint64_t diff = offset - tc_mem_pool.mmap_start;
	pwrite(fd, data, size * HEAPWORD, diff);
#endif
}
	
// Explicit asynchronous write 'data' with 'size' in certain 'offset' using
// system call without memcpy.
// Do not use r_awrite with r_write
void r_awrite(char *data, char *offset, size_t size) {
	
	uint64_t diff = offset - tc_mem_pool.mmap_start;

	req_add(fd, data, size * HEAPWORD, diff);
}
	
// Check if all the asynchronous requestes have been completed
// Return 1 on succesfull, and 0 otherwise
int	r_areq_completed() {
	return is_all_req_completed();
}
	
// We need to ensure that all the writes will be flushed from buffer
// cur_alloc_ptrcur_alloc_ptrhe and they will be written to the device.
void r_fsync() {
#ifdef ASSERT
	int check = fsync(fd);
	assertf(check == 0, "Error in fsync");
#else
	fsync(fd);
#endif
}

// This function if for the FastMap hybrid version. Give advise to kernel to
// serve all the pagefault using regular pages.
void r_enable_regular_flts(void) {
	madvise(tc_mem_pool.mmap_start, dev_size, MADV_NOHUGEPAGE);
}

// This function if for the FastMap hybrid version. Give advise to kernel to
// serve all the pagefault using huge pages.
void r_enable_huge_flts(void) {
	madvise(tc_mem_pool.mmap_start, dev_size, MADV_HUGEPAGE);
}
  
// This function is to get the start address of the mmaped space
// for H2
unsigned long r_get_mmaped_start(void) {
  return (unsigned long) tc_mem_pool.mmap_start;
}
