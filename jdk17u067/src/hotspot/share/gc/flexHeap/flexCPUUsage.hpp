#ifndef SHARE_GC_FLEXHEAP_FLEXCPUUSAGE_HPP
#define SHARE_GC_FLEXHEAP_FLEXCPUUSAGE_HPP

#include "memory/allocation.hpp"
#include <sys/resource.h>

#define STAT_START true
#define STAT_END false

class FlexCPUUsage : public CHeapObj<mtInternal> {

public:
  // Read CPU usage
  virtual void read_cpu_usage(bool is_start) = 0;
  
  // Calculate iowait time of the process
  virtual void calculate_iowait_time(double duration, double *iowait_time) = 0;
};

class FlexSimpleCPUUsage : public FlexCPUUsage {
private:
  unsigned long long iowait_start;    //< CPU iowait at the start of the window
  unsigned long long iowait_end;      //< CPU iowait at the end of the window

  unsigned long long cpu_start;       //< CPU usage at the start of the window
  unsigned long long cpu_end;         //< CPU usage at the end of the window

public:
  // Read CPU usage
  void read_cpu_usage(bool is_start);
  
  // Calculate iowait time of the process
  void calculate_iowait_time(double duration, double *iowait_time);
};

class FlexMultiTenantCPUUsage : public FlexCPUUsage {
private:
  struct rusage start, end;

public:
  // Read CPU usage
  void read_cpu_usage(bool is_start);
  
  // Calculate iowait time of the process
  void calculate_iowait_time(double duration, double *iowait_time);
};

#endif // SHARE_GC_FLEXHEAP_FLEXCPUUSAGE_HPP
