#ifndef SHARE_GC_TERAHEAP_TERAPEBS_HPP
#define SHARE_GC_TERAHEAP_TERAPEBS_HPP

#include "memory/allocation.hpp"
#include <linux/perf_event.h>

struct PerfSample {
  struct perf_event_header header;
  __u64	ip;
  __u64 addr;        /* PERF_SAMPLE_ADDR */
  __u64 data_src;    /* PERF_SAMPLE_DATA_SRC */
};

struct PebsArgs {
  struct perf_event_mmap_page *pebs_buffer;
  volatile int *stop_thread;
  char *old_gen_start_addr;
  char *young_gen_start_addr;
  char *young_gen_end_addr;
  uint64_t *total_pebs_samples;
  uint64_t *total_young_gen_samples;
  uint64_t *total_old_gen_samples;
  uint64_t *total_zero_addr_samples;
  uint64_t *total_throttles;
  uint64_t *total_unthrottles;
};

class TeraPebs: public CHeapObj<mtInternal> {
private:
  struct perf_event_attr pe;                //< Perf event
  struct perf_event_mmap_page *pebs_buffer; //< Pebs buffer
  int fd;                                   //< File descriptor for perf event
  size_t mmap_size;                         //< Size for the pebs buffer
  pthread_t scan_thread;                    //< Scanner thread
  volatile int stop_thread = 0;             //< Flag to stop scanner thread
  char *old_gen_start_addr;                 //< Start address of the old generation
  char *young_gen_start_addr;               //< Start address of the young generation
  char *young_gen_end_addr;                 //<  End address of the young generation
  struct PebsArgs *args;                    //< Pebs scanner thread arguments

  // Statistics
  uint64_t total_loads;
  uint64_t total_pebs_samples;
  uint64_t total_young_gen_samples;
  uint64_t total_old_gen_samples;
  uint64_t total_zero_addr_samples;
  uint64_t total_throttles;
  uint64_t total_unthrottles;

  // The scanner thread iterates the pebs scanner finding new PEBS
  // records and check their addresses.
  static void* pebs_scan_thread(void* arg);

  void initialize_counters(void);

public:
  // Constructor
  void init_pebs(char *old_gen_start_addr, char *young_gen_start_addr,
           char *young_gen_stop_addr, int sample_period,
           bool load_ops);
  
  void init_perf(bool load_instruction);
  TeraPebs();
  ~TeraPebs();

  // Start performance event
  void start_perf(void);

  // Stop performance event
  void stop_perf(void);
  
  // Stop pebs
  void stop_pebs_scanner_thread(void);

  // Print statistics
  void print_pebs_statistics(void);

  void print_total_loads(void);
};

#endif 
