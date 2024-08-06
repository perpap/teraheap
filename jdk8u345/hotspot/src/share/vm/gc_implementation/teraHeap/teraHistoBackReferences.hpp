#ifndef SHARE_VM_GC_IMPLEMENTATION_TERAHEAP_TERAHISTOBACKREFERENCES_HPP
#define SHARE_VM_GC_IMPLEMENTATION_TERAHEAP_TERAHISTOBACKREFERENCES_HPP

//#include "memory/allocation.hpp"
//#include "oops/oop.hpp"
//
//#include <map>
//#include <tr1/tuple>
//
//class TeraHistoBackReferences: public CHeapObj<mtInternal> {
//private:
//  // This histogram keeps internally statistics for the backward
//  // references (H2 to H1)
//  std::map<oop *, std::tr1::tuple<int, int, int> > histogram;
//  oop *back_ref_obj;
//
//public:
//  // Add a new entry to the histogram for back reference that start from
//  // 'obj' and results in H1 (new or old generation).
//  // Use this function with a single GC thread
//  void update_back_ref_stats(bool is_in_old_gen, bool is_in_h2);
//
//  void enable_back_ref_traversal(oop *obj);
//
//  // Print the histogram
//  void print_back_ref_stats();
//};

#endif // SHARE_VM_GC_IMPLEMENTATION_TERAHEAP_TERAHISTOBACKREFERENCES_HPP
