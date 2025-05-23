/*
 * Copyright (c) 1997, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_OOPS_OOP_INLINE_HPP
#define SHARE_OOPS_OOP_INLINE_HPP

#include "oops/oop.hpp"

#include "memory/universe.hpp"
#include "memory/sharedDefines.h"
#include "oops/access.inline.hpp"
#include "oops/arrayKlass.hpp"
#include "oops/arrayOop.hpp"
#include "oops/compressedOops.inline.hpp"
#include "oops/markWord.inline.hpp"
#include "oops/oopsHierarchy.hpp"
#include "runtime/atomic.hpp"
#include "runtime/globals.hpp"
#include "utilities/align.hpp"
#include "utilities/debug.hpp"
#include "utilities/macros.hpp"
#include "utilities/globalDefinitions.hpp"

// Implementation of all inlined member functions defined in oop.hpp
// We need a separate file to avoid circular references

markWord oopDesc::mark() const {
  uintptr_t v = HeapAccess<MO_RELAXED>::load_at(as_oop(), mark_offset_in_bytes());
  return markWord(v);
}

markWord* oopDesc::mark_addr() const {
  return (markWord*) &_mark;
}

#ifdef TERA_FLAG
// Save the H2 destination address of the object. By saving the
// destination address to the teraflag we avoid to overwrite the
// mark word of the object
void oopDesc::set_h2_dst_addr(uint64_t addr) {
  _tera_flag = addr;
}

// Get the H2 destination address of the candidate object.
inline uint64_t oopDesc::get_h2_dst_addr() {
  return _tera_flag;
}
// Mark an object with 'id' to be moved in H2. H2 allocator uses the

// 'id' to locate objects with the same 'id' by to the same region.
// 'id' is defined by the application.
void oopDesc::mark_move_h2(uint64_t label, uint64_t sublabel) {
#ifdef TERA_ASSERT
  if(sublabel >= MAX_PARTITIONS){
    fprintf(stderr, "[DATASET_ERROR][%s|%s|%d] partId %" PRIu64 "is out of bounds; MAX_PARTITIONS = %" PRIu64 "\n", __FILE__, __func__, __LINE__, sublabel, MAX_PARTITIONS);
    exit(EXIT_FAILURE);
  }
#endif
  // Clear bits 48-63 and set them with the new value
  _tera_flag &= ~(0xFFFFULL << 48);
  _tera_flag |= (sublabel & 0xFFFFULL) << 48;

  // Clear bits 32-47 and set them with the new value
  _tera_flag &= ~(0xFFFFULL << 32);
  _tera_flag |= (label & 0xFFFFULL) << 32;
  
  // Clear bits 0-15th and the set with the new value
  _tera_flag &= ~((1ULL << 16) - 1);
  _tera_flag |= MOVE_TO_TERA;
}

void oopDesc::mark_move_h2(void) {
  // Clear bits 0-15th
  _tera_flag &= ~((1ULL << 16) - 1);
  _tera_flag |= MOVE_TO_TERA;
}

// Check if an object is marked to be moved in H2
bool oopDesc::is_marked_move_h2() { 
  return (_tera_flag & 0xffff) == MOVE_TO_TERA ;
}

// Mark this object that is located in H2 - TeraHeap
void oopDesc::set_in_h2() { 
  // Clear bits 0-15th
  _tera_flag &= ~((1ULL << 16) - 1);
  _tera_flag |= IN_TERA_HEAP;
}

// Get the state of the object
uint64_t oopDesc::get_obj_state() { 
  return (_tera_flag & 0xffff);
}

// Init the object state 
void oopDesc::init_obj_state() { 
  _tera_flag = INIT_TF;
}

// Get the object group id
int oopDesc::get_obj_group_id() {
  return (_tera_flag >> 32) & 0xffff;
}

// Get object partition Id
uint64_t oopDesc::get_obj_part_id() {
  return (_tera_flag >> 48) & 0xffff;
}

// This function is used only for statistic purposes to count how
// many objects in H2 are alive and how many are dead. 
// Return true if the objects in H2 is live
//        false otherwise
bool oopDesc::is_live() {
  uint64_t state = _tera_flag & 0xffff;
  return (state == LIVE_TERA_OBJ || state == VISITED_TERA_OBJ || state == MOVE_TO_TERA);
}

// This function is used only for statistic purposes to count how
// many objects in H2 are alive and how many are dead. 
// We reset the object state for the next major GC.
void oopDesc::reset_live() {
  set_in_h2();
}

// This function is used only for statistic purposes to count how
// many objects in H2 are alive and how many are dead. 
// This funtion set the teraflag to indicate that the object is live.
void oopDesc::set_live() {
  // Clear bits 0-15th
  _tera_flag &= ~((1ULL << 16) - 1);
  _tera_flag |= LIVE_TERA_OBJ;
}

// This function is used only for statistic purposes to count how many
// objects in H2 are alive and how many are dead. This funtion set the
// teraflag to indicate that the object is visited during marking
// phase.
void oopDesc::set_visited() {
  // Clear bits 0-15th
  _tera_flag &= ~((1ULL << 16) - 1);
  _tera_flag |= VISITED_TERA_OBJ;
}

// This function is used only for statistic purposes to count how many
// objects in H2 are alive and how many are dead. 
// Return true if the object is visited
//        false otherwise
bool oopDesc::is_visited() {
  uint64_t state = _tera_flag & 0xffff;
  return (state == VISITED_TERA_OBJ);
}

// Set object flag if is promitive array or leaf object. Leaf
// objects are the objects that contain only primitive fields and no
// references to other objects
void oopDesc::set_primitive(bool is_primitive_array) {
  // Clear the 16th and 17th bits
  _tera_flag &= ~(0x3ULL << 16);
  _tera_flag |= (is_primitive_array) ? (1ULL << 17) : (1ULL << 16);
}

// Set object flag if is a non primitive object
void oopDesc::set_non_primitive() {
  // Clear the 16th and 17th bits
  _tera_flag &= ~(0x3ULL << 16);
}
  
  // Check if the object is primitive array or leaf object. Leaf
  // objects are the objects that contain only primitive fields and no
  // references to other objects
bool oopDesc::is_primitive() {
  // Check if either the 16th or 17th bit is set
  return (_tera_flag & ((1ULL << 16) | (1ULL << 17))) != 0;
}

// Check if the object is non-primitive. 
bool oopDesc::is_non_primitive() {
  // Check if both the 16th or 17th bit are set
  return (_tera_flag & ((1ULL << 16) | (1ULL << 17))) == 0;
}

// Increase object writeness frequency
void oopDesc::incr_writness() {
  // Increase the writeness of an object
  uint64_t counter = (_tera_flag >> 20) & 0xFFF;
  counter++;

  // Clear bits 20-31 in _tera_flag
  _tera_flag &= ~(0xFFFULL << 20);

  // Set the updated counter in bits 20-31
  _tera_flag |= (counter & 0xFFFULL) << 20;
}
  
// Increase object age. This is used only for objects in the old
// generation
void oopDesc::incr_oldgen_obj_age() {
  uint64_t age = (_tera_flag >> 32) & 0xFFFF;
  age++;

  // Clear bits 20-31 in _tera_flag
  _tera_flag &= ~(0xFFFFULL << 32);

  // Set the updated counter in bits 20-31
  _tera_flag |= (age & 0xFFFULL) << 32;
}
  
// Enable the class bit to indicate that this objects is in the
// closure of an instance mirror class (class objects)
void oopDesc::set_instance_mirror_klass_ref() {
  // Set the 18th bit
  _tera_flag |= (1ULL << 18);
}

// Check if this object belongs to the transitive closure of an instance
// mirror class object (class objects)
bool oopDesc::is_instance_mirror_klass_ref() {
  // Check if the 18th bit is set
  return (_tera_flag & (1ULL << 18)) != 0;
}

#endif // TERA_FLAG

void oopDesc::set_mark(markWord m) {
  HeapAccess<MO_RELAXED>::store_at(as_oop(), mark_offset_in_bytes(), m.value());
}

void oopDesc::set_mark(HeapWord* mem, markWord m) {
  *(markWord*)(((char*)mem) + mark_offset_in_bytes()) = m;
}

void oopDesc::release_set_mark(markWord m) {
  HeapAccess<MO_RELEASE>::store_at(as_oop(), mark_offset_in_bytes(), m.value());
}

markWord oopDesc::cas_set_mark(markWord new_mark, markWord old_mark) {
  uintptr_t v = HeapAccess<>::atomic_cmpxchg_at(as_oop(), mark_offset_in_bytes(), old_mark.value(), new_mark.value());
  return markWord(v);
}

markWord oopDesc::cas_set_mark(markWord new_mark, markWord old_mark, atomic_memory_order order) {
  return Atomic::cmpxchg(&_mark, old_mark, new_mark, order);
}

void oopDesc::init_mark() {
  set_mark(markWord::prototype_for_klass(klass()));
}

Klass* oopDesc::klass() const {
  if (UseCompressedClassPointers) {
    return CompressedKlassPointers::decode_not_null(_metadata._compressed_klass);
  } else {
    return _metadata._klass;
  }
}

Klass* oopDesc::klass_or_null() const {
  if (UseCompressedClassPointers) {
    return CompressedKlassPointers::decode(_metadata._compressed_klass);
  } else {
    return _metadata._klass;
  }
}

Klass* oopDesc::klass_or_null_acquire() const {
  if (UseCompressedClassPointers) {
    narrowKlass nklass = Atomic::load_acquire(&_metadata._compressed_klass);
    return CompressedKlassPointers::decode(nklass);
  } else {
    return Atomic::load_acquire(&_metadata._klass);
  }
}

void oopDesc::set_klass(Klass* k) {
  assert(Universe::is_bootstrapping() || (k != NULL && k->is_klass()), "incorrect Klass");
  if (UseCompressedClassPointers) {
    _metadata._compressed_klass = CompressedKlassPointers::encode_not_null(k);
  } else {
    _metadata._klass = k;
  }
}

void oopDesc::release_set_klass(HeapWord* mem, Klass* k) {
  assert(Universe::is_bootstrapping() || (k != NULL && k->is_klass()), "incorrect Klass");
  char* raw_mem = ((char*)mem + klass_offset_in_bytes());
  if (UseCompressedClassPointers) {
    Atomic::release_store((narrowKlass*)raw_mem,
                          CompressedKlassPointers::encode_not_null(k));
  } else {
    Atomic::release_store((Klass**)raw_mem, k);
  }
}

int oopDesc::klass_gap() const {
  return *(int*)(((intptr_t)this) + klass_gap_offset_in_bytes());
}

void oopDesc::set_klass_gap(HeapWord* mem, int v) {
  if (UseCompressedClassPointers) {
    *(int*)(((char*)mem) + klass_gap_offset_in_bytes()) = v;
  }
}

void oopDesc::set_klass_gap(int v) {
  set_klass_gap((HeapWord*)this, v);
}

bool oopDesc::is_a(Klass* k) const {
  return klass()->is_subtype_of(k);
}

int oopDesc::size()  {
  return size_given_klass(klass());
}

int oopDesc::size_given_klass(Klass* klass)  {
  int lh = klass->layout_helper();
  int s;

  // lh is now a value computed at class initialization that may hint
  // at the size.  For instances, this is positive and equal to the
  // size.  For arrays, this is negative and provides log2 of the
  // array element size.  For other oops, it is zero and thus requires
  // a virtual call.
  //
  // We go to all this trouble because the size computation is at the
  // heart of phase 2 of mark-compaction, and called for every object,
  // alive or dead.  So the speed here is equal in importance to the
  // speed of allocation.

  if (lh > Klass::_lh_neutral_value) {
    if (!Klass::layout_helper_needs_slow_path(lh)) {
      s = lh >> LogHeapWordSize;  // deliver size scaled by wordSize
    } else {
      s = klass->oop_size(this);
    }
  } else if (lh <= Klass::_lh_neutral_value) {
    // The most common case is instances; fall through if so.
    if (lh < Klass::_lh_neutral_value) {
      // Second most common case is arrays.  We have to fetch the
      // length of the array, shift (multiply) it appropriately,
      // up to wordSize, add the header, and align to object size.
      size_t size_in_bytes;
      size_t array_length = (size_t) ((arrayOop)this)->length();
      size_in_bytes = array_length << Klass::layout_helper_log2_element_size(lh);
      size_in_bytes += Klass::layout_helper_header_size(lh);

      // This code could be simplified, but by keeping array_header_in_bytes
      // in units of bytes and doing it this way we can round up just once,
      // skipping the intermediate round to HeapWordSize.
      s = (int)(align_up(size_in_bytes, MinObjAlignmentInBytes) / HeapWordSize);

      // UseParallelGC and UseG1GC can change the length field
      // of an "old copy" of an object array in the young gen so it indicates
      // the grey portion of an already copied array. This will cause the first
      // disjunct below to fail if the two comparands are computed across such
      // a concurrent change.
      assert((s == klass->oop_size(this)) ||
             (Universe::is_gc_active() && is_objArray() && is_forwarded() && (get_UseParallelGC() || get_UseG1GC())),
             "wrong array object size");
    } else {
      // Must be zero, so bite the bullet and take the virtual call.
      s = klass->oop_size(this);
    }
  }

  assert(s > 0, "Oop size must be greater than zero, not %d", s);
  assert(is_object_aligned(s), "Oop size is not properly aligned: %d", s);
  return s;
}

bool oopDesc::is_instance()  const { return klass()->is_instance_klass();  }
bool oopDesc::is_array()     const { return klass()->is_array_klass();     }
bool oopDesc::is_objArray()  const { return klass()->is_objArray_klass();  }
bool oopDesc::is_typeArray() const { return klass()->is_typeArray_klass(); }

void*    oopDesc::field_addr(int offset)     const { return reinterpret_cast<void*>(cast_from_oop<intptr_t>(as_oop()) + offset); }

template <class T>
T*       oopDesc::obj_field_addr(int offset) const { return (T*) field_addr(offset); }

template <typename T>
size_t   oopDesc::field_offset(T* p) const { return pointer_delta((void*)p, (void*)this, 1); }

template <DecoratorSet decorators>
inline oop  oopDesc::obj_field_access(int offset) const             { return HeapAccess<decorators>::oop_load_at(as_oop(), offset); }
inline oop  oopDesc::obj_field(int offset) const                    { return HeapAccess<>::oop_load_at(as_oop(), offset);  }

inline void oopDesc::obj_field_put(int offset, oop value)           { HeapAccess<>::oop_store_at(as_oop(), offset, value); }

inline jbyte oopDesc::byte_field(int offset) const                  { return HeapAccess<>::load_at(as_oop(), offset);  }
inline void  oopDesc::byte_field_put(int offset, jbyte value)       { HeapAccess<>::store_at(as_oop(), offset, value); }

inline jchar oopDesc::char_field(int offset) const                  { return HeapAccess<>::load_at(as_oop(), offset);  }
inline void  oopDesc::char_field_put(int offset, jchar value)       { HeapAccess<>::store_at(as_oop(), offset, value); }

inline jboolean oopDesc::bool_field(int offset) const               { return HeapAccess<>::load_at(as_oop(), offset); }
inline void     oopDesc::bool_field_put(int offset, jboolean value) { HeapAccess<>::store_at(as_oop(), offset, jboolean(value & 1)); }
inline jboolean oopDesc::bool_field_volatile(int offset) const      { return HeapAccess<MO_SEQ_CST>::load_at(as_oop(), offset); }
inline void     oopDesc::bool_field_put_volatile(int offset, jboolean value) { HeapAccess<MO_SEQ_CST>::store_at(as_oop(), offset, jboolean(value & 1)); }
inline jshort oopDesc::short_field(int offset) const                { return HeapAccess<>::load_at(as_oop(), offset);  }
inline void   oopDesc::short_field_put(int offset, jshort value)    { HeapAccess<>::store_at(as_oop(), offset, value); }

inline jint oopDesc::int_field(int offset) const                    { return HeapAccess<>::load_at(as_oop(), offset);  }
inline jint oopDesc::int_field_raw(int offset) const                { return RawAccess<>::load_at(as_oop(), offset);   }
inline void oopDesc::int_field_put(int offset, jint value)          { HeapAccess<>::store_at(as_oop(), offset, value); }

inline jlong oopDesc::long_field(int offset) const                  { return HeapAccess<>::load_at(as_oop(), offset);  }
inline void  oopDesc::long_field_put(int offset, jlong value)       { HeapAccess<>::store_at(as_oop(), offset, value); }

inline jfloat oopDesc::float_field(int offset) const                { return HeapAccess<>::load_at(as_oop(), offset);  }
inline void   oopDesc::float_field_put(int offset, jfloat value)    { HeapAccess<>::store_at(as_oop(), offset, value); }

inline jdouble oopDesc::double_field(int offset) const              { return HeapAccess<>::load_at(as_oop(), offset);  }
inline void    oopDesc::double_field_put(int offset, jdouble value) { HeapAccess<>::store_at(as_oop(), offset, value); }

bool oopDesc::is_locked() const {
  return mark().is_locked();
}

bool oopDesc::is_unlocked() const {
  return mark().is_unlocked();
}

bool oopDesc::has_bias_pattern() const {
  return mark().has_bias_pattern();
}

// Used only for markSweep, scavenging
bool oopDesc::is_gc_marked() const {
  return mark().is_marked();
}

// Used by scavengers
bool oopDesc::is_forwarded() const {
  // The extra heap check is needed since the obj might be locked, in which case the
  // mark would point to a stack location and have the sentinel bit cleared
  return mark().is_marked();
}

// Used by scavengers
void oopDesc::forward_to(oop p) {
  verify_forwardee(p);
  markWord m = markWord::encode_pointer_as_mark(p);
  assert(m.decode_pointer() == p, "encoding must be reversable");
  set_mark(m);
}

// Used by parallel scavengers
bool oopDesc::cas_forward_to(oop p, markWord compare, atomic_memory_order order) {
  verify_forwardee(p);
  markWord m = markWord::encode_pointer_as_mark(p);
  assert(m.decode_pointer() == p, "encoding must be reversable");
  return cas_set_mark(m, compare, order) == compare;
}

oop oopDesc::forward_to_atomic(oop p, markWord compare, atomic_memory_order order) {
  verify_forwardee(p);
  markWord m = markWord::encode_pointer_as_mark(p);
  assert(m.decode_pointer() == p, "encoding must be reversable");
  markWord old_mark = cas_set_mark(m, compare, order);
  if (old_mark == compare) {
    return NULL;
  } else {
    return cast_to_oop(old_mark.decode_pointer());
  }
}

// Note that the forwardee is not the same thing as the displaced_mark.
// The forwardee is used when copying during scavenge and mark-sweep.
// It does need to clear the low two locking- and GC-related bits.
oop oopDesc::forwardee() const {
  return cast_to_oop(mark().decode_pointer());
}

// The following method needs to be MT safe.
uint oopDesc::age() const {
  assert(!is_forwarded(), "Attempt to read age from forwarded mark");
  if (has_displaced_mark()) {
    return displaced_mark().age();
  } else {
    return mark().age();
  }
}

void oopDesc::incr_age() {
  assert(!is_forwarded(), "Attempt to increment age of forwarded mark");
  if (has_displaced_mark()) {
    set_displaced_mark(displaced_mark().incr_age());
  } else {
    set_mark(mark().incr_age());
  }
}

template <typename OopClosureType>
void oopDesc::oop_iterate(OopClosureType* cl) {
  OopIteratorClosureDispatch::oop_oop_iterate(cl, this, klass());
}

template <typename OopClosureType>
void oopDesc::oop_iterate(OopClosureType* cl, MemRegion mr) {
  OopIteratorClosureDispatch::oop_oop_iterate(cl, this, klass(), mr);
}

template <typename OopClosureType>
int oopDesc::oop_iterate_size(OopClosureType* cl) {
  Klass* k = klass();
  int size = size_given_klass(k);
  OopIteratorClosureDispatch::oop_oop_iterate(cl, this, k);
  return size;
}

template <typename OopClosureType>
int oopDesc::oop_iterate_size(OopClosureType* cl, MemRegion mr) {
  Klass* k = klass();
  int size = size_given_klass(k);
  OopIteratorClosureDispatch::oop_oop_iterate(cl, this, k, mr);
  return size;
}

template <typename OopClosureType>
void oopDesc::oop_iterate_backwards(OopClosureType* cl) {
  oop_iterate_backwards(cl, klass());
}

template <typename OopClosureType>
void oopDesc::oop_iterate_backwards(OopClosureType* cl, Klass* k) {
  assert(k == klass(), "wrong klass");
  OopIteratorClosureDispatch::oop_oop_iterate_backwards(cl, this, k);
}

bool oopDesc::is_instanceof_or_null(oop obj, Klass* klass) {
  return obj == NULL || obj->klass()->is_subtype_of(klass);
}

intptr_t oopDesc::identity_hash() {
  // Fast case; if the object is unlocked and the hash value is set, no locking is needed
  // Note: The mark must be read into local variable to avoid concurrent updates.
  markWord mrk = mark();
  if (mrk.is_unlocked() && !mrk.has_no_hash()) {
    return mrk.hash();
  } else if (mrk.is_marked()) {
    return mrk.hash();
  } else {
    return slow_identity_hash();
  }
}

bool oopDesc::has_displaced_mark() const {
  return mark().has_displaced_mark_helper();
}

markWord oopDesc::displaced_mark() const {
  return mark().displaced_mark_helper();
}

void oopDesc::set_displaced_mark(markWord m) {
  mark().set_displaced_mark_helper(m);
}

bool oopDesc::mark_must_be_preserved() const {
  return mark_must_be_preserved(mark());
}

bool oopDesc::mark_must_be_preserved(markWord m) const {
  return m.must_be_preserved(this);
}

bool oopDesc::mark_must_be_preserved_for_promotion_failure(markWord m) const {
  return m.must_be_preserved_for_promotion_failure(this);
}

#endif // SHARE_OOPS_OOP_INLINE_HPP
