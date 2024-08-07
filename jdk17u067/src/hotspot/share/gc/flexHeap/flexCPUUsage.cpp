#include "gc/flexHeap/flexCPUUsage.hpp"

#define BUFFER_SIZE 1024

// This functions uses /proc/stat to read the cpu usage. This is the
// ideal scenario of calculating the iowait time because we run
void FlexSimpleCPUUsage::read_cpu_usage(bool is_start) {
  unsigned long long total_cpu;
  unsigned long long cpu_iowait;
  FILE* stat_file = fopen("/proc/stat", "r");

  if (!stat_file) {
    fprintf(stderr, "Failed to open /proc/stat\n");
    return;
  }

  char buffer[BUFFER_SIZE];
  unsigned long long user, nice, system, idle, iowait, irq, softirq;

  while (fgets(buffer, BUFFER_SIZE, stat_file)) {
    if (strncmp(buffer, "cpu", 3) == 0) {
      sscanf(buffer, "%*s %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq);

      //total_cpu = user + nice + system + iowait + irq + softirq + idle;
      total_cpu = user + nice + system + iowait + irq + softirq;
      cpu_iowait = iowait;
      break;
    }
  }

  is_start ? cpu_start = total_cpu : cpu_end = total_cpu;
  is_start ? iowait_start = cpu_iowait : iowait_end = cpu_iowait;

  int res = fclose(stat_file);
  if (res != 0) {
    fprintf(stderr, "Error closing file");
  }
}

// Calculate iowait time based on the following formula
//
//                (cpu_iowait_after - cpu_iowait_before) 
//  iowait_time = -------------------------------------- * duration 
//                 (total_cpu_after - total_cpu_before)
//
void FlexSimpleCPUUsage::calculate_iowait_time(double duration,
                                               double *iowait_time) {
  unsigned long long iowait_diff = 0;
  unsigned long long cpu_diff = 0;

  iowait_diff = iowait_end - iowait_start;
  cpu_diff = cpu_end - cpu_start;

  *iowait_time = (cpu_diff == 0) ? 0 : ((double) iowait_diff / cpu_diff) * duration;
}
  

// Read CPU usage
void FlexMultiTenantCPUUsage::read_cpu_usage(bool is_start) {
  struct rusage tmp;
  getrusage(RUSAGE_SELF, &tmp);

  is_start ? start = tmp : end = tmp;
}

// Calculate iowait time of the process
void FlexMultiTenantCPUUsage::calculate_iowait_time(double duration,
                                                    double *iowait_time) {
  double user_time_ms = (double) (end.ru_utime.tv_sec - start.ru_utime.tv_sec) * 1000 +
                        (double) (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1000;

  double system_time_ms = (double) (end.ru_stime.tv_sec - start.ru_stime.tv_sec) * 1000 +
                          (double) (end.ru_stime.tv_usec - start.ru_stime.tv_usec) / 1000;

  *iowait_time = duration - ((user_time_ms + system_time_ms) / 8);
}


