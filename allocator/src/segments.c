#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <limits.h>
#include "../include/segments.h"
#include "../include/regions.h"
#include "../include/sharedDefines.h"

struct region *region_array = NULL;
struct region **id_array = NULL;
struct offset *offset_list;

int32_t		 region_enabled;
int32_t		 _next_region;

#if STATISTICS
uint32_t    total_deps = 0;
double		  alloc_elapsedtime = 0.0;
double		  free_elapsedtime = 0.0;
#endif

/*
 * Initialize region array, group array and their fields
 */
void init_regions(){
  int32_t i;

  region_enabled = -1;
  offset_list = NULL;
  int32_t rdd_id_size = MAX_PARTITIONS * max_rdd_id;

  region_array=(struct region*) malloc(region_array_size* sizeof(struct region));
  if (region_array == NULL) {
    perror("[Error] - Failed to allocate memory for region_array\n");
    exit(EXIT_FAILURE);
  }

  id_array=(struct region**) malloc(( MAX_PARTITIONS * max_rdd_id) * sizeof(struct region*));
  if (id_array == NULL) {
    perror("[Error] - Failed to allocate memory for id_array\n");
    exit(EXIT_FAILURE);
  }

#if DEBUG_PRINT 
  fprintf(allocator_log_fp, "[%s|%s|%d]Total num of regions:%" PRIu64 "\nrdd_id_size:%d\n", __FILE__, __func__, __LINE__, region_array_size, rdd_id_size);
#endif

  for (i = 0; i < region_array_size ; i++) {
    region_array[i].start_address             = (i == 0) ? start_addr_mem_pool() : (region_array[i - 1].start_address + (uint64_t) REGION_SIZE);
    region_array[i].used                      = 0;
    region_array[i].last_allocated_end        = region_array[i].start_address;
    region_array[i].last_allocated_start      = NULL;
    region_array[i].first_allocated_start     = NULL;
    region_array[i].dependency_list           = NULL;
    region_array[i].rdd_id                    = rdd_id_size;
    region_array[i].part_id                   = rdd_id_size;
#if PR_BUFFER
    region_array[i].pr_buffer                 = malloc(sizeof(struct pr_buffer));
    region_array[i].pr_buffer->buffer         = NULL;
    region_array[i].pr_buffer->size           = 0;
    region_array[i].pr_buffer->alloc_ptr      = NULL;
    region_array[i].pr_buffer->first_obj_addr = NULL;
#if 1//perpap
    if(mtx_init(&region_array[i].mutex, mtx_plain) == thrd_error){
        #if DEBUG_PRINT
    	fprintf(allocator_log_fp, "[%s|%s|%d]Mutex initialization failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
        #endif
	exit(EXIT_FAILURE);
    }	     
#endif
//assertf(mtx_init(&region_array[i].mutex, mtx_plain) == thrd_error, "Mutex initialization failed! error:%d",thrd_error);
#endif
  }
  for (i = 0; i < rdd_id_size; i++) {
    id_array[i]                               = NULL;
  }
}

/*
 * Returns the start of cont_regions empty regions
 */
int32_t get_cont_regions(int32_t cont_regions){
  static int32_t i = 0;
  int32_t j, index, end_index = i;

  for(; i < (region_array_size + end_index); i++) {
    for (j = i ; j < (i + cont_regions); j++) {
      if (region_array[j % (int32_t) region_array_size].last_allocated_end == 
          region_array[j % (int32_t) region_array_size].start_address) 
        continue;
      else
        break;
    }

    // Find region
    if (((j - 1) % (int32_t) region_array_size) == ((i % (int32_t) region_array_size) + cont_regions -1 )) {
      index = i;
      i = j % (int32_t) region_array_size;
      return index;
    }
    else
      i = j;
  }

  return -1;
}


/*
 * Finds an empty region and returns its starting address
 * Arguments: size is the size of the object we want to allocate (in
 * Bytes)

 * mark unused
 *  marking phase
        mark_used
   precompaction phase
      new address
 */
char* new_region(size_t size){
  int32_t i;
  int32_t cont_regions = (size % REGION_SIZE != 0) ? (size / REGION_SIZE) + 1 : (size / REGION_SIZE);
  int32_t cur_region = get_cont_regions(cont_regions);

  if (cur_region == -1)
    return NULL;

  for (i = cur_region ; i < cur_region + cont_regions ; i++){
    assertf(region_array[i].used == 0, "Error, write to an already used region");
    mark_used(region_array[i].start_address);
    references(region_array[cur_region].start_address, region_array[i].start_address);
    //references(region_array[i].start_address, region_array[cur_region].start_address);
    region_array[i].last_allocated_start = region_array[cur_region].start_address;
    region_array[i].first_allocated_start = region_array[cur_region].start_address;
    region_array[i].last_allocated_end = region_array[cur_region].start_address + size;
  }

  return region_array[cur_region].start_address;
}

uint64_t get_id(uint64_t rdd_id, uint64_t partition_id){
  assertf(partition_id < MAX_PARTITIONS, "Partition id out of bounds");
    return (rdd_id % max_rdd_id) * MAX_PARTITIONS + partition_id;
}

char* allocate_to_region(size_t size, uint64_t rdd_id, uint64_t partition_id) {
#if STATISTICS
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);
#endif

  int32_t id_index = get_id(rdd_id, partition_id);
  if (id_array[id_index] == NULL) {
    char* res = new_region(size);
  
    if (res == NULL) {
      perror("[Error] - H2 Allocator is full");
      exit(EXIT_FAILURE);
    }
    /* If object spans more than 1 region we don't want to allocate more objects with it*/
    if (size < (uint64_t) REGION_SIZE) {
      id_array[id_index] = &region_array[((res+size) - region_array[0].start_address) / ((uint64_t)REGION_SIZE)];
      id_array[id_index]->rdd_id = rdd_id;
      id_array[id_index]->part_id = partition_id;
    }

#if DEBUG_PRINT
    printf("Allocating from region %ld until region %ld\n",(res - region_array[0].start_address) / ((uint64_t)REGION_SIZE),(res+size - region_array[0].start_address) / ((uint64_t)REGION_SIZE));
#endif

    return res;
  }

  if (id_array[id_index]->last_allocated_end + size > id_array[id_index]->start_address + (uint64_t)REGION_SIZE) {

    char* res = new_region(size);
    
    if (res == NULL) {
      perror("[Error] - H2 Allocator is full");
      exit(EXIT_FAILURE);
    }

    /* If object spans more than 1 region we don't want to allocate more objects with it*/
    if (size < (uint64_t) REGION_SIZE){
      id_array[id_index] = &region_array[(res+size - region_array[0].start_address) / (uint64_t)REGION_SIZE];
      id_array[id_index]->rdd_id = rdd_id;
      id_array[id_index]->part_id = partition_id;
    }

    assertf(res != NULL, "No empty region");

#if DEBUG_PRINT
    printf("Allocating from region %ld until region %ld\n",(res - region_array[0].start_address) / (uint64_t)REGION_SIZE,(res+size - region_array[0].start_address) / (uint64_t)REGION_SIZE);
#endif

    return res;
  }

  mark_used(id_array[id_index]->start_address);
  id_array[id_index]->last_allocated_start = id_array[id_index]->last_allocated_end;
  id_array[id_index]->last_allocated_end = id_array[id_index]->last_allocated_start + size;

#if STATISTICS
  gettimeofday(&end_time, NULL);
  alloc_elapsedtime += (end_time.tv_sec - start_time.tv_sec) * 1000.0;
  alloc_elapsedtime += (end_time.tv_usec - start_time.tv_usec) / 1000.0;
#endif

#if DEBUG_PRINT
  printf("Allocating from region %ld until region %ld\n",(id_array[id_index]->last_allocated_start - region_array[0].start_address) / (uint64_t)REGION_SIZE, (id_array[id_index]->last_allocated_start+size - region_array[0].start_address) / (uint64_t)REGION_SIZE);
#endif

  return id_array[id_index]->last_allocated_start;
}


/*
 * function that connects two regions in a group
 * arguments: obj1: the object that references the other
 * obj2 the object that is referenced
 */

void references(char *obj1, char *obj2){
    int32_t seg1 = (obj1 - region_array[0].start_address) / (uint64_t)REGION_SIZE;
    int32_t seg2 = (obj2 - region_array[0].start_address) / (uint64_t)REGION_SIZE;
    if (seg1 >= region_array_size || seg2 >= region_array_size || seg1 < 0 || seg2 < 0)
        return;
  
    if (seg1 == seg2)
        return;
  
    struct group *ptr = region_array[seg1].dependency_list;

    while (ptr != NULL){
        if (ptr->region == &region_array[seg2])
            break;
        ptr = ptr->next;
    }
    if (ptr)
        return;
    struct group *new = malloc(sizeof(struct group));

#if STATISTICS
    total_deps++;
#endif

    new->next = region_array[seg1].dependency_list;
    new->region = &region_array[seg2];
    region_array[seg1].dependency_list = new;
    if (region_array[seg1].used)
        mark_used(region_array[seg2].start_address);
}

/*
 * function that connects two regions in a group
 * arguments: obj: the object that must be checked to be groupped with the region_enabled
 */
void check_for_group(char *obj){
    int32_t seg1 = region_enabled;
    int32_t seg2 = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
    if (seg1 >= region_array_size || seg2 >= region_array_size || seg1 < 0 || seg2 < 0){ 
        return;
    }
    if (seg1 == seg2)
        return;
    struct group *ptr = region_array[seg1].dependency_list;
    while (ptr != NULL){
        if (ptr->region == &region_array[seg2])
            return;
        ptr = ptr->next;
    }

    struct group *new = malloc(sizeof(struct group));
#if STATISTICS
    total_deps++;
#endif
    new->next = region_array[seg1].dependency_list;
    new->region = &region_array[seg2];
    region_array[seg1].dependency_list = new;
    if (region_array[seg1].used)
        mark_used(region_array[seg2].start_address);
}

/*
 * prints all the region groups that contain something
 */
void print_groups(){
    int32_t i;
    fprintf(stderr, "Groups:\n");
    for (i = 0; i < region_array_size; i++){
        if (region_array[i].dependency_list != NULL){
            struct group *ptr = region_array[i].dependency_list;
            fprintf(stderr, "Region %d depends on regions:\n", i);
            while (ptr != NULL){
                fprintf(stderr, "\tRegion %lu\n", ptr->region-region_array);
                ptr = ptr->next;
            }
        }
    }
}

/*
 * Resets the used field of all regions and groups
 */
void reset_used(){
    int32_t i;
    for (i = 0 ; i < region_array_size ; i++)
        region_array[i].used = 0;
}

/*
 * Marks the region that contains this obj as used and increases group
 * counter (if it belongs to a group)
 * Arguments: obj: the object that is alive
 */
void mark_used(char *obj) {
	struct group *ptr = NULL;
    uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;

	assertf(seg >= 0 && seg < region_array_size,
			"Segment index is out of range %lu", seg); 
    if (region_array[seg].used == 1)
        return;

    region_array[seg].used = 1;
    ptr = region_array[seg].dependency_list;

    while (ptr) {
        mark_used(ptr->region->start_address);
        ptr = ptr->next;
    }
}

#if STATISTICS
void print_statistics(){
    uint64_t wasted_space = 0;
    uint64_t total_regions = 0;
    int32_t i;

    for(i = 0 ; i < region_array_size ; i++ ) {
        if (region_array[i % region_array_size].last_allocated_end != region_array[i % region_array_size].start_address ) {
            total_regions++;
            if (region_array[i].last_allocated_end <= region_array[i].start_address + REGION_SIZE){ 
                wasted_space += (region_array[i].start_address + (uint64_t) REGION_SIZE) - region_array[i].last_allocated_end; 
            }
        }
    }
    fprintf(stderr, "Total Wasted Space: %zu MBytes\n", wasted_space / (1024 * 1024));
    fprintf(stderr, "Total regions: %zu\n", total_regions);
    if (total_regions)
        fprintf(stderr, "Average wasted space: %zu KBytes\n", wasted_space / (1024 * total_regions));
    fprintf(stderr, "Total dependencies:%d\n", total_deps);
    fprintf(stderr, "Total time spent in allocate_to_region:%f ms\n", alloc_elapsedtime);
    fprintf(stderr, "Total time spent in free_regions:%f ms\n", free_elapsedtime);
}
#endif

/*
 * Frees all unused regions
 */
struct region_list* free_regions() {
#if STATISTICS
  struct timeval t1,t2;
  gettimeofday(&t1, NULL);
#endif
  int32_t i;
  struct region_list *head = NULL;
  for (i = 0; i < region_array_size; i++){
    if (region_array[i].used == 0 && region_array[i].last_allocated_end != region_array[i].start_address){
      struct group *ptr = region_array[i].dependency_list;
      struct group *next = NULL;
      while (ptr != NULL) {
        next = ptr->next;
        free(ptr);
#if STATISTICS
        total_deps--;
#endif
        ptr = next;
      }
      region_array[i].dependency_list = NULL;

      if (region_array[i].last_allocated_start >= region_array[i].start_address) {
        struct region_list *new_node = malloc(sizeof(struct region_list));
        new_node->start = region_array[i].start_address;
        new_node->end = region_array[i].last_allocated_start;
        new_node->next = head;
        head = new_node;
      }

      region_array[i].last_allocated_end = region_array[i].start_address;
      region_array[i].last_allocated_start = NULL;
      region_array[i].first_allocated_start = NULL;

      if (id_array[get_id(region_array[i].rdd_id, region_array[i].part_id)] == &region_array[i]) {
        id_array[get_id(region_array[i].rdd_id, region_array[i].part_id)] = NULL;
      }
      region_array[i].rdd_id = MAX_PARTITIONS * max_rdd_id;

#if STATISTICS
      fprintf(stderr, "Freeing region %d \n",i);
#endif
    }
  }
#if STATISTICS
  print_statistics();
  gettimeofday(&t2, NULL);
  free_elapsedtime += (t2.tv_sec - t1.tv_sec) * 1000.0;
  free_elapsedtime += (t2.tv_usec - t1.tv_usec) / 1000.0;
#endif
  return head;
}

/*
 * Prints all the allocated regions
 */
void print_regions(){
    int32_t i;
    fprintf(stderr, "Regions:\n");
    for (i = 0; i < region_array_size; i++){
        if (region_array[i].last_allocated_end != region_array[i].start_address)
            fprintf(stderr, "Region %d\n",i);
    }
}

/*
 * Prints all the used regions
 */
void print_used_regions(){
    int32_t i;
    fprintf(stderr, "Used Regions:\n");
    for (i = 0 ; i < region_array_size ; i++){
        if (region_array[i].used == 1)
            fprintf(stderr, "Region %d\n", i);
    }
}

/*
 * Checks if obj is before last object of region
 */
bool is_before_last_object(char *obj){
    uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
	assertf(seg >= 0 && seg < region_array_size,
			"Segment index is out of range %lu", seg); 
    return (obj >= region_array[seg].last_allocated_end) ? false : true;
}

/*
 * Returns last object of region
 */
char* get_last_object(char *obj){
    uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
	assertf(seg >= 0 && seg < region_array_size,
			"Segment index is out of range %lu", seg); 
    return region_array[seg].last_allocated_end;
}

// Returns true if object is first of its region false otherwise
bool is_region_start(char *obj){
    uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
	assertf(seg >= 0 && seg < region_array_size,
			"Segment index is out of range %lu", seg); 

	return (region_array[seg].first_allocated_start == obj) ? true : false;
}

/*
 * Enables groupping with the region in which obj belongs to
 */
void enable_region_groups(char *obj){
  region_enabled = ((uint64_t)(obj - region_array[0].start_address)) / (uint64_t) REGION_SIZE;
  assertf(region_enabled >= 0 && region_enabled < INT32_MAX, "Sanity check for overflow");
}

/*
 * Disables groupping with the region previously enabled
 */
void disable_region_groups(void){
  region_enabled = region_array_size;
  assertf(region_enabled >= 0 && region_enabled < INT32_MAX, "Sanity check for overflow");
}


void print_objects_temporary_function(char *obj,const char *string){
    printf("Object name: %s\n",string);
}

/*
 * Start iteration over all active regions to print their object state.
 */
void start_iterate_regions() {
	_next_region = 0;
}

/*
 * Return the next active region or NULL if we reached the end of the region
 * array.
 */
char* get_next_region() {
  char *region_start_addr;

  // Find the next active region
  while (_next_region < region_array_size &&
    (region_array[_next_region].used == 0 || region_array[_next_region].first_allocated_start != region_array[_next_region].start_address)) {
      _next_region++; 
  }

  if (_next_region >= region_array_size)
    return NULL;

  fprintf(stderr, "[PLACEMENT] Region: %d\n", _next_region);

  region_start_addr = region_array[_next_region].start_address;
  _next_region++;

  return region_start_addr;
}

char *get_first_object(char *addr) {
  uint64_t seg = (addr - region_array[0].start_address) / (uint64_t)REGION_SIZE;
  assertf(seg >= 0 && seg < region_array_size, "Segment index is out of range %lu", seg); 
  return region_array[seg].first_allocated_start;
}

int get_num_of_continuous_regions(char *addr){
  uint64_t seg = (addr - region_array[0].start_address) / (uint64_t)REGION_SIZE;
  #ifdef ASSERT
  static const char *border = "------------------------------------------------------------";
  //fprintf(stderr, "%s\n[%s|%s|%d]\nsegment = %" PRIu64 "\naddress = %p\n region_array[0].start_address = %p\n%s\n", border, __FILE__, __func__, __LINE__, seg, addr, region_array[0].start_address, border);
  assertf(addr >= region_array[0].start_address, "%s\n[%s|%s|%d]\nsegment = %" PRIu64 "\naddress = %p\n region_array[0].start_address = %p\n%s\n", border, __FILE__, __func__, __LINE__, seg, addr, region_array[0].start_address, border);
  #endif

  if (region_array[seg].last_allocated_end == region_array[seg].start_address)
    return 0;

  return (region_array[seg].last_allocated_end - region_array[seg].first_allocated_start) % (uint64_t)REGION_SIZE != 0 ? 
  (region_array[seg].last_allocated_end - region_array[seg].first_allocated_start) / (uint64_t)REGION_SIZE + 1 :
  (region_array[seg].last_allocated_end - region_array[seg].first_allocated_start) / (uint64_t)REGION_SIZE; 
}

/*
 * Get objects 'obj' region start address
 */
char* get_region_start_addr(char *obj, uint64_t rdd_id, uint64_t part_id) {
	uint64_t index = get_id(rdd_id, part_id);

	return id_array[index]->start_address;
}

/*
 * Get object 'groupId' (RDD Id). Each object is allocated based on a group Id
 * and the partition Id that locates in teraflag.
 *
 * @obj: address of the object
 *
 * Return: the object rdd id
 */
uint64_t get_obj_group_id(char *obj) {
  uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
  assertf(seg >= 0 && seg < region_array_size,
          "Segment index is out of range %lu", seg); 
  return region_array[seg].rdd_id;
}

/*
 * Get object 'groupId'. Each object is allocated based on a group Id
 * and the partition Id that locates in teraflag.
 *
 * obj: address of the object
 *
 * returns: the object partition Id
 */
uint64_t get_obj_part_id(char *obj) {
  uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
  assertf(seg >= 0 && seg < region_array_size,
          "Segment index %" PRIu64 " is out of range\n%-30s = %p\n%-30s = %p\nregion_array_size = %" PRIu64 "\n", seg, "object address", obj, "region_array[0].start_address", region_array[0].start_address, region_array_size); 

  return region_array[seg].part_id;
}

/*
 * Check if these two objects belong to the same group
 *
 * obj1: address of the object
 * obj2: address of the object
 *
 * returns: 1 if objects are in the same group, 0 otherwise
 */
int is_in_the_same_group(char *obj1, char *obj2) {
	uint64_t seg1 = (obj1 - region_array[0].start_address) / (uint64_t)REGION_SIZE;
	uint64_t seg2 = (obj2 - region_array[0].start_address) / (uint64_t)REGION_SIZE;
	struct group *ptr = NULL;

	assertf(seg1 < region_array_size && seg2 < region_array_size && seg1 >= 0
			&& seg2 >=0, "Segment index is out of range %lu, %lu", seg1, seg2); 

	/* Objects belong to the same group */
	if (seg1 == seg2)
		return 1;

	for (ptr = region_array[seg1].dependency_list; ptr != NULL; ptr = ptr->next) {
		if (ptr->region == &region_array[seg2])
			return 1;
	}

	return 0;
}

/*                                                                              
 * Get the total allocated regions                                              
 * Return the total number of allocated regions or zero, otherwise              
 */                                                                             
long total_allocated_regions() {                                                
	int32_t i;                                                                      
	long counter = 0;                                                           

	for (i = 0; i < region_array_size; i++){                                   
		if (region_array[i].last_allocated_end != region_array[i].start_address)
			counter++;                                                          
	}                                                                           

	return counter;                                                             
}

/*                                                                              
 * Get the total number of used regions                                         
 * Return the total number of used regions or zero, otherwise                   
 */                                                                             
long total_used_regions() {                                                     
	int32_t i;                                                                      
	long counter = 0;                                                           

	for (i = 0 ; i < region_array_size; i++) {                                 
		if (region_array[i].used == 1)                                          
			counter++;                                                          
	}                                                                           
	return counter;                                                             
}

#if PR_BUFFER

/*
 * Flush the promotion buffer of a certain region
 *
 * seg: Index of the region in region array
 *
 */
void flush_buffer(uint64_t seg) {
	struct pr_buffer *buf = region_array[seg].pr_buffer;

	if (buf->size == 0)
		return;

	assertf(buf->size <= PR_BUFFER_SIZE, "Sanity check");

	// Write the buffer to TeraCache
	r_awrite(buf->buffer, buf->first_obj_addr, buf->size / HeapWordSize);

	buf->alloc_ptr = buf->buffer;
	buf->first_obj_addr = NULL;
	buf->size = 0;
}

/*
 * Add an obect to the promotion buffer. We use promotion buffer to avoid write
 * system calls for small sized objects.
 *
 * obj: Object that will be writter in the promotion buffer
 * new_adr: Is used to know where the first object in the promotion buffer will
 *			be move to H2
 * size: Size of the object
 */
void buffer_insert(char* obj, char* new_adr, size_t size) {
	uint64_t seg = (new_adr - region_array[0].start_address) / (uint64_t)REGION_SIZE;
#if 1//perpap 
	if(mtx_lock(&region_array[seg].mutex) == thrd_error){
            #if DEBUG_PRINT
	    fprintf(allocator_log_fp, "[%s|%s|%d]Mutex lock failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
            #endif
	    exit(EXIT_FAILURE);
	} 
	//assertf(mtx_lock(&region_array[seg].mutex) == thrd_error, "Mutex lock failed! error:%d", thrd_error);	
#endif
	struct pr_buffer *buf = region_array[seg].pr_buffer;

	char*  start_adr  = buf->first_obj_addr;
	size_t cur_size   = buf->size;
	size_t free_space = PR_BUFFER_SIZE - cur_size;

	assertf(THRESHOLD < PR_BUFFER_SIZE, "Threshold should be less that promotion buffer size");

	if ((size * HeapWordSize) > THRESHOLD) {
		r_awrite(obj, new_adr, size);
#if 1//perpap
                //assertf(mtx_unlock(&region_array[seg].mutex) == thrd_error, "Mutex unlock failed! error:%d", thrd_error);
		if(mtx_unlock(&region_array[seg].mutex) == thrd_error){
                    #if DEBUG_PRINT
	            fprintf(allocator_log_fp, "[%s|%s|%d]Mutex unlock failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
                    #endif
	            exit(EXIT_FAILURE);
	        }
#endif
		return;
	}

	/* Allocate a buffer for the region and set buffer allocation ptr */
	if (buf->buffer == NULL) {
		buf->buffer = malloc(PR_BUFFER_SIZE * sizeof(char));
		buf->alloc_ptr = buf->buffer;
	}

	/* Case1: Buffer is empty */
	if (cur_size == 0) {
		assertf(start_adr == NULL, "Sanity check");

		memcpy(buf->alloc_ptr, obj, size * HeapWordSize);

		buf->first_obj_addr = new_adr;
		buf->alloc_ptr += size * HeapWordSize;
		buf->size = size * HeapWordSize;
#if 1//perpap
                //assertf(mtx_unlock(&region_array[seg].mutex) == thrd_error, "Mutex unlock failed! error:%d", thrd_error);
		if(mtx_unlock(&region_array[seg].mutex) == thrd_error){
                    #if DEBUG_PRINT
	            fprintf(allocator_log_fp, "[%s|%s|%d]Mutex unlock failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
                    #endif
	            exit(EXIT_FAILURE);
	        }
#endif
		return;
	}
	
	/* 
	 * Case2: Object size is grater than the available free space in buffer
	 * Case3: Object's new address is not contignious with the other objects - 
	 * object belongs to next addresses in the region
	 * In both cases we flush the buffer and allocate the object in a new
	 * buffer.
	 */
	if (((size * HeapWordSize) > free_space) || ((start_adr + cur_size) != new_adr)) {
		flush_buffer(seg);
		
		memcpy(buf->alloc_ptr, obj, size * HeapWordSize);

		buf->first_obj_addr = new_adr;
		buf->alloc_ptr += size * HeapWordSize;
		buf->size = size * HeapWordSize;
#if 1//perpap
                //assertf(mtx_unlock(&region_array[seg].mutex) == thrd_error, "Mutex unlock failed! error:%d", thrd_error);//perpap
		if(mtx_unlock(&region_array[seg].mutex) == thrd_error){
                    #if DEBUG_PRINT
	            fprintf(allocator_log_fp, "[%s|%s|%d]Mutex unlock failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
                    #endif
	            exit(EXIT_FAILURE);
	        }
#endif
		return;
	}
	
	memcpy(buf->alloc_ptr, obj, size * HeapWordSize);

	buf->alloc_ptr += size * HeapWordSize;
	buf->size += size * HeapWordSize;
#if 1//perpap
        //assertf(mtx_unlock(&region_array[seg].mutex) == thrd_error, "Mutex unlock failed! error:%d", thrd_error);	
	if(mtx_unlock(&region_array[seg].mutex) == thrd_error){
            #if DEBUG_PRINT
	    fprintf(allocator_log_fp, "[%s|%s|%d]Mutex unlock failed! error:%d\n", __FILE__, __func__, __LINE__, thrd_error);
            #endif
	    exit(EXIT_FAILURE);
	}
#endif
}

/*
 * Flush all active buffers and free each buffer memory. We need to free their
 * memory to limit waste space.
 */
void free_all_buffers() {
	//uint64_t i;
	struct pr_buffer *buf;

  for (uint64_t i = 0; i < region_array_size; i++) {
    buf = region_array[i].pr_buffer;
    #ifdef ASSERT
    assertf(buf != NULL, "pr_buffer is NULL! buffer_index = %" PRIu64 " region_array_size = %" PRIu64 "\n", i, region_array_size);
    #endif
    /* Buffer is not empty, so flush it*/
    if (buf->size != 0)
      flush_buffer(i);

    /* If the buffer is already flushed, just free bufffer's memory */
    if (buf->buffer != NULL) {
      free(buf->buffer);
      buf->buffer = NULL;
      buf->alloc_ptr = NULL;
      buf->first_obj_addr = NULL;
      buf->size = 0;
    }
  }
}

bool object_starts_from_region(char *obj) {
  uint64_t seg = (obj - region_array[0].start_address) / (uint64_t)REGION_SIZE;
  assertf(seg >= 0 && seg < region_array_size,
          "Segment index %" PRIu64 " is out of range!\n%-30s = %p\n%-30s = %p\nregion_array_size = %" PRIu64 "\n", seg, "object address", obj, "region_array[0].start_address", region_array[0].start_address, region_array_size); 
  return (region_array[seg].first_allocated_start != region_array[seg].start_address) ? false : true;
}
#endif
