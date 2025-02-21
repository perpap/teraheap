#include "teraWritePolicy.hpp"

void DefaultWritePolicy::h2_write(char *data, char *offset, size_t size, uint64_t worker_id) const {
    memcpy(offset, data, size * 8);
}

void FmapWritePolicy::h2_write(char *data, char *offset, size_t size, uint64_t worker_id) const {
    r_write(data, offset, size, worker_id);
}

void SyncWritePolicy::h2_write(char *data, char *offset, size_t size, uint64_t worker_id) const {
    r_write(data, offset, size, worker_id);
}

void AsyncWritePolicy::h2_write(char *data, char *offset, size_t size, uint64_t worker_id) const {
    r_awrite(data, offset, size, worker_id);
}

void DefaultWritePolicy::h2_complete_transfers() const {}

void DefaultWritePolicy::h2_complete_transfers(uint worker_id) const {}

void FmapWritePolicy::h2_complete_transfers() const {
    r_fsync();
}

void FmapWritePolicy::h2_complete_transfers(uint worker_id) const {
    r_fsync();
}

void SyncWritePolicy::h2_complete_transfers() const {}
void SyncWritePolicy::h2_complete_transfers(uint worker_id) const {}

void AsyncWritePolicy::h2_complete_transfers() const {
    while(!r_areq_completed());
}

void AsyncWritePolicy::h2_complete_transfers(uint worker_id) const {
    while(!r_areq_completed_parallel(worker_id));
}
