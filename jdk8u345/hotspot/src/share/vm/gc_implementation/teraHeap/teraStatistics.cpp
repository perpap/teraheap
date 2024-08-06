#include "gc_implementation/teraHeap/teraStatistics.hpp"

TeraStatistics::TeraStatistics() {
  total_objects = 0;
  total_objects_size = 0;
  forward_ref = 0;
  backward_ref_fgc = 0;
  moved_objects_per_gc = 0;

  memset(obj_distr_size, 0, sizeof(obj_distr_size));

  primitive_arrays_size = 0;
  primitive_obj_size = 0;
  non_primitive_obj_size = 0;

  num_primitive_arrays = 0;
  num_primitive_obj = 0;
  num_non_primitive_obj = 0;

  class_obj_closure_size = 0;
  memset(class_obj_closure_distr_size, 0, sizeof(class_obj_closure_distr_size));
}

// Increase by one the counter that shows the total number of
// objects that are moved to H2. Increase by 'size' the counter that shows 
// the total size of the objects that are moved to H2. Increase by one
// the number of objects that are moved in the current gc cycle to H2.
void TeraStatistics::add_object(size_t size) {
  total_objects++;
  total_objects_size += size * HeapWordSize;
  moved_objects_per_gc++;

  update_object_distribution(size);
}

// We count the number of forwarding references from
// objects in H1 to objects in H2 during the marking phase.
void TeraStatistics::add_forward_ref() {
  forward_ref++;
}
  
// Increase by one the number of backward references per full GC;
void TeraStatistics::add_back_ref() {
  backward_ref_fgc++;
}
  
// Update the distribution of objects size. We divide the objects
// into three categories:
//  Bytes
//  KBytes
//  MBytes
void TeraStatistics::update_object_distribution(size_t size) {
  size_t obj_size = (size * HeapWordSize) / 1024UL;
  int count = 0;

  while (obj_size > 0) {
    count++;
    obj_size/=1024UL;
  }

  assert(count <=2, "Array out of range");
  obj_distr_size[count]++;
}
  
// Print the statistics of TeraHeap at the end of each FGC
// Will print:
//	- the total forward references from the H1 to the H2
//	- the total backward references from H2 to the H1
//	- the total objects that has been moved to H2
//	- the current total size of objects in H2
//	- the current total objects that are moved in H2
void TeraStatistics::print_major_gc_stats() {
	thlog_or_tty->print_cr("[STATISTICS] | TOTAL_FORWARD_PTRS = %lu\n", forward_ref);
	thlog_or_tty->print_cr("[STATISTICS] | TOTAL_BACK_PTRS = %lu\n", backward_ref_fgc);
	thlog_or_tty->print_cr("[STATISTICS] | TOTAL_TRANS_OBJ = %lu\n", moved_objects_per_gc);

	thlog_or_tty->print_cr("[STATISTICS] | TOTAL_OBJECTS  = %lu\n", total_objects);
	thlog_or_tty->print_cr("[STATISTICS] | TOTAL_OBJECTS_SIZE = %lu\n", total_objects_size);
  thlog_or_tty->print_cr("[STATISTICS] | DISTRIBUTION | B = %lu | KB = %lu | MB = %lu\n",
                         obj_distr_size[0], obj_distr_size[1], obj_distr_size[2]);

	thlog_or_tty->print_cr("[STATISTICS] | NUM_PRIMITIVE_ARRAYS  = %lu\n", num_primitive_arrays);
	thlog_or_tty->print_cr("[STATISTICS] | PRIMITIVE_ARRAYS_SIZE  = %lu\n", primitive_arrays_size);
  thlog_or_tty->print_cr("[STATISTICS] | NUM_PRIMITIVE_OBJ  = %lu\n", num_primitive_obj);
	thlog_or_tty->print_cr("[STATISTICS] | PRIMITIVE_OBJ_SIZE  = %lu\n", primitive_obj_size);
  thlog_or_tty->print_cr("[STATISTICS] | NUM_NON_PRIMITIVE_OBJ  = %lu\n", num_non_primitive_obj);
	thlog_or_tty->print_cr("[STATISTICS] | NON_PRIMITIVE_OBJ_SIZE  = %lu\n", non_primitive_obj_size);

	thlog_or_tty->print_cr("[STATISTICS] | CLASS_OBJ_CLOSURE_SIZE = %lu\n", class_obj_closure_size);
	thlog_or_tty->print_cr("[STATISTICS] | CLASS_OBJ_CLOSURE_SIZE_DISTRIBUTION | B = %lu | KB = %lu | MB = %lu\n",
                        class_obj_closure_distr_size[0], class_obj_closure_distr_size[1], class_obj_closure_distr_size[2]);

#ifdef BACK_REF_STAT
  back_ref_histogram->print_back_ref_stats();
#endif

  thlog_or_tty->flush();

  init_counters();
}

// Init the statistics counters of TeraHeap to zero for the next GC
void TeraStatistics::init_counters() {
  forward_ref = 0;
  backward_ref_fgc = 0;
  moved_objects_per_gc = 0;
  primitive_arrays_size = 0;
  primitive_obj_size = 0;
  non_primitive_obj_size = 0;
  num_primitive_arrays = 0;
  num_primitive_obj = 0;
  num_non_primitive_obj = 0;
  class_obj_closure_size = 0;
  memset(class_obj_closure_distr_size, 0, sizeof(class_obj_closure_distr_size));
}

// Update the statistics for primitive arrays (e.g., char[], int[]).
// Keep the number of 'instances' and their 'total_size' per major GC.
void TeraStatistics::add_primitive_arrays_stats(size_t instances, size_t total_size) {
  num_primitive_arrays += instances;
  primitive_arrays_size += total_size;
}

// Update the statistics for objects with only primitive type fields.
// Keep the number of 'instances' and their 'total_size' per major GC.
void TeraStatistics::add_primitive_obj_stats(size_t instances, size_t total_size) {
  num_primitive_obj += instances;
  primitive_obj_size += total_size;
}
  
// Update the statistics for objects with non primitive type fields.
// Keep the number of 'instances' and their 'total_size' per major GC.
void TeraStatistics::add_non_primitive_obj_stats(size_t instances, size_t total_size) {
  num_non_primitive_obj += instances;
  non_primitive_obj_size += total_size;
}

void TeraStatistics::incr_class_objects_closure_size(size_t size) {
  size_t obj_size = size * HeapWordSize;
  int count = 0;

  class_obj_closure_size += obj_size;

  obj_size /= 1024UL;

  while (obj_size > 0) {
    count++;
    obj_size /= 1024UL;
  }

  assert(count <=2, "Array out of range");
  class_obj_closure_distr_size[count]++;
}
  
// Increase backward references per minor GC cycle
void TeraStatistics::incr_back_ref_mgc() {
  backward_ref_mgc++;
}
  
// Reset the backward references counter for the new minor GC
// cycle
void TeraStatistics::reset_back_ref_mgc() {
  backward_ref_mgc = 0;
}

// Print the backward references per minor GC cycles
void TeraStatistics::print_back_ref_mgc() {
  thlog_or_tty->print_cr("[STATISTICS] | BACK_PTRS_PER_MGC = %lu\n", backward_ref_mgc);
  thlog_or_tty->flush();
}

#ifdef BACK_REF_STAT
// Enable backward reference traversal
void TeraStatistics::enable_back_ref_traversal(oop *obj) {
  back_ref_histogram.enable_back_ref_traversal(obj);
}
  
// Add a new entry to the histogram for back reference that start from
// 'obj' and results in H1 (new or old generation).
// Use this function with a single GC thread
void TeraStatistics::update_back_ref_stats(bool is_in_old_gen, bool is_in_h2) {
  back_ref_histogram.update_back_ref_stats(is_in_old_gen, is_in_h2);
}
#endif // BACK_REF_STAT
