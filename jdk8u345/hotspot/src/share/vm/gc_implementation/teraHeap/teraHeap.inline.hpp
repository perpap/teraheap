#include "gc_implementation/teraHeap/teraHeap.hpp"

// The state machine uses this function to set if the GC should
// shrink H1 as a result to free the physical pages. Then the OS
// will reclaim the physical pahges and will use them as part of the
// buffer cache.
inline void TeraHeap::set_shrink_h1() {
  shrink_h1 = true;
}
  
// Reinitalize the shrink_h1 flag
inline void TeraHeap::unset_shrink_h1() {
  shrink_h1 = false;
}
  
// Check if the state machine identifies that we need to shrink H1.
inline bool  TeraHeap::need_to_shink_h1() {
  return shrink_h1;
}
  
// The state machine uses this function to set if the GC should
// grow H1.
inline void TeraHeap::set_grow_h1() {
  grow_h1 = true;
}
// Reinitalize the grow_h1 flag to the default value
inline void TeraHeap::unset_grow_h1() {
  grow_h1 = false;
}
// Check if the state machine identifies that we need to grow H1.
inline bool TeraHeap::need_to_grow_h1() {
  return grow_h1;
}

// Timers of TeraHeap to be used for breakdowns
inline TeraTimers* TeraHeap::get_tera_timers() {
  return tera_timers;
}

// Tera statistics for objects that we move to H2, forward references,
// and backward references.
inline TeraStatistics* TeraHeap::get_tera_stats() {
  return tera_stats;
}
  
inline void TeraHeap::set_direct_promotion() {
  direct_promotion = true;
}
  
inline void TeraHeap::unset_direct_promotion() {
  direct_promotion = false;
}

inline TeraDynamicResizingPolicy* TeraHeap::get_resizing_policy() {
  return dynamic_resizing_policy;
}

// Traverse the fields of a class object. These fields correspond to
// the transitive closure of a class object
inline void TeraHeap::enable_traverse_class_object_field() {
  traverse_class_object_field = true;
}

inline void TeraHeap::disable_traverse_class_object_field() {
  traverse_class_object_field = false;
}

inline bool TeraHeap::is_traverse_class_object_field() {
  return traverse_class_object_field;
}
  
// Reset object ref field flag
inline void TeraHeap::reset_obj_ref_field_flag() {
  traced_obj_has_ref_field = false;
}

// Enable the flag if the object has reference fields
inline void TeraHeap::set_obj_ref_field_flag() {
  traced_obj_has_ref_field = true;
}
