#ifndef SHARE_GC_TERAHEAP_TERATIMERS_HPP
#define SHARE_GC_TERAHEAP_TERATIMERS_HPP

#include "memory/allocation.hpp"
#include "gc/teraHeap/teraDynamicResizingPolicy.hpp"
#include <sys/time.h> 

class TeraTimers: public CHeapObj<mtInternal> {
  
private:
  //static const uint64_t CYCLES_PER_SECOND;

  struct timespec h2_scavenge_start_time;
  struct timespec h2_scavenge_end_time;

  struct timespec h1_marking_phase_start_time;
  struct timespec h1_marking_phase_end_time;

  struct timespec h2_mark_bwd_ref_start_time;
  struct timespec h2_mark_bwd_ref_end_time;
  
  struct timespec h2_precompact_start_time;
  struct timespec h2_precompact_end_time;

  struct timespec h1_summary_phase_start_time;
  struct timespec h1_summary_phase_end_time;
  
  struct timespec h2_compact_start_time;
  struct timespec h2_compact_end_time;
 /*
  struct timespec *h2_compact_group_region_lock_start_time;
  struct timespec *h2_compact_group_region_lock_end_time;
  double *h2_compact_group_region_lock_total_time;
  struct timespec *h2_compact_region_lock_start_time;
  struct timespec *h2_compact_region_lock_end_time;
  double *h2_compact_region_lock_total_time;
 */
  struct timespec h2_adjust_bwd_ref_start_time;
  struct timespec h2_adjust_bwd_ref_end_time;
  
  struct timespec h1_adjust_roots_start_time;
  struct timespec h1_adjust_roots_end_time;
  
  struct timespec h1_compact_start_time;
  struct timespec h1_compact_end_time;
  
  struct timespec h2_clear_fwd_table_start_time;
  struct timespec h2_clear_fwd_table_end_time;

  struct timespec h2_insert_fwd_table_start_time;
  struct timespec h2_insert_fwd_table_end_time;

  struct timespec *h1_card_table_start_time;
  struct timespec *h1_card_table_end_time;

  struct timespec *h2_card_table_start_time;
  struct timespec *h2_card_table_end_time;

  struct timespec malloc_start_time;
  struct timespec malloc_end_time;
  double malloc_time_per_gc;
 
  void print_ellapsed_time(struct timespec start_time, struct timespec end_time, const char* msg);
  void print_ellapsed_time(double elapsed_time, const char* msg);

public:
  TeraTimers();
  ~TeraTimers();
 
  void h2_scavenge_start();
  void h2_scavenge_end();

  void h1_marking_phase_start();
  void h1_marking_phase_end();

  void h2_mark_bwd_ref_start();
  void h2_mark_bwd_ref_end();
  
  void h2_precompact_start();
  void h2_precompact_end();

  void h1_summary_phase_start();
  void h1_summary_phase_end();

  void h2_compact_start();
  void h2_compact_end();
#if defined(H2_COMPACT_STATISTICS)
void h2_total_buffer_insert_elapsed_time();
void h2_total_flush_buffer_elapsed_time();
//void h2_total_flush_buffer_fragmentation_elapsed_time();
//void h2_total_flush_buffer_nofreespace_elapsed_time();
//void h2_total_async_request_elapsed_time();
#endif
  //void h2_compact_group_region_lock_start(unsigned int worker_id);
  //void h2_compact_group_region_lock_end(unsigned int worker_id);
  //void h2_compact_group_region_lock_add_total(unsigned int worker_id);
  //void h2_compact_region_lock_start(unsigned int worker_id);
  //void h2_compact_region_lock_end(unsigned int worker_id);
  //void h2_compact_region_lock_add_total(unsigned int worker_id);

  //void print_h2_compact_lock_time();

  void h2_adjust_bwd_ref_start();
  void h2_adjust_bwd_ref_end();

  void h1_adjust_roots_start();
  void h1_adjust_roots_end();

  void h1_compact_start();
  void h1_compact_end();
  
  void h2_clear_fwd_table_start();
  void h2_clear_fwd_table_end();
  
  // Keep for each GC thread the time that need to traverse the H1
  // card table.
  // Each thread writes the time in a table based on each ID and then we
  // take the maximum time from all the threads as the total time.
  void h1_card_table_start(unsigned int worker_id);
  void h1_card_table_end(unsigned int worker_id);

  // Keep for each GC thread the time that need to traverse the H2
  // card table.
  // Each thread writes the time in a table based on each ID and then we
  // take the maximum time from all the threads as the total time.
  void h2_card_table_start(unsigned int worker_id);
  void h2_card_table_end(unsigned int worker_id);
  
  void malloc_start();
  void malloc_end();
  void print_malloc_time();

  void print_card_table_scanning_time();

  // Keep 
  //void print_action_state(TeraDynamicResizingPolicy::state action);
};

#endif
