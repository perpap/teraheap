#ifndef SHARE_GC_TERAHEAP_TERAWRITEPOLICY_HPP
#define SHARE_GC_TERAHEAP_TERAWRITEPOLICY_HPP

#include "memory/allocation.hpp"
//#include "oops/oopsHierarchy.hpp"
//#include "gc/shared/gc_globals.hpp"
#include <regions.h>

class WritePolicy : public CHeapObj<mtInternal> {
    public:
	virtual void h2_write(char *data, char *offset, size_t size) const = 0;
	virtual void h2_complete_transfers() const = 0;
};

class DefaultWritePolicy : public WritePolicy{
    public:
        virtual void h2_write(char *data, char *offset, size_t size) const override;
        virtual void h2_complete_transfers() const override;
};

class FmapWritePolicy : public WritePolicy{
    public:
        virtual void h2_write(char *data, char *offset, size_t size) const override;
        virtual void h2_complete_transfers() const override;
};

class SyncWritePolicy : public WritePolicy{
    public:
        virtual void h2_write(char *data, char *offset, size_t size) const override;
        virtual void h2_complete_transfers() const override;
};

class AsyncWritePolicy : public WritePolicy{
    public:
        virtual void h2_write(char *data, char *offset, size_t size) const override;
        virtual void h2_complete_transfers() const override;
};

#endif //SHARE_GC_TERAHEAP_TERAWRITEPOLICY_HPP
