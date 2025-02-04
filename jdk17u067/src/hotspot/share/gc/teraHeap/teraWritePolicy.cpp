#include "teraWritePolicy.hpp"

void DefaultWritePolicy::h2_write(char *data, char *offset, size_t size) const {
    memcpy(offset, data, size * 8);
}

void FmapWritePolicy::h2_write(char *data, char *offset, size_t size) const {
    r_write(data, offset, size);
}

void SyncWritePolicy::h2_write(char *data, char *offset, size_t size) const {
    r_write(data, offset, size);
}

void AsyncWritePolicy::h2_write(char *data, char *offset, size_t size) const {
    r_awrite(data, offset, size);
}

void DefaultWritePolicy::h2_complete_transfers() const {
}

void FmapWritePolicy::h2_complete_transfers() const {
    r_fsync();
}

void SyncWritePolicy::h2_complete_transfers() const {
}

void AsyncWritePolicy::h2_complete_transfers() const {
    while(!r_areq_completed());
}
