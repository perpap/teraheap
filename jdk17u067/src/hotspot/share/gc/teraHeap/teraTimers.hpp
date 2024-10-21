#ifndef SHARE_GC_TERAHEAP_TERATIMERS_HPP
#define SHARE_GC_TERAHEAP_TERATIMERS_HPP

#include "memory/allocation.hpp"
#include "gc/teraHeap/teraDynamicResizingPolicy.hpp"
#include <sys/time.h>

//#define rdtsc get_cycles
#if 1
#include <chrono>
//#include <functional>
#endif

template <typename TimeUnit = std::chrono::milliseconds>
class ScopedTimer {
  public:
    using ClockType = std::chrono::steady_clock;
    // Start the timer upon construction
    ScopedTimer(const char *msg = "Timer") 
	: m_msg(msg), m_start(ClockType::now()) {}
    //ScopedTimer(const ScopedTimer&) = delete;
    //ScopedTimer(ScopedTimer&&) = default;
    //auto operator=(const ScopedTimer&) -> ScopedTimer& = delete;
    //auto operator=(ScopedTimer&&) -> ScopedTimer& = default;
    // Destructor stops the timer and prints the elapsed time
    ~ScopedTimer() {
	using namespace std::chrono;
	auto end = ClockType::now();
	auto elapsed = duration_cast<TimeUnit>(end - m_start).count();
	//thlog_or_tty->print_cr("[STATISTICS] | %s %f %s\n", m_msg, (double)elapsed, timeUnitName()); 
        thlog_or_tty->print_cr("[STATISTICS] | %s %f\n", m_msg, elapsed); 
    }
    // Factory function to create a Timer scoped to the calling function
    //template <typename TimeUnit = std::chrono::milliseconds>
    static ScopedTimer<TimeUnit> createScopedTimer(const char *msg = "Scoped Timer") {
        return ScopedTimer<TimeUnit>(msg);
    }
  private:
    const char *m_msg;
    std::chrono::time_point<ClockType> m_start;
    
    // Helper function to display time unit names
    const char* timeUnitName() const {
	if (std::is_same<TimeUnit, std::chrono::nanoseconds>::value) {
	    return "ns";
	} else if (std::is_same<TimeUnit, std::chrono::microseconds>::value) {
	    return "µs";
	} else if (std::is_same<TimeUnit, std::chrono::milliseconds>::value) {
	    return "ms";
	} else if (std::is_same<TimeUnit, std::chrono::seconds>::value) {
	    return "s";
	} else {
	    return "unknown unit";
	}
    }
};// end of ScopedTimer
  
  

class TeraTimers: public CHeapObj<mtInternal> {
public:
  #if 0	
  template <typename TimeUnit = std::chrono::milliseconds>
  class ScopedTimer {
  public:
    using ClockType = std::chrono::steady_clock;
    // Start the timer upon construction
    ScopedTimer(const char *msg = "Timer") 
	: m_msg(msg), m_start(ClockType::now()) {}
    //ScopedTimer(const ScopedTimer&) = delete;
    //ScopedTimer(ScopedTimer&&) = default;
    //auto operator=(const ScopedTimer&) -> ScopedTimer& = delete;
    //auto operator=(ScopedTimer&&) -> ScopedTimer& = default;
    // Destructor stops the timer and prints the elapsed time
    ~ScopedTimer() {
	using namespace std::chrono;
	auto end = ClockType::now();
	auto elapsed = duration_cast<TimeUnit>(end - m_start).count();
	//thlog_or_tty->print_cr("[STATISTICS] | %s %f %s\n", m_msg, (double)elapsed, timeUnitName()); 
        thlog_or_tty->print_cr("[STATISTICS] | %s %f\n", m_msg, elapsed); 
    }

  private:
    const char *m_msg;
    std::chrono::time_point<ClockType> m_start;
    
    // Helper function to display time unit names
    const char* timeUnitName() const {
	if (std::is_same<TimeUnit, std::chrono::nanoseconds>::value) {
	    return "ns";
	} else if (std::is_same<TimeUnit, std::chrono::microseconds>::value) {
	    return "µs";
	} else if (std::is_same<TimeUnit, std::chrono::milliseconds>::value) {
	    return "ms";
	} else if (std::is_same<TimeUnit, std::chrono::seconds>::value) {
	    return "s";
	} else {
	    return "unknown unit";
	}
    }
  };// end of ScopedTimer
  
  // Factory function to create a Timer scoped to the calling function
  template <typename TimeUnit = std::chrono::milliseconds>
  static ScopedTimer<TimeUnit> createScopedTimer(const char *msg = "Scoped Timer") {
        return ScopedTimer<TimeUnit>(msg);
  }
#endif
private:
  static const uint64_t CYCLES_PER_SECOND;

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

#define CREATE_SCOPED_TIMER_IF(cond, timerName, timeUnit, msg) \
    ScopedTimer<timeUnit> timerName(""); \
    if (cond) { \
        timerName = ScopedTimer<timeUnit>(msg); \
    }


#endif
