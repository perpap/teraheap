#ifndef SHARE_GC_TERAHEAP_TERAHEAP_HPP
#define SHARE_GC_TERAHEAP_TERAHEAP_HPP

#include "gc/parallel/objectStartArray.hpp"
#include "gc/shared/collectedHeap.inline.hpp"
#include "gc/teraHeap/teraDynamicResizingPolicy.hpp"
#include "gc/teraHeap/teraTimers.hpp"
#include "gc/teraHeap/teraTraceDirtyPages.hpp"
#include "gc/teraHeap/teraTransferPolicy.hpp"
#include "gc/teraHeap/teraStatistics.hpp"
#include "utilities/stack.inline.hpp"
#include "memory/sharedDefines.h"
#include "oops/oop.hpp"

#include <regions.h>

#ifdef BACK_REF_STAT
#include <map>
#include <tr1/tuple>
#endif

class TeraHeap: public CHeapObj<mtInternal> {
private:
  static char *_start_mmap;         // TeraHeap unaligned start address of mmap region
  static char *_start_addr;         // TeraHeap start address of mmap region
  static char *_stop_addr;          // TeraHeap ends address of mmap region

  ObjectStartArray _start_array;    // Keeps track of where objects
                                    // start in a 2^CARD_SEGMENT_SIZE block

  /*-----------------------------------------------
   * Stacks
   *---------------------------------------------*/
  // Stack to keep back pointers (Objects that are pointed out of
  // TeraHeap objects) to mark them as alive durin mark_and_push phase of
  // the Full GC.
  static Stack<oop *, mtGC> _tc_stack;

  // Stack to keep the element addresses of objects that are located in
  // TeraHeap and point to objects in the heap. We adjust these pointers
  // during adjust phase of the Full GC.
  static Stack<oop *, mtGC> _tc_adjust_stack;

#ifdef TERA_TIMERS
  TeraTimers *teraTimer;
#endif

#ifdef TERA_STATS
  TeraStatistics *tera_stats;
#endif

  static long int cur_obj_group_id; //<We save the current object
                                    // group id for tera-marked
                                    // object to promote this id
                                    // to their reference objects
  static long int cur_obj_part_id;  //<We save the current object
                                    // partition id for tera-marked
                                    // object to promote this id
                                    // to their reference objects

  HeapWord *obj_h1_addr;            // We need to check this
                                    // object that will be moved
                                    // to H2 if it has back ptrs
                                    // to H1

  HeapWord *obj_h2_addr;            // We need to check this
                                    // object that will be moved
                                    // to H2 if it has back ptrs
                                    // to H1

  TransferPolicy *tera_policy;      //< Transfer policy for H2

  bool shrink_h1 = false;           //< This flag indicate that H1
                                    // should be shrinked
  bool grow_h1 = false;             //< This flag indicate that H1
                                    // should be grown
  
  TeraDynamicResizingPolicy* dynamic_resizing_policy; 
  TeraTraceDirtyPages* trace_dirty_pages;

#ifdef BACK_REF_STAT
  // This histogram keeps internally statistics for the backward
  // references (H2 to H1)
  std::map<oop *, std::tr1::tuple<int, int, int> > histogram;
  oop *back_ref_obj;
#endif

#ifdef FWD_REF_STAT
  // This histogram keeps internally statistics for the forward references
  // (H1 to H2) per object
  std::map<oop, int> fwd_ref_histo;
  
  // Print the histogram
  void h2_print_fwd_ref_stat();
#endif

  void h2_count_marked_objects();

  void h2_reset_marked_objects();

  // Create a transfer policy for moving ojects from H1 to H2
  TransferPolicy* create_transfer_policy();

  // Explicit (using systemcall) write 'data' with 'size' to the specific
  // 'offset' in the file.
  void h2_write(char *data, char *offset, size_t size);

  // Explicit (using systemcall) asynchronous write 'data' with 'size' to
  // the specific 'offset' in the file.
  void h2_awrite(char *data, char *offset, size_t size);

  // We need to ensure that all the writes in TeraHeap using asynchronous
  // I/O have been completed succesfully.
  int h2_areq_completed();

  // Fsync writes in TeraHeap
  // We need to make an fsync when we use fastmap
  void h2_fsync();

#ifdef PR_BUFFER
  // Add an object 'obj' with size 'size' to the promotion buffer. 'New_adr' is
  // used to know where the object will move to H2. We use promotion buffer to
  // reduce the number of system calls for small sized objects.
  void h2_promotion_buffer_insert(char *obj, char *new_adr, size_t size);

  // At the end of the major GC flush and free all the promotion
  // buffers.
  void h2_free_promotion_buffers();
#endif

public:
  // Default Constructor with hint for H2 placement after H1 address range
  TeraHeap(HeapWord* heap_end = nullptr);
  // Destructor
  ~TeraHeap();

  // Get object start array for h2
  ObjectStartArray *h2_start_array() { return &_start_array; }
 
  // Return H2 unaligned start address
  char *h2_start_mmap_addr(void);
  // Return H2 start address
  char *h2_start_addr(void);

  // Return H2 stop address
  char *h2_end_addr(void);
  
  // Get the top allocated address of the H2. This address depicts the
  // end address of the last allocated object in the last region of
  // H2.
  char *h2_top_addr(void);
  
  // Check if H2 is empty.
  // Return true if H2 is empty, false otherwise
  bool h2_is_empty(void);
  
  // Check if an object `ptr` belongs to the TeraHeap. If the object belongs
  // then the function returns true, otherwise it returns false.
  bool is_in_h2(const void* p);
  
  // Check if an object `ptr` belongs to the TeraHeap. If the object belongs
  // then the function returns true, otherwise it returns false.
  bool is_obj_in_h2(oop ptr);
  
  // Check if reference `p` which depicts the field of the object
  // belongs to TeraHeap. If the object belongs then the function
  // returns true, otherwise it returns false.
  bool is_in_h2(HeapWord *p);

  // Check if reference `p` which depicts the field of the object
  // belongs to TeraHeap. If the object belongs then the function
  // returns true, otherwise it returns false.
  bool is_field_in_h2(void *p);
  
  // Deallocate the backward references stacks
  void h2_clear_back_ref_stacks();
  
  // Give advise to kernel to expect page references in sequential order
  void h2_enable_seq_faults();

  // Give advise to kernel to expect page references in random order
  void h2_enable_rand_faults();
  
  // Check if the first object `obj` in the H2 region is valid. If not
  // that depicts that the region is empty
  bool check_if_valid_object(HeapWord *obj);

  // Get the ending address of the last object of the region obj
  // belongs to.
  HeapWord *get_last_object_end(HeapWord *obj);

  // Checks if the address of obj is the beginning of a region
  bool is_start_of_region(HeapWord *obj);
  
  // Retrurn the start address of the first object of the secific region
  HeapWord *get_first_object_in_region(HeapWord *addr);

  // Add new object in the region
  char *h2_add_object(oop obj, size_t size);

  // Pop the objects that are in `_tc_stack`. These objects are
  // located in the Java Heap and we need to ensure that they will be
  // kept alive.
  oop* h2_get_next_back_reference();

  // Update backward reference stacks that we use in marking and
  // pointer adjustment phases of major GC.
  void h2_push_backward_reference(void *p, oop o);

  // Get the next backward reference from the stack to adjust
  oop* h2_adjust_next_back_reference();

  // Resets the used field of all regions
  void h2_reset_used_field(void);

  // Marks the region containing obj as used
  void mark_used_region(HeapWord *obj);

  // Prints all active regions
  void print_h2_active_regions(void);

  // Groups the region of obj with the previously enabled region
  void group_region_enabled(HeapWord *obj, void *obj_field);

  // Frees all unused regions
  void free_unused_regions(void);

  // Prints all the region groups
  void print_region_groups(void);

  // Check if the collector transfers and adjust H2 candidate objects.
  bool compact_h2_candidate_obj_enabled();

  // Enables groupping with region of obj
  void enable_groups(HeapWord *old_addr, HeapWord *new_addr);

  // Disables region groupping
  void disable_groups(void);

  //void print_object_name(HeapWord *obj, const char *name);

  // Add a new entry to `obj1` region dependency list that reference
  // `obj2` region
  void group_regions(HeapWord *obj1, HeapWord *obj2);

  // Iterate over all objects in each region and print their states
  // This function is for debugging purposes to understand and fix the
  // locality in regions
  void h2_print_objects_per_region(void);

  void mark_live(HeapWord *p);

  void h2_mark_live_objects_per_region();

  // Check if backward adjust stack is empty
  bool h2_is_empty_back_ref_stacks();

  // Get the group Id of the objects that belongs to this region. We
  // locate the objects of the same group to the same region. We use the
  // field 'p' of the object to identify in which region the object
  // belongs to.
  uint64_t h2_get_region_groupId(void *p);

  // Get the partition Id of the objects that belongs to this region. We
  // locate the objects of the same group to the same region. We use the
  // field 'p' of the object to identify in which region the object
  // belongs to.
  uint64_t h2_get_region_partId(void *p);

#ifdef BACK_REF_STAT
  // Add a new entry to the histogram for back reference that start from
  // 'obj' and results in H1 (new or old generation).
  // Use this function with a single GC thread
  void h2_update_back_ref_stats(bool is_old, bool is_tera_cache);

  void h2_enable_back_ref_traversal(oop *obj);

  // Print the histogram
  void h2_print_back_ref_stats();
#endif

#ifdef FWD_REF_STAT
  // Add a new entry to the histogram for forward reference that start from
  // H1 and results in 'obj' in H2
  void h2_add_fwd_ref_stat(oop obj);
#endif

  TransferPolicy* get_policy() { return tera_policy; }

  // Check if the object `obj` is an instance of the following
  // metadata class:
  // - Instance Mirror Klass
  // - Instance Reference Klass
  // - Instance Class Loader Klass
  // If yes return true, otherwise false
  bool is_metadata(oop obj);

  // Check if the object with `addr` span multiple regions
  int h2_continuous_regions(HeapWord *addr);

  // Check where the object starts
  bool h2_object_starts_in_region(HeapWord *obj);

  // Move object with size 'size' from source address 'src' to the h2
  // destination address 'dst' 
  void h2_move_obj(HeapWord *src, HeapWord *dst, size_t size);

  // Complete the transfer of the objects in H2
  void h2_complete_transfers();

  // Check if the group of regions in H2 is enabled
  bool is_h2_group_enabled();

#ifdef TERA_TIMERS
  // Tera timers maintains timers for the different phases of the
  // major GC
  TeraTimers* getTeraTimer();
#endif

#ifdef TERA_STATS
  // Tera statistics for objects that we move to H2, forward references,
  // and backward references.
  TeraStatistics* get_tera_stats();
#endif

  // Init the allocator to reserve dram space for the allocation of
  // forwarding tables 
  void init_tera_dram_allocator(uint64_t entries);
  
  // Request an address from the allocator. Sizes should be up to 24bytes.
  // Eeach entry in the forwarding table is 24bytes
  inline char* tera_dram_malloc(size_t size);

  // Destroy the reserved dram space
  void destroy_tera_dram_allocator();

  // TODO: The following 6 functions should move to
  // DynamicResizingPolicy class
  // The state machine uses this function to set if the GC should
  // shrink H1 as a result to free the physical pages. Then the OS
  // will reclaim the physical pahges and will use them as part of the
  // buffer cache.
  void set_shrink_h1() { shrink_h1 = true; }
  // Reinitalize the shrink_h1 flag
  void unset_shrink_h1() { shrink_h1 = false; }
  // Check if the state machine identifies that we need to shrink H1.
  bool  need_to_shink_h1() { return shrink_h1; }
  
  // The state machine uses this function to set if the GC should
  // grow H1.
  void set_grow_h1() { grow_h1 = true; }
  // Reinitalize the grow_h1 flag to the default value
  void unset_grow_h1() { grow_h1 = false; }
  // Check if the state machine identifies that we need to grow H1.
  bool  need_to_grow_h1() { return grow_h1; }

  TeraDynamicResizingPolicy* get_resizing_policy() {
    return dynamic_resizing_policy;
  }
  
  void trace_dirty_h2_pages(void) {
    trace_dirty_pages->find_dirty_pages();
  }

  // Check if the address is already reserved and mapped
  bool check_if_address_is_mapped(pid_t pid, uintptr_t h1_end_addr);
  
  // Check if the address is already reserved and mapped
  bool is_address_aligned(uintptr_t p, uintptr_t alignment);

  #if 0//perpap
  bool is_aligned(char *p, uintptr_t N);
  uint64_t available_virtual_space(void *address1, void *address2);
  void check_address(pid_t pid, uintptr_t address);
  #endif
};

#endif
