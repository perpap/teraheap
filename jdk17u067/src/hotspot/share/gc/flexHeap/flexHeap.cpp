#include "gc/shared/collectedHeap.hpp"
#include "gc/parallel/parallelScavengeHeap.hpp"
#include "gc/flexHeap/flexHeap.hpp"
#include "memory/universe.hpp"
#include "memory/universe.hpp"

#define BUFFER_SIZE 1024
#define CYCLES_PER_SECOND 2.4e9; // CPU frequency of 2.4 GHz
#define REGULAR_INTERVAL ((2LL * 1000)) 

// Intitilize the cpu usage statistics
FlexCPUUsage* FlexHeap::init_cpu_usage_stats() {
  return FlexCPUStatsPolicy ? static_cast<FlexCPUUsage*>(new FlexMultiTenantCPUUsage()) :
                              static_cast<FlexCPUUsage*>(new FlexSimpleCPUUsage());
}

// Initialize the policy of the state machine
FlexStateMachine* FlexHeap::init_state_machine_policy() {
  switch (FlexResizingPolicy) {
    case 1:
      return new FlexSimpleWaitStateMachine();
    case 2:
      return new FlexFullOptimizedStateMachine(); 
    default:
      break;
  }
  
  return new FlexSimpleStateMachine();
}

// We use this function to take decision in case of minor GC which
// happens before a major gc.
void FlexHeap::dram_repartition(bool *need_full_gc) {
  double avg_gc_time_ms, avg_io_time_ms;

  calculate_gc_io_costs(&avg_gc_time_ms, &avg_io_time_ms);

  state_machine->fsm(&cur_state, &cur_action, avg_gc_time_ms, avg_io_time_ms);
  print_state_action();

  switch (cur_action) {
    case FH_SHRINK_HEAP:
      action_shrink_heap();
      break;
    case FH_GROW_HEAP:
      action_grow_heap(need_full_gc);
      break;
    case FH_NO_ACTION:
    case FH_IOSLACK:
    case FH_WAIT_AFTER_GROW:
    case FH_CONTINUE:
      break;
  }
  reset_counters();
}

// Print states (for debugging and logging purposes)
void FlexHeap::print_state_action() {
  //if (!FlexHeapStatistics)
  //  return;

  tty->stamp(true);
  tty->print_cr("STATE = %s\n", state_name[cur_state]);
  tty->print_cr("ACTION = %s\n", action_name[cur_action]);
  //tty->flush();
  //fprintf(stderr, "STATE = %s\n", state_name[cur_state]);
  //fprintf(stderr, "ACTION = %s\n", action_name[cur_action]);
}

// Initialize the array of state names
void FlexHeap::init_state_actions_names() {
  strncpy(action_name[0], "NO_ACTION",       10);
  strncpy(action_name[1], "SHRINK_HEAP",     12);
  strncpy(action_name[2], "GROW_HEAP",       10);
  strncpy(action_name[3], "CONTINUE",         9);
  strncpy(action_name[4], "IOSLACK",          8);
  strncpy(action_name[5], "WAIT_AFTER_GROW", 16);
  
  strncpy(state_name[0], "S_NO_ACTION",      12);
  strncpy(state_name[1], "S_WAIT_SHRINK",    14);
  strncpy(state_name[2], "S_WAIT_GROW",      12);
}

// Calculate the average of gc and io costs and return their values.
// We use these values to determine the next actions.
void FlexHeap::calculate_gc_io_costs(double *avg_gc_time_ms,
                                     double *avg_io_time_ms) {
  double iowait_time_ms = 0;
  uint64_t dev_time_end = 0;

  // Check if we are inside the window
  if (!is_window_limit_exeed()) {
    *avg_gc_time_ms = 0;
    *avg_io_time_ms = 0;
    return;
  }

  // Calculate the user and iowait time during the window interval
  cpu_usage->read_cpu_usage(STAT_END);
  cpu_usage->calculate_iowait_time(interval, &iowait_time_ms);

  assert(gc_time <= interval, "GC time should be less than the window interval");
  assert(iowait_time_ms <= interval, "GC time should be less than the window interval");
  assert(*device_active_time_ms <= interval, "GC time should be less than the window interval");

  //fprintf(stderr, ">>>>>>>> iowait_time_ms = %f\n", iowait_time_ms);

  if (iowait_time_ms < 0)
    iowait_time_ms = 0;

  history(gc_time, iowait_time_ms);

  *avg_io_time_ms = calc_avg_time(hist_iowait_time, FH_HIST_SIZE);
  *avg_gc_time_ms = calc_avg_time(hist_gc_time, FH_GC_HIST_SIZE);
  //fprintf(stderr, "IO_TIME_MS = %f\n", *avg_io_time_ms);
  //fprintf(stderr, "GC_TIME_MS = %f\n", *avg_gc_time_ms);

  if (FlexHeapStatistics)
    debug_print(*avg_io_time_ms, *avg_gc_time_ms, interval, iowait_time_ms, gc_time);
}

FlexHeap::FlexHeap() {
  cpu_usage = init_cpu_usage_stats();

  window_start_time = rdtsc();
  cpu_usage->read_cpu_usage(STAT_START);
  gc_time = 0;
  cur_action = FH_NO_ACTION;
  cur_state = FHS_NO_ACTION;

  memset(hist_gc_time, 0, FH_GC_HIST_SIZE * sizeof(double));
  memset(hist_iowait_time, 0, FH_HIST_SIZE * sizeof(double));

  window_interval = REGULAR_INTERVAL;
  prev_full_gc_end = 0;
  last_full_gc_start = 0;

  init_state_actions_names();
  state_machine = init_state_machine_policy();
}
  
// Calculate ellapsed time
double FlexHeap::ellapsed_time(uint64_t start_time,
                                                uint64_t end_time) {

  double elapsed_time = (double)(end_time - start_time) / CYCLES_PER_SECOND;
  return (elapsed_time * 1000.0);
}

// Set current time since last window
void FlexHeap::reset_counters() {
  window_start_time = rdtsc();
  cpu_usage->read_cpu_usage(STAT_START);
  gc_time = 0;
  window_interval = REGULAR_INTERVAL;
}

// Check if the window limit exceed time
bool FlexHeap::is_window_limit_exeed() {
  uint64_t window_end_time;
  static int i = 0;
  window_end_time = rdtsc();
  interval = ellapsed_time(window_start_time, window_end_time);

#ifdef PER_MINOR_GC
  return true;
#else
  return (interval >= window_interval) ? true : false;
#endif // PER_MINOR_GC
}

void FlexHeap::gc_start(double start_time) {
  last_full_gc_start = start_time;
}

// Count the iowait time during gc and update the gc_iowait_time_ms
// counter for the current window
void FlexHeap::gc_end(double gc_duration, double last_full_gc) {
  static double last_gc_end = 0;
  gc_time += gc_duration;
  prev_full_gc_end = last_gc_end;
  last_gc_end = last_full_gc;
}

void FlexHeap::action_grow_heap(bool *need_full_gc) {
  should_grow_heap = true;
  ParallelScavengeHeap::old_gen()->resize(10000);
  should_grow_heap = false;

  // Recalculate the GC cost
  calculate_gc_cost(0);
  // We grow the heap so, there is no need to perform the gc. We
  // postpone the gc.
  *need_full_gc = false;
}

void FlexHeap::action_shrink_heap() {
  should_shrink_heap = true;
  ParallelScavengeHeap::old_gen()->resize(10000);
  should_shrink_heap = false;

  // Recalculate the GC cost
  calculate_gc_cost(0);
}

// Print counters for debugging purposes
void FlexHeap::debug_print(double avg_iowait_time, double avg_gc_time,
                                            double interval, double cur_iowait_time, double cur_gc_time) {
  thlog_or_tty->print_cr("avg_iowait_time_ms = %lf\n", avg_iowait_time);
  thlog_or_tty->print_cr("avg_gc_time_ms = %lf\n", avg_gc_time);
  thlog_or_tty->print_cr("cur_iowait_time_ms = %lf\n", cur_iowait_time);
  thlog_or_tty->print_cr("cur_gc_time_ms = %lf\n", cur_gc_time);
  thlog_or_tty->print_cr("interval = %lf\n", interval);
  thlog_or_tty->flush();
}

// Find the average of the array elements
double FlexHeap::calc_avg_time(double *arr, int size) {
  double sum = 0;

  for (int i = 0; i < size; i++) {
    sum += arr[i];
  }

  return (double) sum / size;
}

// Grow the capacity of H1.
size_t FlexHeap::grow_heap() {
  PSOldGen *old_gen = ParallelScavengeHeap::old_gen();
  size_t new_size = 0;
  size_t cur_size = old_gen->capacity_in_bytes();
  size_t ideal_page_cache  = prev_page_cache_size + shrinked_bytes;
  size_t cur_page_cache = state_machine->read_cgroup_mem_stats(true);

  bool ioslack = (cur_page_cache < ideal_page_cache); 
  shrinked_bytes = 0;
  prev_page_cache_size = 0;

  if (ioslack) {
    if (FlexHeapStatistics) {
      thlog_or_tty->print_cr("Grow_by = %lu\n", (ideal_page_cache - cur_page_cache));
      thlog_or_tty->flush();
    }

    // Reset the metrics for page cache because now we use the ioslack
    return cur_size + (ideal_page_cache - cur_page_cache); 
  }

  new_size = cur_size + (GROW_STEP * state_machine->read_cgroup_mem_stats(true));

  if (FlexHeapStatistics) {
    thlog_or_tty->print_cr("[GROW_H1] Before = %lu | After = %lu | PageCache = %lu\n",
                           cur_size, new_size, state_machine->read_cgroup_mem_stats(true));
    thlog_or_tty->flush();
  }

  return new_size;
}

// Shrink the capacity of H1 to increase the page cache size.
// This functions is called by the collector
size_t FlexHeap::shrink_heap() {
  PSOldGen *old_gen = ParallelScavengeHeap::old_gen();
  size_t cur_size = old_gen->capacity_in_bytes();
  size_t used_size = old_gen->used_in_bytes();
  size_t free_space = cur_size - used_size;
  size_t new_size = used_size + (SHRINK_STEP * free_space); ;

  set_shrinked_bytes(cur_size - new_size);
  set_current_size_page_cache();

  if (FlexHeapStatistics) {
    thlog_or_tty->print_cr("[SHRINK_H1] Before = %lu | After = %lu | PageCache = %lu\n",
                           cur_size, new_size, state_machine->read_cgroup_mem_stats(true));
    thlog_or_tty->flush();
  }

  return new_size;
}
  
// Save the history of the GC and iowait overheads. We maintain two
// ring buffers (one for GC and one for iowait) and update these
// buffers with the new values for GC cost and IO overhead.
void FlexHeap::history(double gc_time_ms, double iowait_time_ms) {
  static int index = 0;
  double gc_ratio = calculate_gc_cost(gc_time_ms);

  if (gc_ratio >= 1) {
    hist_gc_time[index % FH_GC_HIST_SIZE] = (interval - iowait_time_ms);
  } else {
    hist_gc_time[index % FH_GC_HIST_SIZE] = gc_ratio * interval;
    //hist_gc_time[index % FH_GC_HIST_SIZE] = gc_ratio * (interval - iowait_time_ms);
  }

  //hist_gc_time[index % FH_GC_HIST_SIZE] = gc_ratio * interval;
  hist_iowait_time[index % FH_HIST_SIZE] = iowait_time_ms;
  index++;
}

// Calculation of the GC cost prediction.
double FlexHeap::calculate_gc_cost(double gc_time_ms) {
  static double last_gc_time_ms  = 0;
  static double last_gc_free_bytes_ratio = 1;
  static double gc_percentage_ratio = 0;

  // Interval between the previous and the current gc
  double gc_interval_ms = (last_full_gc_start - prev_full_gc_end) * 1000;

  // Non major GC cycles happens.
  if (gc_time_ms == 0 && gc_interval_ms == 0)
    return 0;

  bool is_no_action = !(cur_action == FH_SHRINK_HEAP || cur_action == FH_GROW_HEAP);

  if (gc_time_ms == 0 && is_no_action) {
    return gc_percentage_ratio;
  }

  // Free bytes
  PSOldGen *old_gen = ParallelScavengeHeap::old_gen();
  double cur_live_bytes_ratio = (double) old_gen->used_in_bytes() / old_gen->capacity_in_bytes();
  double cur_free_bytes_ratio = (1 - cur_live_bytes_ratio);
  double cost = 0;

  if (gc_time_ms == 0) {
    cost = last_gc_time_ms * (1 - cur_free_bytes_ratio);
    gc_percentage_ratio = (cost * last_gc_free_bytes_ratio) / (cur_free_bytes_ratio * gc_interval_ms);

    return gc_percentage_ratio;
  }

  // Calculate the cost of current GC
  last_gc_time_ms = gc_time_ms;
  last_gc_free_bytes_ratio = cur_free_bytes_ratio;
  cost = last_gc_time_ms * (1 - cur_free_bytes_ratio);
  gc_percentage_ratio = (cost * last_gc_free_bytes_ratio) / (cur_free_bytes_ratio * gc_interval_ms);

  return gc_percentage_ratio;
}
