/***************************************************
*
* file: tc_async_write.c
*
* @Author:   Iacovos G. Kolokasis
* @Author:   Giannos Evdorou
* @Version:  29-11-2021
* @email:    kolokasis@ics.forth.gr
*
* Test to verify:
*       - allocator initialization
*       - object allocation in the correct positions
*       - synchronous write with explicit IO
***************************************************/
#include "tc_heap.h"

#define SIZE_80B (80)
#define SIZE_160B (160)
#define SIZE_1M (1*1024LU*1024)
#define SIZE_4M (4*1024LU*1024)

#define HEAPWORD (8)

#define SIZE_TO_WORD(SIZE) ((size_t) (SIZE / HEAPWORD))

int main(int argc, char **argv) {
  if(argc != 4){
    fprintf(stderr,"Usage: ./tc_allocate.bin <mount_point> <h1_size> <h2_size>\n");
    exit(EXIT_FAILURE);
  }
  unsigned long long h1_size = convert_string_to_number(argv[2]);
  ERRNO_CHECK
  unsigned long long h2_size = convert_string_to_number(argv[3]);
  ERRNO_CHECK

	char *obj1, *obj2, *obj3, *obj4;
	char *tmp, *tmp2, *tmp3, *tmp4;
	
	initialize_h1(H1_ALIGNMENT, NULL, h1_size * GB, 0);
  initialize_h2(16, H2_ALIGNMENT, argv[1], h2_size * GB, (void *)(h1.start_address + h1_size * GB));
  print_heap_statistics();

	tmp = malloc(SIZE_80B * sizeof(char));
	memset(tmp, '1', SIZE_80B);
	tmp[SIZE_80B - 1] = '\0';

	tmp2 = malloc(SIZE_160B * sizeof(char));
	memset(tmp2, '2', SIZE_160B);
	tmp2[SIZE_160B - 1] = '\0';

	tmp3 = malloc(SIZE_1M * sizeof(char));
	memset(tmp3, '3', SIZE_1M);
	tmp3[SIZE_1M - 1] = '\0';

	tmp4 = malloc(SIZE_4M * sizeof(char));
	memset(tmp4, '4', SIZE_4M);
	tmp4[SIZE_4M - 1] = '\0';
	
	obj1 = allocate(SIZE_TO_WORD(SIZE_80B), 0, 0);
	r_awrite(tmp, obj1, SIZE_TO_WORD(SIZE_80B), 0);
	
	obj2 = allocate(SIZE_TO_WORD(SIZE_160B), 1, 0);
	r_awrite(tmp2, obj2, SIZE_TO_WORD(SIZE_160B), 0);
	
	obj3 = allocate(SIZE_TO_WORD(SIZE_1M), 0, 0);
	r_awrite(tmp3, obj3, SIZE_TO_WORD(SIZE_1M), 0);
	
	obj4 = allocate(SIZE_TO_WORD(SIZE_4M), 1, 0);
	r_awrite(tmp4, obj4, SIZE_TO_WORD(SIZE_4M), 0);

	while (!r_areq_completed());

	assertf(strlen(obj1) == SIZE_80B - 1, "Error in size %lu", strlen(obj1));
	assertf(strlen(obj2) == SIZE_160B - 1, "Error in size %lu", strlen(obj2));
	assertf(strlen(obj3) == SIZE_1M - 1, "Error in size %lu", strlen(obj3));
	assertf(strlen(obj4) == SIZE_4M - 1, "Error in size %lu", strlen(obj4));
	
	printf("--------------------------------------\n");
	printf("TC_Async_Write:\t\t\t\033[1;32m[PASS]\033[0m\n");
	printf("--------------------------------------\n");
	
	return 0;
}
