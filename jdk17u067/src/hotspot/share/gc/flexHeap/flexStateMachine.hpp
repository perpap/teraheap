#ifndef SHARE_GC_FLEXHEAP_FLEXSTATEMACHINE_HPP
#define SHARE_GC_FLEXHEAP_FLEXSTATEMACHINE_HPP

#include "gc/flexHeap/flexEnum.h"
#include "gc/shared/collectedHeap.hpp"
#include "memory/allocation.hpp"
#include "memory/sharedDefines.h"

class FlexStateMachine : public CHeapObj<mtInternal> {
public:
  virtual void fsm(fh_states *cur_state, fh_actions *cur_action, double gc_time_ms,
                   double io_time_ms) = 0;

  virtual void state_wait_after_grow(fh_states *cur_state, fh_actions *cur_action,
                                     double gc_time_ms, double io_time_ms) = 0;

  virtual void state_wait_after_shrink(fh_states *cur_state, fh_actions *cur_action,
                                       double gc_time_ms, double io_time_ms) = 0;

  void state_no_action(fh_states *cur_state, fh_actions *cur_action,
                       double gc_time_ms, double io_time_ms);
  
  // Read the memory statistics for the cgroup
  size_t read_cgroup_mem_stats(bool read_page_cache);

  // Read the process anonymous memory
  size_t read_process_anon_memory();
};

class FlexSimpleStateMachine : public FlexStateMachine {
public:
  FlexSimpleStateMachine() {
    tty->print_cr("Resizing Policy = FlexSimpleStateMacine\n");
    tty->flush();
  }
  void fsm(fh_states *cur_state, fh_actions *cur_action, double gc_time_ms,
           double io_time_ms);

  void state_wait_after_grow(fh_states *cur_state, fh_actions *cur_action,
                             double gc_time_ms, double io_time_ms) {
    return;
  }

  void state_wait_after_shrink(fh_states *cur_state, fh_actions *cur_action,
                               double gc_time_ms, double io_time_ms) {
    return;
  }
};

class FlexSimpleWaitStateMachine : public FlexStateMachine {
public:
  FlexSimpleWaitStateMachine() {
    tty->print_cr("Resizing Policy = FlexSimpleWaitStateMacine\n");
    tty->flush();
  }

  void fsm(fh_states *cur_state, fh_actions *cur_action, double gc_time_ms,
           double io_time_ms);

  void state_wait_after_grow(fh_states *cur_state, fh_actions *cur_action,
                             double gc_time_ms, double io_time_ms);

  void state_wait_after_shrink(fh_states *cur_state, fh_actions *cur_action,
                               double gc_time_ms, double io_time_ms);
};

class FlexFullOptimizedStateMachine : public FlexSimpleWaitStateMachine {
public:
  FlexFullOptimizedStateMachine() {
    tty->print_cr("Resizing Policy = FlexFullOptimizedStateMachine\n");
    tty->flush();
  }
  
  void state_wait_after_shrink(fh_states *cur_state, fh_actions *cur_action,
                             double gc_time_ms, double io_time_ms);

  void state_wait_after_grow(fh_states *cur_state, fh_actions *cur_action,
                             double gc_time_ms, double io_time_ms);
};

#endif // SHARE_GC_FLEXHEAP_FLEXSTATEMACHINE_HPP
