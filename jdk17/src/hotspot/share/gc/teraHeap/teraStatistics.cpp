#include "gc/shared/gc_globals.hpp"
#include "gc/teraHeap/teraStatistics.hpp"
#include "runtime/arguments.hpp"

TeraStatistics::TeraStatistics() {
  total_objects_moved = 0;
  total_objects_size = 0;
  backward_ref = 0;
  _is_mixed_gc = false;
  _is_full_gc = false;
}


// Increase by one the counter that shows the total number of
// objects that are moved to H2. Increase by 'size' the counter that shows 
// the total size of the objects that are moved to H2. Increase by one
// the number of objects that are moved in the current gc cycle to H2.
void TeraStatistics::add_object(long size) {
  total_objects_moved++;
  total_objects_size += size;
}


// Increase by one the number of backward references per full GC;
void TeraStatistics::add_back_ref() {
  backward_ref++;
}



// Print the statistics of TeraHeap at the end of each FGC
// Will print:
//	- the curr backward references from H2 to the H1
//	- the total objects that has been moved to H2
void TeraStatistics::print_gc_stats() {

  if (_is_mixed_gc) {
    thlog_or_tty->print_cr("[MIXED] | BACK_PTRS = %lu", backward_ref);
    thlog_or_tty->print_cr("[MIXED] | TOTAL_OBJECTS  = %lu", total_objects_moved);
    thlog_or_tty->print_cr("[MIXED] | TOTAL_OBJECTS_SIZE = %lu", total_objects_size);
    thlog_or_tty->print_cr("[MIXED] | TIME_SCAN_H2_CT %.3lf ms", h2_card_table_scan_time_ms);
    thlog_or_tty->print_cr("[MIXED] | TIME_TO_ALLOC_H2 %.3lf ms", h2_allocate_ms);
    thlog_or_tty->print_cr("[MIXED] | TIME_TO_COPY_H2 %.3lf ms", h2_copy_ms);
    // thlog_or_tty->print_cr("[MIXED] | marked/moved: %lu/%lu", Universe::teraHeap()->get_marked(), Universe::teraHeap()->get_moved());
  } else if (_is_full_gc) {
    // FGC phases breakdown
    thlog_or_tty->print_cr("[FULL] | BACK_PTRS = %lu", backward_ref);
    thlog_or_tty->print_cr("[FULL] | TOTAL_OBJECTS  = %lu", total_objects_moved);
    thlog_or_tty->print_cr("[FULL] | TOTAL_OBJECTS_SIZE = %lu", total_objects_size);
    thlog_or_tty->print_cr("[FULL] | TIME_SCAN_H2_CT %.3lf ms", h2_card_table_scan_time_ms);
    thlog_or_tty->print_cr("[FULL] | TIME_TO_ALLOC_H2 %.3lf ms", h2_allocate_ms);
    thlog_or_tty->print_cr("[FULL] | TIME_TO_COPY_H2 %.3lf ms (accurate for single threaded)", h2_copy_ms);
    // thlog_or_tty->print_cr("[FULL] | marked/moved: %lu/%lu", Universe::teraHeap()->get_marked(), Universe::teraHeap()->get_moved());
  } else {
    // Young
    thlog_or_tty->print_cr("[YOUNG] | BACK_PTRS = %lu", backward_ref);
    thlog_or_tty->print_cr("[YOUNG] | TIME_SCAN_H2_CT %.3lf ms", h2_card_table_scan_time_ms);
  }

  thlog_or_tty->flush();

  // Init the statistics counters of TeraHeap to zero for the next GC  
  backward_ref = 0;
  _is_mixed_gc = false;
  _is_full_gc = false;
}

