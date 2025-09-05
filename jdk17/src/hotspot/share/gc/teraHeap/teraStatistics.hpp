#ifndef SHARE_GC_TERAHEAP_TERASTATISTICS_HPP
#define SHARE_GC_TERAHEAP_TERASTATISTICS_HPP

#include "memory/allocation.hpp"
#include "oops/oop.hpp"

#ifdef RUSAGE_MUTATOR
  #include <sys/resource.h>
#endif // RUSAGE_MUTATOR

class TeraStatistics: public CHeapObj<mtInternal> {
private:
  long  total_objects_moved;
  long  total_objects_size;
  long  backward_ref;

  // time to scan h2 card table
  double h2_card_table_scan_time_ms;
  // time for evacuation to be completed
  double evac_time_ms;
  // total time of allocation to H2
  double h2_allocate_ms;

  // total time of copying to H2
  double h2_copy_ms;

  bool _is_mixed_gc;
  bool _is_full_gc;

  uint waste_space;

#ifdef RUSAGE_MUTATOR
  time_t last_mutator_system_time_s;
  long last_mutator_major_page_faults;
  long last_mutator_minor_page_faults;

  time_t mutator_system_time_s;
  long mutator_major_page_faults;
  long mutator_minor_page_faults;
#endif // RUSAGE_MUTATOR

public:

  TeraStatistics();

  // Increase by one the counter that shows the total number of
  // objects that are moved to H2. Increase by 'size' the counter that shows
  // the total size of the objects that are moved to H2. Increase by one
  // the number of objects that are moved in the current gc cycle to H2.
  void add_object(long size);


  // Increase by one the number of backward references per full GC;
  void add_back_ref();


  // Print the statistics of TeraHeap at the end of each FGC
  // Will print:
  //	- the total forward references from the H1 to the H2
  //	- the total backward references from H2 to the H1
  //	- the total objects that has been moved to H2
  //	- the current total size of objects in H2
  //	- the current total objects that are moved in H2
  void print_gc_stats();

  void record_h2_scan_time(double time){
    h2_card_table_scan_time_ms = time;
  }

  void record_evacuation_time(double time ){
    evac_time_ms = time;
  }

  void record_h2_allocate_time(double time) {
    h2_allocate_ms = time;
  }

  void record_h2_copy_time(double time) {
    h2_copy_ms = time;
  }

  void set_is_in_mix(bool is_mixed_gc) {
    _is_mixed_gc = is_mixed_gc;
  }

  void set_is_in_full_gc(bool is_full_gc) {
    _is_full_gc = is_full_gc;
  }

  void add_waste(uint waste) {
    waste_space += waste;
  }

  uint get_waste() {
    return waste_space;
  }

#ifdef RUSAGE_MUTATOR
  time_t get_last_mutator_system_time() {
    return last_mutator_system_time_s;
  }

  long get_last_mutator_major_page_faults() {
    return last_mutator_major_page_faults;
  }

  long get_last_mutator_minor_page_faults() {
    return last_mutator_minor_page_faults;
  }

  void set_last_mutator_system_time(time_t sys_time_s) {
    last_mutator_system_time_s = sys_time_s;
  }

  void set_last_mutator_major_page_faults(long major_page_faults) {
    last_mutator_major_page_faults = major_page_faults;
  }

  void set_last_mutator_minor_page_faults(long minor_page_faults) {
    last_mutator_minor_page_faults = minor_page_faults;
  }

  time_t get_mutator_system_time() {
    return mutator_system_time_s;
  }

  long get_mutator_major_page_faults() {
    return mutator_major_page_faults;
  }

  long get_mutator_minor_page_faults() {
    return mutator_minor_page_faults;
  }

  void add_mutator_system_time(time_t sys_time_s) {
    mutator_system_time_s += sys_time_s;
  }

  void add_mutator_major_page_faults(long major_page_faults) {
    mutator_major_page_faults += major_page_faults;
  }

  void add_mutator_minor_page_faults(long minor_page_faults) {
    mutator_minor_page_faults += minor_page_faults;
  }

  void report_rusage();

#endif // RUSAGE_MUTATOR
};

#endif // SHARE_GC_TERAHEAP_TERASTATISTICS_HPP
