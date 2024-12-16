#pragma once
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/sharedDefines.h"
#include "../include/regions.h"
#include "../include/segments.h"
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>

void initialize_h1(uint64_t alignment, const char *mount_point, uint64_t size, char *address);
void initialize_h2(uint64_t gc_threads, uint64_t alignment, const char *mount_point, uint64_t size, char *address);
void print_heap_statistics();
unsigned long long convert_string_to_number(const char *str);

static inline void* align_ptr_up(void* ptr, size_t alignment) {
  return (void *) ( ((uintptr_t)ptr + alignment - 1) & ~(alignment - 1) );
}

/*
static inline void check_errno(){
    if(errno){
      exit(EXIT_FAILURE);
    }
}
*/
typedef struct _heap{
  uint64_t alignment;
  uint64_t size;
  char *start_address;
  char *end_address;
  const char *mount_point;
  //int flags;
  //int protections;
  //int fd;
}heap;


static heap h1, h2;

void initialize_h1(uint64_t alignment, const char *mount_point, uint64_t size, char *address){
  h1.alignment = alignment;
  h1.size = size;
  h1.mount_point = mount_point;
  h1.start_address = aligned_mmap(size, alignment, NULL, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (h1.start_address == NULL) {
    fprintf(stderr, "Failed to allocate h1\n");
    exit(EXIT_FAILURE);
  }
  h1.end_address = (char *)((uintptr_t)h1.start_address + size);
}

void initialize_h2(uint64_t gc_threads, uint64_t alignment, const char *mount_point, uint64_t size, char *address){
  h2.alignment = alignment;
  h2.size = size;
  h2.mount_point = mount_point;
  init(gc_threads, alignment, mount_point, size, address);
  h2.start_address = start_addr_mem_pool();
  h2.end_address = stop_addr_mem_pool();
}

void print_heap_statistics(){
    printf("h1 mapped at: %p\n", h1.start_address);
    // Calculate the end address of h1
    uintptr_t h1_end = (uintptr_t)h1.start_address + h1.size;
    printf("h1 is aligned? %d\n", is_aligned(h1.start_address, h1.alignment));
    //printf("h1_end is aligned? %d\n", is_aligned((void *)h1_end, H2_ALIGNMENT));
    printf("h1_end is aligned? %d\n", is_aligned(align_ptr_up((char *)h1_end, h2.alignment), h2.alignment));
    // Init allocator
    //init(H2_ALIGNMENT, "/spare2/perpap/fmap/", H2_SIZE, (void *)h1_end);
    printf("h2 mapped at: %p\n", h2.start_address);
    printf("%-20s %-20s %-20s %-20s %-20s %-20s %-20s\n", "HEAP", "START_ADDRESS", "END_ADDRESS", "SIZE(GB)", "ALIGNMENT(KB)", "CARDSIZE(KB)", "PAGESIZE(KB)");
    printf("%-20s %-20p %-20p %-20td %-20zd %-20zd %-20zd\n", "H1", h1.start_address, (char *)h1_end, CONVERT_TO_GB((ptrdiff_t)((char *)h1_end-h1.start_address)), CONVERT_TO_KB(H1_ALIGNMENT), CONVERT_TO_KB(H1_CARD_SIZE), CONVERT_TO_KB(PAGE_SIZE));
    printf("%-20s %-20p %-20p %-20td %-20zd %-20zd %-20zd\n", "H2", h2.start_address, h2.end_address, CONVERT_TO_GB((ptrdiff_t)(h2.end_address-h2.start_address)), CONVERT_TO_KB(H2_ALIGNMENT), CONVERT_TO_KB(H2_CARD_SIZE), CONVERT_TO_KB(PAGE_SIZE));

    //printf("%-20s %-20s %-20s %-20s %-20s %-20s\n", "", "START_ADDRESS(HEX)", "START_ADDRESS(DEC)", "END_ADDRESS(HEX)", "END_ADDRESS(DEC)", "SIZE(GB)");
    //printf("%-20s %-20p %-20llu %-20p %-20llu %-20td\n", "H1", h1.start_address, (unsigned long long)h1.start_address, (char *)h1_end, (unsigned long long)h1_end,CONVERT_TO_GB((ptrdiff_t)((char *)h1_end-h1.start_address)));
    //printf("%-20s %-20p %-20llu %-20p %-20llu %-20td\n", "H2", h2.start_address, (unsigned long long)h2.start_address, h2.end_address, (unsigned long long)h2.end_address,CONVERT_TO_GB((ptrdiff_t)(h2.end_address-h2.start_address)));
}

unsigned long long convert_string_to_number(const char *str){
    errno = 0;
    char *endptr={NULL};
    unsigned long long int value = strtoull(str, &endptr, 10);

    // Check for various possible errors
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid number: %s\n", str);
        errno = EINVAL;
        return 0;
    }

    if (value == ULLONG_MAX && errno == ERANGE) {
        fprintf(stderr, "Number out of range: %s\n", str);
        return value;
    }

    return value;
}

