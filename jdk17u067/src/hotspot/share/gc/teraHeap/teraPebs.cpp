#include "gc/teraHeap/teraPebs.hpp"
#include "runtime/os.hpp"
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/mman.h>

#define PERF_PAGES (1 + (1 << 14))	// Has to be == 1+2^n

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void TeraPebs::initialize_counters(void) {
  total_loads             = 0;
  total_pebs_samples      = 0;
  total_young_gen_samples = 0;
  total_old_gen_samples   = 0;
  total_zero_addr_samples = 0;
  total_throttles         = 0;
  total_unthrottles       = 0;
}

TeraPebs::TeraPebs() {
  fprintf(stderr, "%s\n", __func__);
  
  args = nullptr;
  pebs_buffer = nullptr;
  mmap_size = 0;

  initialize_counters();
}

void* TeraPebs::pebs_scan_thread(void* arg) {
  struct PebsArgs *args = (struct PebsArgs *)arg;
  struct perf_event_mmap_page *pebs_buffer = args->pebs_buffer;
  volatile int *stop_thread = args->stop_thread;
  char *old_gen_start_addr = args->old_gen_start_addr;
  char *young_gen_start_addr = args->young_gen_start_addr;
  char *young_gen_end_addr = args->young_gen_end_addr;
  uint64_t *total_pebs_samples = args->total_pebs_samples;
  uint64_t *young_gen_samples = args->total_young_gen_samples;
  uint64_t *old_gen_samples = args->total_old_gen_samples;
  uint64_t *zero_addr_samples = args->total_zero_addr_samples;
  uint64_t *total_throttles = args->total_throttles;
  uint64_t *total_unthrottles = args->total_unthrottles;

  cpu_set_t cpu_set;
  pthread_t thread;

  thread = pthread_self();
  CPU_ZERO(&cpu_set);
  CPU_SET(0, &cpu_set);

  int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpu_set);
  if (ret != 0) {
    perror("pthread_setaffinity_np");
  }

  struct perf_event_mmap_page *metadata_page = pebs_buffer;
  uint64_t data_head;
  uint64_t data_tail = metadata_page->data_tail;
  char *pbuf = (char *)metadata_page + metadata_page->data_offset;

  while (!(*stop_thread)) {
    __sync_synchronize();
    data_head = metadata_page->data_head; 

    while (data_tail != data_head) {
      struct perf_event_header *ph = (struct perf_event_header *) (pbuf + (data_tail % metadata_page->data_size));
      PerfSample* ps;

      switch (ph->type) {
        case PERF_RECORD_SAMPLE:
          ps = (PerfSample *) ph;
          assert(ps != NULL, "Perf sample record is null");

          (*total_pebs_samples)++;

          if (ps->addr == 0) {
            (*zero_addr_samples)++;
            break;
          }

          if ((char *) ps->addr >= old_gen_start_addr && (char *) ps->addr < young_gen_start_addr) {
            (*old_gen_samples)++;
            break;
          }

          if ((char *) ps->addr >= young_gen_start_addr && (char *) ps->addr < young_gen_end_addr) {
            (*young_gen_samples)++;
            break;
          }

          break;

        case PERF_RECORD_THROTTLE:
          (*total_throttles)++;
          break;

        case PERF_RECORD_UNTHROTTLE:
          (*total_unthrottles)++;
          break;

        default:
          break;
      }

      data_tail += ph->size;
      metadata_page->data_tail = data_tail;
    }
  }
  return NULL;
}

void TeraPebs::init_pebs(char *old_gen_start, char *young_gen_start,
                   char *young_gen_end, int sample_period, bool load_ops) {
  printf("Start: Seeting up intel PEBS\n");
  
  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  // pe.config = load_ops ? 0x81d0 : 0x82d0;
  pe.config = load_ops ? 0x20d1 : 0x82d0;
  pe.disabled = 0;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.exclude_callchain_kernel = 1;
  pe.exclude_callchain_user = 1;
  pe.precise_ip = 1;
  pe.sample_period = sample_period;
  pe.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR | PERF_SAMPLE_DATA_SRC;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    perror("perf_event_open");
    exit(EXIT_FAILURE);
  }
  
  mmap_size = sysconf(_SC_PAGESIZE) * PERF_PAGES;
  pebs_buffer = (struct perf_event_mmap_page *) mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if(pebs_buffer == MAP_FAILED) {
    perror("mmap");
    close(fd);
    exit(EXIT_FAILURE);
  }

  old_gen_start_addr = old_gen_start;
  young_gen_start_addr = young_gen_start;
  young_gen_end_addr = young_gen_end;

  //args = (struct PebsArgs *)os::malloc(sizeof(struct PebsArgs), mtInternal);
  args = NEW_C_HEAP_OBJ(struct PebsArgs, mtInternal);

  args->pebs_buffer = pebs_buffer;
  args->stop_thread = &stop_thread;
  args->old_gen_start_addr = old_gen_start_addr;
  args->young_gen_start_addr = young_gen_start_addr;
  args->young_gen_end_addr = young_gen_end_addr;
  args->total_pebs_samples = &total_pebs_samples;
  args->total_young_gen_samples = &total_young_gen_samples;
  args->total_old_gen_samples = &total_old_gen_samples;
  args->total_zero_addr_samples = &total_zero_addr_samples;
  args->total_throttles = &total_throttles;
  args->total_unthrottles = &total_unthrottles;

  int ret = pthread_create(&scan_thread, NULL, pebs_scan_thread, args);

  if (ret != 0) {
    perror("pthread_create");
    close(fd);
    munmap(pebs_buffer, mmap_size);
    exit(EXIT_FAILURE);
  }
}

// This constructor is used without pebs. Just to counter the total
// loads and stores instructions.
void TeraPebs::init_perf(bool load_ops) {
  printf("Setups pebs: start\n");

  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  // pe.config = load_ops ? 0x81d0 : 0x82d0;
  pe.config = load_ops ? 0x20d1 : 0x82d0;
  pe.disabled = 0;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    perror("perf_event_open");
    exit(EXIT_FAILURE);
  }

  printf("Setups pebs: finished\n");
}

TeraPebs::~TeraPebs(void) {
  if (args != nullptr)
    FREE_C_HEAP_OBJ(args);

  close(fd);

  if (pebs_buffer != nullptr)
    munmap(pebs_buffer, mmap_size);
}

void TeraPebs::start_perf(void) {
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

void TeraPebs::stop_perf(void) {
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
}

void TeraPebs::stop_pebs_scanner_thread(void) {
  stop_thread = 1;
  pthread_join(scan_thread, NULL);
}

void TeraPebs::print_pebs_statistics(void) {
  fprintf(stderr, "---------------\n");
  fprintf(stderr, "Perf. Statistics\n");
  fprintf(stderr, "---------------\n");

  fprintf(stderr, "Pebs Samples: %lu\n", total_pebs_samples);
  fprintf(stderr, "Young Gen. Samples: %lu\n", total_young_gen_samples);
  fprintf(stderr, "Old Gen. Samples: %lu\n", total_old_gen_samples);
  fprintf(stderr, "Zero Addr Samples: %lu\n", total_zero_addr_samples);
  uint64_t other_samples = total_pebs_samples - (total_young_gen_samples + total_old_gen_samples + total_zero_addr_samples);
  fprintf(stderr, "Irrelevant Samples: %lu\n", other_samples);
  fprintf(stderr, "Throttle Samples: %lu\n", total_throttles);
  fprintf(stderr, "Total_unthrottles Samples: %lu\n", total_unthrottles);
}

void TeraPebs::print_total_loads() {
  long long count = 0;
  ssize_t bytesRead = read(fd, &count, sizeof(long long));

  if (bytesRead == -1 || bytesRead != sizeof(long long)) {
    perror("Error reading file");
    exit(EXIT_FAILURE);
  }

  total_loads += count;

  fprintf(stderr, "Total Loads : %lu\n", total_loads);
}
