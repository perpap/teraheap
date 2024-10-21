#include "gc/teraHeap/teraTimers.hpp"
#include "gc/shared/gc_globals.hpp"
#include "gc/shared/cycleCounting.hpp"
#include "runtime/java.hpp"

const uint64_t TeraTimers::CYCLES_PER_SECOND{get_cycles_per_second()};

void TeraTimers::print_ellapsed_time(struct timespec start_time,
		struct timespec end_time, const char *msg) {

	//double elapsed_time = (double)(end_time - start_time) / CYCLES_PER_SECOND;
	//double elapsed_time_ms = elapsed_time * 1000.0;
        // Calculate the time difference in milliseconds
        double elapsed_time_ms = (double)((end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec)) / 1000000L;
	thlog_or_tty->print_cr("[STATISTICS] | %s %f\n", msg, elapsed_time_ms);
}

TeraTimers::TeraTimers(){

  if (!TeraHeapStatistics)
    vm_exit_during_initialization("Enable -XX:+TeraHeapStatistics");

	h1_card_table_start_time = NEW_C_HEAP_ARRAY(/*uint64_t*/struct timespec, ParallelGCThreads, mtGC);
	h1_card_table_end_time = NEW_C_HEAP_ARRAY(/*uint64_t*/struct timespec, ParallelGCThreads, mtGC);
	h2_card_table_start_time = NEW_C_HEAP_ARRAY(/*uint64_t*/struct timespec, ParallelGCThreads, mtGC);
	h2_card_table_end_time = NEW_C_HEAP_ARRAY(/*uint64_t*/struct timespec, ParallelGCThreads, mtGC);

	malloc_time_per_gc = 0;
};

TeraTimers::~TeraTimers() {
	FREE_C_HEAP_ARRAY(/*uint64_t*/struct timespec, h1_card_table_start_time);
	FREE_C_HEAP_ARRAY(/*uint64_t*/struct timespec, h1_card_table_end_time);
	FREE_C_HEAP_ARRAY(/*uint64_t*/struct timespec, h2_card_table_start_time);
	FREE_C_HEAP_ARRAY(/*uint64_t*/struct timespec, h2_card_table_end_time);
}

void TeraTimers::h2_scavenge_start() {
	//h2_scavenge_start_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_scavenge_start_time);
}

void TeraTimers::h2_scavenge_end() {
	char msg[12] = "H2_SCAVENGE";

	//h2_scavenge_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_scavenge_end_time);
	print_ellapsed_time(h2_scavenge_start_time, h2_scavenge_end_time, msg);
}

void TeraTimers::h1_marking_phase_start() {
	//h1_marking_phase_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h1_marking_phase_start_time);
}

void TeraTimers::h1_marking_phase_end() {
	char msg[17] = "H1_MARKING_PHASE";

	//h1_marking_phase_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h1_marking_phase_end_time);
	print_ellapsed_time(h1_marking_phase_start_time, h1_marking_phase_end_time, msg);
}

void TeraTimers::h2_mark_bwd_ref_start() {
	//h2_mark_bwd_ref_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h2_mark_bwd_ref_start_time);
}

void TeraTimers::h2_mark_bwd_ref_end() {
	char msg[19] = "H2_MARKING_BWD_REF";

	//h2_mark_bwd_ref_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_mark_bwd_ref_end_time);
	print_ellapsed_time(h2_mark_bwd_ref_start_time, h2_mark_bwd_ref_end_time, msg);
}

void TeraTimers::h2_precompact_start() {
	//h2_precompact_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h2_precompact_start_time);
}

void TeraTimers::h2_precompact_end() {
	char msg[14] = "H2_PRECOMPACT";

	//h2_precompact_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_precompact_end_time);
	print_ellapsed_time(h2_precompact_start_time, h2_precompact_end_time, msg);
}

void TeraTimers::h1_summary_phase_start() {
	//h1_summary_phase_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h1_summary_phase_start_time);
}

void TeraTimers::h1_summary_phase_end() {
	char msg[17] = "H1_SUMMARY_PHASE";

	//h1_summary_phase_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h1_summary_phase_end_time);
	print_ellapsed_time(h1_summary_phase_start_time, h1_summary_phase_end_time, msg);
}

void TeraTimers::h2_compact_start() {
	//h2_compact_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h2_compact_start_time);
}

void TeraTimers::h2_compact_end() {
	char msg[17] = "H2_COMPACT_PHASE";

	//h2_compact_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_compact_end_time);
	print_ellapsed_time(h2_compact_start_time, h2_compact_end_time, msg);
}

void TeraTimers::h2_adjust_bwd_ref_start() {
	//h2_adjust_bwd_ref_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h2_adjust_bwd_ref_start_time);
}

void TeraTimers::h2_adjust_bwd_ref_end() {
	char msg[18] = "H2_ADJUST_BWD_REF";

	//h2_adjust_bwd_ref_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_adjust_bwd_ref_end_time);
	print_ellapsed_time(h2_adjust_bwd_ref_start_time, h2_adjust_bwd_ref_end_time, msg);
}

void TeraTimers::h1_adjust_roots_start() {
	//h1_adjust_roots_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h1_adjust_roots_start_time);
}

void TeraTimers::h1_adjust_roots_end() {
	char msg[16] = "H1_ADJUST_ROOTS";
	//h1_adjust_roots_end_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h1_adjust_roots_end_time);
	print_ellapsed_time(h1_adjust_roots_start_time, h1_adjust_roots_end_time, msg);
}

void TeraTimers::h1_compact_start() {
	//h1_compact_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h1_compact_start_time);
}

void TeraTimers::h1_compact_end() {
	char msg[11] = "H1_COMPACT";

	//h1_compact_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h1_compact_end_time);
	print_ellapsed_time(h1_compact_start_time, h1_compact_end_time, msg);
}

void TeraTimers::h2_clear_fwd_table_start() {
	//h2_clear_fwd_table_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h2_clear_fwd_table_start_time);
}

void TeraTimers::h2_clear_fwd_table_end() {
	char msg[19] = "H2_CLEAR_FWD_TABLE";

	//h2_clear_fwd_table_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_clear_fwd_table_end_time);
	print_ellapsed_time(h2_clear_fwd_table_start_time, h2_clear_fwd_table_end_time, msg);
}

// Keep for each GC thread the time that need to traverse the H1
// card table.
// Each thread writes the time in a table based on their ID and then we
// take the maximum time from all the threads as the total time.
void TeraTimers::h1_card_table_start(unsigned int worker_id) {
	assert(worker_id < ParallelGCThreads, "Index out of bound");
	//h1_card_table_start_time[worker_id] = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h1_card_table_start_time[worker_id]);
}

void TeraTimers::h1_card_table_end(unsigned int worker_id) {
	assert(worker_id < ParallelGCThreads, "Index out of bound");
	//h1_card_table_end_time[worker_id] = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h1_card_table_end_time[worker_id]);
}

// Keep for each GC thread the time that need to traverse the H2
// card table.
// Each thread writes the time in a table based on each ID and then we
// take the maximum time from all the threads as the total time.
void TeraTimers::h2_card_table_start(unsigned int worker_id) {
	assert(worker_id < ParallelGCThreads, "Index out of bound");
	//h2_card_table_start_time[worker_id] = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &h2_card_table_start_time[worker_id]);
}

void TeraTimers::h2_card_table_end(unsigned int worker_id) {
	assert(worker_id < ParallelGCThreads, "Index out of bound");
	//h2_card_table_end_time[worker_id] = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &h2_card_table_end_time[worker_id]);
}

// Print the time to traverse the TeraHeap dirty card tables
// and the time to traverse the Heap dirty card tables during minor
// GC.
void TeraTimers::print_card_table_scanning_time() {
	double h1_max_time = 0;
	double h2_max_time = 0;

	for (unsigned int i = 0; i < ParallelGCThreads; i++) {
		//double elapsed_time = (double)(h1_card_table_end_time[i] - h1_card_table_start_time[i]) / CYCLES_PER_SECOND;
		//double elapsed_time_ms = elapsed_time * 1000.0;
		double elapsed_time_ms = (double)((h1_card_table_end_time[i].tv_sec - h1_card_table_start_time[i].tv_sec) * 1000000000L + (h1_card_table_end_time[i].tv_nsec - h1_card_table_start_time[i].tv_nsec)) / 1000000L;
		if (h1_max_time < elapsed_time_ms)
			h1_max_time = elapsed_time_ms;

		//elapsed_time = (double)(h2_card_table_end_time[i] - h2_card_table_start_time[i]) / CYCLES_PER_SECOND;
		//elapsed_time_ms = elapsed_time * 1000.0;
                elapsed_time_ms = (double)((h2_card_table_end_time[i].tv_sec - h2_card_table_start_time[i].tv_sec) * 1000000000L + (h2_card_table_end_time[i].tv_nsec - h2_card_table_start_time[i].tv_nsec)) / 1000000L;
		if (h2_max_time < elapsed_time_ms)
			h2_max_time = elapsed_time_ms;
	}

	thlog_or_tty->print_cr("[STATISTICS] | H1_CT_TIME = %f\n", h1_max_time);
	thlog_or_tty->print_cr("[STATISTICS] | H2_CT_TIME = %f\n", h2_max_time);

	// Initialize arrays for the next minor collection
	memset(h1_card_table_start_time, 0, ParallelGCThreads * sizeof(/*uint64_t*/struct timespec));
	memset(h1_card_table_end_time, 0, ParallelGCThreads * sizeof(/*uint64_t*/struct timespec));
	memset(h2_card_table_start_time, 0, ParallelGCThreads * sizeof(/*uint64_t*/struct timespec));
	memset(h2_card_table_end_time, 0, ParallelGCThreads * sizeof(/*uint64_t*/struct timespec));
}

void TeraTimers::malloc_start() {
	//malloc_start_time = get_cycles();
        clock_gettime(CLOCK_MONOTONIC_RAW, &malloc_start_time);
}

void TeraTimers::malloc_end() {
	//malloc_end_time = get_cycles();
	clock_gettime(CLOCK_MONOTONIC_RAW, &malloc_end_time);
	//double elapsed_time = (double)(malloc_end_time - malloc_start_time) / CYCLES_PER_SECOND;
	//malloc_time_per_gc += (elapsed_time_ms * 1000.0);
	// Calculate the time difference in milliseconds
        double elapsed_time_ms = (double)((malloc_end_time.tv_sec - malloc_start_time.tv_sec) * 1000000000L + (malloc_end_time.tv_nsec - malloc_start_time.tv_nsec)) / 1000000L;
        malloc_time_per_gc +=(elapsed_time_ms);
}

void TeraTimers::print_malloc_time() {
	thlog_or_tty->print_cr("[STATISTICS] | MALLOC %f\n", malloc_time_per_gc);
	malloc_time_per_gc = 0;
}

//void TeraTimers::print_action_state() {
//  thlog_or_tty->stamp(true);
//  thlog_or_tty->print("");
//
//}
