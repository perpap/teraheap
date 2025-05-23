/*
 * Copyright (c) 2000, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_GC_IMPLEMENTATION_SHARED_MARKSWEEP_INLINE_HPP
#define SHARE_VM_GC_IMPLEMENTATION_SHARED_MARKSWEEP_INLINE_HPP

#include "gc_implementation/shared/markSweep.hpp"
#include "gc_interface/collectedHeap.hpp"
#include "utilities/stack.inline.hpp"
#include "utilities/macros.hpp"
#if INCLUDE_ALL_GCS
#include "gc_implementation/g1/g1StringDedup.hpp"
#include "gc_implementation/parallelScavenge/psParallelCompact.hpp"
#include "gc_implementation/teraHeap/teraHeap.hpp"
#include "gc_implementation/teraHeap/teraHeap.inline.hpp"
#endif // INCLUDE_ALL_GCS

inline void MarkSweep::mark_object(oop obj) {
#if INCLUDE_ALL_GCS
  if (G1StringDedup::is_enabled()) {
    // We must enqueue the object before it is marked
    // as we otherwise can't read the object's age.
    G1StringDedup::enqueue_from_mark(obj);
  }
#endif
  // some marks may contain information we need to preserve so we store them away
  // and overwrite the mark.  We'll restore it at the end of markSweep.
  markOop mark = obj->mark();
  obj->set_mark(markOopDesc::prototype()->set_marked());

  if (mark->must_be_preserved(obj)) {
    preserve_mark(obj, mark);
  }
}

inline void MarkSweep::follow_klass(Klass* klass) {
  oop op = klass->klass_holder();
  MarkSweep::mark_and_push(&op);
}

template <class T> inline void MarkSweep::follow_root(T* p) {
  assert(!Universe::heap()->is_in_reserved(p),
         "roots shouldn't be things within the heap");
  T heap_oop = oopDesc::load_heap_oop(p);
  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);
    if (!obj->mark()->is_marked()) {
      mark_object(obj);
      obj->follow_contents();
    }
  }
  follow_stack();
}

#ifdef TERA_MAJOR_GC

template <class T> inline void MarkSweep::tera_back_ref_mark_and_push(T* p) {
  T heap_oop = oopDesc::load_heap_oop(p);
  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);

    if (EnableTeraHeap && Universe::teraHeap()->is_obj_in_h2(obj))
    {
      TeraHeap *th = Universe::teraHeap();
      // Mark H2 region as active
      th->mark_used_region((HeapWord*)obj);

      if (H2LivenessAnalysis)
        obj->set_live();

      if (TeraHeapStatistics)
        th->get_tera_stats()->add_forward_ref();
      return;
    }

#ifdef TEST_CLONE
		DEBUG_ONLY(if (EnableTeraHeap) { assert(obj->get_obj_state() == MOVE_TO_TERA || obj->get_obj_state() == INIT_TF, "Fix clone operation"); });
#endif

		if (!obj->mark()->is_marked()) {

			mark_object(obj);

			if (!(obj->is_marked_move_h2() || Universe::teraHeap()->is_metadata(obj))) {
				uint64_t groupId = Universe::teraHeap()->h2_get_region_groupId((void *) p);
				uint64_t partId = Universe::teraHeap()->h2_get_region_partId((void *) p);
				obj->mark_move_h2(groupId, partId);
//#if defined(HINT_HIGH_LOW_WATERMARK) || defined(NOHINT_HIGH_LOW_WATERMARK)
        // Universe::teraHeap()->h2_incr_total_marked_obj_size(obj->size());
        //Universe::teraHeap()->get_resizing_policy()->increase_h2_candidate_size(obj->size());
//#endif
			}

			_marking_stack.push(obj);
		}
	}
}

template <class T> inline void MarkSweep::tera_mark_and_push(T* p) {
  T heap_oop = oopDesc::load_heap_oop(p);
  TeraHeap *th = Universe::teraHeap();

#ifdef P_PRIMITIVE
    if (EnableTeraHeap)
      th->set_obj_ref_field_flag();
#endif

  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);

    if (EnableTeraHeap && (Universe::teraHeap()->is_obj_in_h2(obj)))
    {
      // Mark H2 region as active
      th->mark_used_region((HeapWord*)obj);

      if (H2LivenessAnalysis)
        obj->set_live();

      if (TeraHeapStatistics)
        th->get_tera_stats()->add_forward_ref();

      return;
    }

#ifdef TEST_CLONE
		DEBUG_ONLY(if (EnableTeraHeap) { assert(obj->get_obj_state() == MOVE_TO_TERA || obj->get_obj_state() == INIT_TF, "Fix clone operation"); });
#endif

#ifdef SPARK_POLICY
    if (!(obj->mark()->is_marked() && obj->is_marked_move_h2())) {
      if (!obj->mark()->is_marked())
        mark_object(obj);

      if (!obj->is_marked_move_h2())
        obj->mark_move_h2(Universe::teraHeap()->get_cur_obj_group_id(),
                            Universe::teraHeap()->get_cur_obj_part_id());

      _marking_stack.push(obj);
    }
#else
    if (!(obj->is_marked_move_h2() || Universe::teraHeap()->is_metadata(obj))) {
      obj->mark_move_h2(th->get_cur_obj_group_id(), th->get_cur_obj_part_id());
//#if defined(HINT_HIGH_LOW_WATERMARK) || defined(NOHINT_HIGH_LOW_WATERMARK)
//      //Universe::teraHeap()->h2_incr_total_marked_obj_size(obj->size());
//      //Universe::teraHeap()->get_resizing_policy()->increase_h2_candidate_size(obj->size());
//#endif
    }

    if (!obj->mark()->is_marked()) {
      mark_object(obj);
      _marking_stack.push(obj);
    }
#endif // SPARK_POLICY
  }
}

// This function is used for h2 liveness analysis. We detect the live
// objects in each H2 regions
template <class T> inline void MarkSweep::h2_liveness_analysis(T* p) {
  //oop obj = oop(p);
  T heap_oop = oopDesc::load_heap_oop(p);
  if (oopDesc::is_null(heap_oop))
    return;
  oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);
  if (!Universe::teraHeap()->is_obj_in_h2(obj))
    return;
  if (obj->is_visited())
    return;
  obj->set_visited();
  obj->h2_follow_contents();
}

#endif // TERA_MAJOR_GC

template <class T> inline void MarkSweep::mark_and_push(T* p) {
  T heap_oop = oopDesc::load_heap_oop(p);
#ifdef P_PRIMITIVE
    if (EnableTeraHeap)
      Universe::teraHeap()->set_obj_ref_field_flag();
#endif
  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);

#ifdef TERA_MAJOR_GC
		if (EnableTeraHeap && Universe::teraHeap()->is_obj_in_h2(obj))
		{
      //Mark active region
      Universe::teraHeap()->mark_used_region((HeapWord*)obj);

      if (H2LivenessAnalysis)
        obj->set_live();

      if (TeraHeapStatistics)
        Universe::teraHeap()->get_tera_stats()->add_forward_ref();

			return;
		}
#endif // TERA_MAJOR_GC

#ifdef TEST_CLONE
		DEBUG_ONLY(if (EnableTeraHeap) { assert(obj->get_obj_state() == MOVE_TO_TERA || obj->get_obj_state() == INIT_TF, "Fix clone operation"); });
#endif

    if (!obj->mark()->is_marked()) {
      mark_object(obj);
      _marking_stack.push(obj);
    }
  }
}

void MarkSweep::push_objarray(oop obj, size_t index) {
  ObjArrayTask task(obj, index);
  assert(task.is_valid(), "bad ObjArrayTask");
  _objarray_stack.push(task);
}

template <class T> inline void MarkSweep::adjust_pointer(T* p) {
  T heap_oop = oopDesc::load_heap_oop(p);
  if (!oopDesc::is_null(heap_oop)) {
    oop obj     = oopDesc::decode_heap_oop_not_null(heap_oop);
#ifdef TERA_MAJOR_GC
		oop new_obj = NULL;
        
		if (EnableTeraHeap && Universe::teraHeap()->is_obj_in_h2(obj))
			new_obj = obj;
		else 
			new_obj = oop(obj->mark()->decode_pointer());

		if (EnableTeraHeap)
			Universe::teraHeap()->group_region_enabled((HeapWord*) new_obj, (void *) p);
#else
		oop new_obj = oop(obj->mark()->decode_pointer());
#endif // TERA_MAJOR_GC
    assert(new_obj != NULL ||                         // is forwarding ptr?
           obj->mark() == markOopDesc::prototype() || // not gc marked?
           (UseBiasedLocking && obj->mark()->has_bias_pattern()),
                                                      // not gc marked?
           "should be forwarded");
    if (new_obj != NULL) {
      assert(Universe::heap()->is_in_reserved(new_obj)
             || Universe::teraHeap()->is_obj_in_h2(new_obj),
             "should be in object space");
      oopDesc::encode_store_heap_oop_not_null(p, new_obj);
    }
  }
}

template <class T> inline void MarkSweep::KeepAliveClosure::do_oop_work(T* p) {
  mark_and_push(p);
}

#endif // SHARE_VM_GC_IMPLEMENTATION_SHARED_MARKSWEEP_INLINE_HPP
