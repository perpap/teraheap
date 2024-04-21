#ifndef SHARE_VM_GC_IMPLEMENTATION_TERAHEAP_TERASTATISTICS_HPP
#define SHARE_VM_GC_IMPLEMENTATION_TERAHEAP_TERASTATISTICS_HPP

#include "memory/allocation.hpp"
#include "oops/oop.hpp"

#ifdef BACK_REF_STAT
#include "gc_implementation/teraHeap/teraHistoBackReferences.hpp"
#endif

class TeraStatistics: public CHeapObj<mtInternal> {
private:
  size_t  total_objects;               //< Total number of objects located in TeraHeap
  size_t  total_objects_size;          //< Total number of objects size

  size_t  forward_ref;                 //< Total number of forward ptrs per full GC
  size_t  backward_ref_fgc;            //< Total number of back ptrs per full GC
  size_t  backward_ref_mgc;            //< Total number of back ptrs per minor GC
  size_t  moved_objects_per_gc;        //< Total number of objects transfered to
                                       //< TeraHeap per FGC

  size_t obj_distr_size[3];            //< Object size distribution between B, KB, MB

  size_t primitive_arrays_size;        //< Total size of promitive arrays
  size_t primitive_obj_size;           //< Total size of objects with ONLY primitive type fields 
  size_t non_primitive_obj_size;       //< Total size of objects with non primitive type fields
  
  size_t num_primitive_arrays;         //< Total number of promitive arrays instances
  size_t num_primitive_obj;            //< Total number of objects with ONLY primitive type fields 
  size_t num_non_primitive_obj;        //< Total size of objects with non primitive type fields
  
  size_t h2_primitive_array_size;      //< Total size of H2 objects that are primitive arrays 
  size_t num_h2_primitive_array;       //< Total number of H2 objects that are primitive arrays
  
  size_t class_obj_closure_size;       //< Size of the transitive closure of
                                       // the mirror instances klass (class objects)
  size_t class_obj_closure_distr_size[3]; //< Objects distribution
                                          //size between B, KB, MB that belongs to the
                                          //transitive closure of the class objects
#ifdef BACK_REF_STAT
  TeraHistoBackReferences back_ref_histogram;
#endif

  // Init the statistics counters of TeraHeap to zero for the next GC
  void init_counters();

public:

  TeraStatistics();

  // Increase by one the counter that shows the total number of
  // objects that are moved to H2. Increase by 'size' the counter that shows
  // the total size of the objects that are moved to H2. Increase by one
  // the number of objects that are moved in the current gc cycle to H2.
  void add_object(size_t size);

  // We count the number of forwarding references from
  // objects in H1 to objects in H2 during the marking phase.
  void add_forward_ref();

  // Increase by one the number of backward references per full GC;
  void add_back_ref();

  // Update the distribution of objects size. We divide the objects
  // into three categories: (1) Bytes, (2) KBytes, and (3) MBytes
  void update_object_distribution(size_t size);
  
  // Update the number of forwarding tables 
  void add_fwd_tables();
  
  // Print the statistics of TeraHeap at the end of each FGC
  // Will print:
  //	- the total forward references from the H1 to the H2
  //	- the total backward references from H2 to the H1
  //	- the total objects that has been moved to H2
  //	- the current total size of objects in H2
  //	- the current total objects that are moved in H2
  void print_major_gc_stats();

  // Update the statistics for primitive arrays (e.g., char[], int[]).
  // Keep the number of 'instances' and their 'total_size' per major GC.
  void add_primitive_arrays_stats(size_t instances, size_t total_size);
  
  // Update the statistics for objects with only primitive type fields.
  // Keep the number of 'instances' and their 'total_size' per major GC.
  void add_primitive_obj_stats(size_t instances, size_t total_size);
  
  // Update the statistics for objects with non primitive type fields.
  // Keep the number of 'instances' and their 'total_size' per major GC.
  void add_non_primitive_obj_stats(size_t instances, size_t total_size);

  // Increase the size of the transitive closure of instance mirror
  // klass objects who represents class objects
  void incr_class_objects_closure_size(size_t size);

  // Increase backward references per minor GC cycle
  void incr_back_ref_mgc();
  
  // Reset the backward references counter for the new minor GC
  // cycle
  void reset_back_ref_mgc();

  // Print the backward references per minor GC cycles
  void print_back_ref_mgc();

#ifdef BACK_REF_STAT
  // Enable backward reference traversal
  void enable_back_ref_traversal(oop *obj);

  // Add a new entry to the histogram for back reference that start from
  // 'obj' and results in H1 (new or old generation).
  // Use this function with a single GC thread
  void update_back_ref_stats(bool is_in_old_gen, bool is_in_h2);
#endif
};

#endif // SHARE_VM_GC_IMPLEMENTATION_TERAHEAP_TERASTATISTICS_HPP
