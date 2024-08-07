#include "gc/flexHeap/flexStateMachine.hpp"
#include "gc/parallel/parallelScavengeHeap.hpp"

#define BUFFER_SIZE 1024
#define EPSILON 50

void FlexStateMachine::state_no_action(fh_states *cur_state, fh_actions *cur_action,
                                       double gc_time_ms, double io_time_ms) {

  if (abs(io_time_ms - gc_time_ms) <= EPSILON) {
    *cur_state = FHS_NO_ACTION;
    *cur_action = FH_NO_ACTION;
    return;
  }

  PSOldGen *old_gen = ParallelScavengeHeap::old_gen();
  bool under_h1_max_limit = old_gen->capacity_in_bytes() < old_gen->max_gen_size();
  if (gc_time_ms >= io_time_ms && under_h1_max_limit) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action = FH_GROW_HEAP;
    return;
  }

  if (io_time_ms > gc_time_ms) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_SHRINK_HEAP;
    return;
  }

  *cur_state = FHS_NO_ACTION;
  *cur_action = FH_NO_ACTION;
}

// Read the process anonymous memory
size_t FlexStateMachine::read_process_anon_memory() {
    // Open /proc/pid/stat file
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/stat", getpid());
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        perror("Error opening /proc/pid/stat");
        exit(EXIT_FAILURE);
    }

    // Read the contents of /proc/pid/stat into a buffer
    char buffer[BUFFER_SIZE];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        perror("Error reading /proc/pid/stat");
        exit(EXIT_FAILURE);
    }

    // Close the file
    fclose(fp);

    // Tokenize the buffer to extract RSS
    char *token = strtok(buffer, " ");
    for (int i = 1; i < 24; ++i) {
        token = strtok(NULL, " ");
        if (token == NULL) {
            fprintf(stderr, "Error tokenizing /proc/pid/stat\n");
            exit(EXIT_FAILURE);
        }
    }

    // Convert the token to a long int
    size_t rss = atol(token) * os::vm_page_size();

    return rss;
}

// Read the memory statistics for the cgroup
size_t FlexStateMachine::read_cgroup_mem_stats(bool read_page_cache) {
  // Define the path to the memory.stat file
  const char* file_path = "/sys/fs/cgroup/memory/memlim/memory.stat";

  // Open the file for reading
  FILE* file = fopen(file_path, "r");

  if (file == NULL) {
    fprintf(stderr, "Failed to open memory.stat\n");
    return 0;
  }

  char line[BUFFER_SIZE];
  size_t res = 0;

  // Read each line in the file
  while (fgets(line, sizeof(line), file)) {
    if (read_page_cache && strncmp(line, "cache", 5) == 0) {
      // Extract the cache value
      res = atoll(line + 6);
      break;
    }

    if (strncmp(line, "rss", 3) == 0) {
      // Extract the rss value
      res = atoll(line + 4);
      break;
    }
  }

  // Close the file
  fclose(file);
  return res;
}

void FlexSimpleStateMachine::fsm(fh_states *cur_state, fh_actions *cur_action,
                                 double gc_time_ms, double io_time_ms) {
  state_no_action(cur_state, cur_action, gc_time_ms, io_time_ms);
  *cur_state = FHS_NO_ACTION;
}

void FlexSimpleWaitStateMachine::fsm(fh_states *cur_state, fh_actions *cur_action,
                                     double gc_time_ms, double io_time_ms) {
  switch (*cur_state) {
    case FHS_WAIT_GROW:
      state_wait_after_grow(cur_state, cur_action, gc_time_ms, io_time_ms);
      break;
    case FHS_WAIT_SHRINK:
      state_wait_after_shrink(cur_state, cur_action, gc_time_ms, io_time_ms);
      break;
    case FHS_NO_ACTION:
      state_no_action(cur_state, cur_action, gc_time_ms, io_time_ms);
      break;
  }
}

void FlexSimpleWaitStateMachine::state_wait_after_grow(fh_states *cur_state, fh_actions *cur_action,
                                                       double gc_time_ms, double io_time_ms) {
  if (abs(io_time_ms - gc_time_ms) <= EPSILON) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action =  FH_WAIT_AFTER_GROW;
    return;
  }

  PSOldGen *old_gen = ParallelScavengeHeap::old_gen();
  size_t cur_size = old_gen->capacity_in_bytes();
  size_t used_size = old_gen->used_in_bytes();
  // Occupancy of the old generation is higher than 70%
  bool high_occupancy = (((double)(used_size) / cur_size) > 0.70);
  
  if ((gc_time_ms > io_time_ms) && !high_occupancy) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action = FH_WAIT_AFTER_GROW;
    return;
  }

  *cur_state = FHS_NO_ACTION;
  *cur_action = FH_NO_ACTION; // remember to change that for optimization in S_GROW_H1
}

void FlexSimpleWaitStateMachine::state_wait_after_shrink(fh_states *cur_state, fh_actions *cur_action,
                                                         double gc_time_ms, double io_time_ms) {
  if (abs(io_time_ms - gc_time_ms) <= EPSILON) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_IOSLACK;
    return;
  }

  size_t cur_rss = read_cgroup_mem_stats(false);
  size_t cur_cache = read_cgroup_mem_stats(true);
  bool ioslack = ((cur_rss + cur_cache) < (FlexDRAMLimit * 0.8));

  if (io_time_ms > gc_time_ms && ioslack) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_IOSLACK;
    return;
  }

  *cur_state = FHS_NO_ACTION;
  *cur_action = FH_NO_ACTION;
}

void FlexFullOptimizedStateMachine::state_wait_after_shrink(fh_states *cur_state, fh_actions *cur_action,
                                                            double gc_time_ms, double io_time_ms) {
  if (abs(io_time_ms - gc_time_ms) <= EPSILON) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_IOSLACK;
    return;
  }
  
  if (gc_time_ms > io_time_ms) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action = FH_GROW_HEAP;
    return;
  }
  
  //size_t cur_rss = read_cgroup_mem_stats(false);
  size_t cur_rss = read_process_anon_memory();
  size_t cur_cache = read_cgroup_mem_stats(true);
  bool ioslack = ((cur_rss + cur_cache) < (FlexDRAMLimit * 0.8));

  if (io_time_ms > gc_time_ms && !ioslack) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_SHRINK_HEAP;
    return;
  }
  
  if (io_time_ms > gc_time_ms && ioslack) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_IOSLACK;
    return;
  }
  
  *cur_state = FHS_NO_ACTION;
  *cur_action = FH_NO_ACTION;

}

void FlexFullOptimizedStateMachine::state_wait_after_grow(fh_states *cur_state, fh_actions *cur_action,
                                                          double gc_time_ms, double io_time_ms) {
  
  if (abs(io_time_ms - gc_time_ms) <= EPSILON) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action =  FH_WAIT_AFTER_GROW;
    return;
  }

  if (io_time_ms > gc_time_ms) {
    *cur_state = FHS_WAIT_SHRINK;
    *cur_action = FH_SHRINK_HEAP;
    return;
  }
  
  PSOldGen *old_gen = ParallelScavengeHeap::old_gen();
  size_t cur_size = old_gen->capacity_in_bytes();
  size_t used_size = old_gen->used_in_bytes();
  // Occupancy of the old generation is higher than 70%
  bool high_occupancy = (((double)(used_size) / cur_size) > 0.70);
  bool under_h1_max_limit = cur_size < old_gen->max_gen_size();
  
  if (gc_time_ms > io_time_ms && high_occupancy && under_h1_max_limit) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action = FH_GROW_HEAP;
    return;
  }

  if (gc_time_ms > io_time_ms && !high_occupancy && under_h1_max_limit) {
    *cur_state = FHS_WAIT_GROW;
    *cur_action = FH_WAIT_AFTER_GROW;
    return;
  }

  *cur_state = FHS_NO_ACTION;
  *cur_action = FH_NO_ACTION; // remember to change that for optimization in S_GROW_H1
}
