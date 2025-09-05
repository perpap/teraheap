#include <pthread.h>
#include "tc_heap.h"

#define SIZE_1MB (131072)
#define THREADS 16

volatile char **objects = NULL;

void *multi_alloc(void *arg) {
  uint64_t thread_id = *((uint64_t *) arg);

  for (int i = 0; i < 10; i++) {
    if ((0 < thread_id && thread_id < 4) || (8 < thread_id && thread_id < 12)) 
      objects[thread_id * 10 + i] = allocate(60 * SIZE_1MB, 0, 0, thread_id);
    else
      objects[thread_id * 10 + i] = allocate(40 * SIZE_1MB, thread_id+1, 0, thread_id);

    fprintf(stderr, "[t#%lu][%lu] Allocate: %p\n", thread_id, thread_id * 10 + i, objects[thread_id * 10 + i]);
  }

  return NULL;
}

int main(int argc, char **argv) {
  if(argc != 4){
    fprintf(stderr,"Usage: ./tc_mt_extend_regions.bin <mount_point> <h1_size> <h2_size>\n");
    exit(EXIT_FAILURE);
  }
  unsigned long long h1_size = convert_string_to_number(argv[2]);
  ERRNO_CHECK
  unsigned long long h2_size = convert_string_to_number(argv[3]);
  ERRNO_CHECK

  initialize_h1(H1_ALIGNMENT, NULL, h1_size * GB, 0);
  initialize_h2(16, H2_ALIGNMENT, argv[1], h2_size * GB, (void *)(h1.start_address + h1_size * GB));
  print_heap_statistics();

  objects = malloc(10 * THREADS * sizeof(char *));
  pthread_t threads[THREADS]; 
  uint64_t ids[THREADS];

  for(int i = 0; i < THREADS; i++) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, multi_alloc, &ids[i]);
  }

  for(int i = 0; i < THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  for (int i = 0; i < 10 * THREADS - 1; i++) {
    for (int j = i + 1; j < 10 * THREADS; j++){
      if (objects[i] == objects[j]) {
        printf("TC_MT_Extend_Regions: Double Allocation [%d] == [%d] == %p\t\t\t\033[1;31m[FAIL]\033[0m\n", i, j, objects[i]);
        free(objects);

        return 1;
      }
    }
  }

  update_top();

  char *calculated_top = NULL;
  int top_obj_size = 0;

  for (int i = 0; i < 10 * THREADS; i++) {
    if (objects[i] > calculated_top) {
      calculated_top = (char *) objects[i];

      int thread_offset = i / 10;
      if ((0 < thread_offset && thread_offset < 4) || (8 < thread_offset && thread_offset < 12)) {
        top_obj_size = 60;
      } else {
        top_obj_size = 40;
      }
    }
  }


  if (cur_alloc_ptr() != calculated_top + 8 * top_obj_size * SIZE_1MB) {
    printf("TC_MT_Extend_Regions: Top address incorrect [%p] == [%p]\t\t\t\033[1;31m[FAIL]\033[0m\n", cur_alloc_ptr(), calculated_top + 8 * top_obj_size * SIZE_1MB);
    free(objects);
    return 1;
  }

  print_heap_statistics();
  print_regions();

  printf("--------------------------------------\n");
  printf("TC_MT_Extend_Regions:\t\t\t\033[1;32m[PASS]\033[0m\n");
  printf("--------------------------------------\n");

  free(objects);

  return 0;
}
