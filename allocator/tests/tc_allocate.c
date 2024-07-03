
/***************************************************
*
* file: tc_allocate.c
*
* @Author:   Iacovos G. Kolokasis
* @Author:   Giannos Evdorou
* @Version:  09-03-2021
* @email:    kolokasis@ics.forth.gr
*
* Test to verify:
*       - allocator initialization
*       - object allocation in the correct positions
***************************************************/
#include "tc_heap.h"

//this test needs 256MB region size
int main(int argc, char **argv) {
    if(argc != 4){
      fprintf(stderr,"Usage: ./tc_allocate.bin <mount_point> <h1_size> <h2_size>\n");
      exit(EXIT_FAILURE);
    }
    unsigned long long h1_size = convert_string_to_number(argv[2]);
    ERRNO_CHECK
    unsigned long long h2_size = convert_string_to_number(argv[3]);
    ERRNO_CHECK
    
    char *obj1, *obj2, *obj3, *obj4, *obj5, *obj6, *obj7, *obj8, *obj9;
     // Memory-map heap 1 (h1)
    initialize_h1(H1_ALIGNMENT, NULL, h1_size * GB, 0);
#if 0//perpap
    //void* h1 = aligned_mmap(H1_SIZE, H1_ALIGNMENT, NULL, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (h1 == NULL) {
        fprintf(stderr, "Failed to allocate h1\n");
        return EXIT_FAILURE;
    }
    printf("h1 mapped at: (HEX)%p (DEC)%zd\n", h1, (uintptr_t)h1);

    // Calculate the end address of h1
    uintptr_t h1_end = (uintptr_t)h1 + H1_SIZE;
    printf("h1 is aligned? %d\n", is_aligned(h1, H1_ALIGNMENT));
    //printf("h1_end is aligned? %d\n", is_aligned((void *)h1_end, H2_ALIGNMENT));
    printf("h1_end is aligned? %d\n", is_aligned(align_ptr_up((void *)h1_end, H2_ALIGNMENT), H2_ALIGNMENT));

    // Init allocator
    //init(CARD_SIZE * PAGE_SIZE, "/mnt/fmap/", 161061273600);
    init(H2_ALIGNMENT, "/spare2/perpap/fmap/", H2_SIZE, (void *)h1_end);
    printf("h2 mapped at: (HEX)%p (DEC)%zd\n", start_addr_mem_pool(), (uintptr_t)start_addr_mem_pool);
    printf("%-20s %-20s %-20s %-20s %-20s %-20s\n", "", "START_ADDRESS(HEX)", "START_ADDRESS(DEC)", "END_ADDRESS(HEX)", "END_ADDRESS(DEC)", "SIZE(GB)");
    printf("%-20s %-20p %-20llu %-20p %-20llu %-20td\n", "H1", h1, (long long unsigned int)h1, (void *)h1_end, (long long unsigned int)h1_end,CONVERT_TO_GB((ptrdiff_t)((char *)h1_end-(char*)h1)));
    printf("%-20s %-20p %-20llu %-20p %-20llu %-20td\n", "H2", start_addr_mem_pool(), (long long unsigned int)start_addr_mem_pool(), stop_addr_mem_pool(), (long long unsigned int)stop_addr_mem_pool(),(ptrdiff_t)CONVERT_TO_GB((stop_addr_mem_pool()-start_addr_mem_pool())));
#endif
    initialize_h2(H2_ALIGNMENT, argv[1], h2_size * GB, (void *)(h1.start_address + h1_size * GB));
    print_heap_statistics();
    //obj1 should be in region 0
    obj1 = allocate(1, 0, 0);
    fprintf(stderr, "Allocate: %p\n", obj1);
    assertf((obj1 - start_addr_mem_pool()) == 0, "Object start position");

    //obj2 should be in region 1 
    obj2 = allocate(200, 1, 0);
    fprintf(stderr, "Allocate: %p\n", obj2);
    assertf((obj2 - obj1)/8 == 33554432, "Object start position %zu", (obj2 - obj1) / 8); 

    //obj3 should be in region 0
    obj3 = allocate(12020, 0, 0);
    fprintf(stderr, "Allocate: %p\n", obj3);
    assertf((obj3 - obj1)/8 == 1, "Object start position");

    //obj4 should be in region 2 
    obj4 = allocate(262140, 2, 0);
    fprintf(stderr, "Allocate: %p\n", obj4);
    assertf((obj4 - obj2)/8 == 33554432, "Object start position %zu", (obj4 - obj2) / 8);

    //obj5 should be in region 1
    obj5 = allocate(4, 1, 0);
    fprintf(stderr, "Allocate: %p\n", obj5);
    assertf((obj5 - obj2)/8 == 200, "Object start position");

    //obj6 should be in region 0 
    obj6 = allocate(200, 0, 0);
    fprintf(stderr, "Allocate: %p\n", obj6);
    assertf((obj6 - obj3)/8 == 12020, "Object start position");

    //obj7 should be in region 3 
    obj7 = allocate(262140, 1, 0);
    fprintf(stderr, "Allocate: %p\n", obj7);
    assertf((obj7 - obj5)/8 == 4, "Object start position %zu", (obj7 - obj5) / 8);

    //obj8 should be in region 4 
    obj8 = allocate(500, 3, 0);
    fprintf(stderr, "Allocate: %p\n", obj8);
    assertf((obj8 - obj4)/8 == 33554432, "Object start position");

    //obj9 should be in region 5 
    obj9 = allocate(500, 2, 0);
    fprintf(stderr, "Allocate: %p\n", obj9);
    assertf((obj9 - obj4)/8 == 262140, "Object start position");
	
	printf("--------------------------------------\n");
	printf("TC_Allocate:\t\t\t\033[1;32m[PASS]\033[0m\n");
	printf("--------------------------------------\n");

	return 0;
}
