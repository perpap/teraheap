/*
 * Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_OOPS_OBJARRAYKLASS_INLINE_HPP
#define SHARE_VM_OOPS_OBJARRAYKLASS_INLINE_HPP

#include "gc_implementation/shared/markSweep.inline.hpp"
#include "oops/objArrayKlass.hpp"
#include "utilities/macros.hpp"
#if INCLUDE_ALL_GCS
#include "gc_implementation/parallelScavenge/psCompactionManager.inline.hpp"
#include "gc_implementation/parallelScavenge/psParallelCompact.hpp"
#endif // INCLUDE_ALL_GCS

void ObjArrayKlass::oop_follow_contents(oop obj, int index) {
  if (UseCompressedOops) {
    objarray_follow_contents<narrowOop>(obj, index);
  } else {
    objarray_follow_contents<oop>(obj, index);
  }
}

template <class T>
void ObjArrayKlass::objarray_follow_contents(oop obj, int index) {
  objArrayOop a = objArrayOop(obj);
  const size_t len = size_t(a->length());
  const size_t beg_index = size_t(index);
  assert(beg_index < len || len == 0, "index too large");

  const size_t stride = MIN2(len - beg_index, ObjArrayMarkingStride);
  const size_t end_index = beg_index + stride;
  T* const base = (T*)a->base();
  T* const beg = base + beg_index;
  T* const end = base + end_index;

#ifdef TERA_MAJOR_GC
#ifdef NO_HINTS
  if (EnableTeraHeap) {
    if (obj->get_oldgen_obj_age() == 0) {
      obj->mark_move_h2(0, 0);
    }
  }
#endif

	if (EnableTeraHeap && Universe::teraHeap()->h2_promotion_policy(obj)) {
		Universe::teraHeap()->set_cur_obj_group_id((long int) obj->get_obj_group_id());
		Universe::teraHeap()->set_cur_obj_part_id((long int) obj->get_obj_part_id());

    // Push the non-NULL elements of the next stride on the marking stack.
		for (T* e = beg; e < end; e++) {
			MarkSweep::tera_mark_and_push<T>(e);
		}
	}
	else {
    // Push the non-NULL elements of the next stride on the marking stack.
    for (T* e = beg; e < end; e++) {
			MarkSweep::mark_and_push<T>(e);
		}
	}
#else
  // Push the non-NULL elements of the next stride on the marking stack.
	for (T* e = beg; e < end; e++) {
		MarkSweep::mark_and_push<T>(e);
	}
#endif

  if (end_index < len) {
    MarkSweep::push_objarray(a, end_index); // Push the continuation.
  }
}

#ifdef TERA_MAJOR_GC
template <class T>
void ObjArrayKlass::h2_objarray_follow_contents(oop obj, int index) {
  objArrayOop a = objArrayOop(obj);
  const size_t len = size_t(a->length());
  const size_t beg_index = size_t(index);
  assert(beg_index < len || len == 0, "index too large");

  //const size_t stride = MIN2(len - beg_index, ObjArrayMarkingStride);
  const size_t end_index = beg_index + len;
  T* const base = (T*)a->base();
  T* const beg = base + beg_index;
  T* const end = base + end_index;

  // Push the non-NULL elements of the next stride on the marking stack.
  for (T* e = beg; e < end; e++) {
    MarkSweep::h2_liveness_analysis<T>(e);
  }
}
#endif

#if INCLUDE_ALL_GCS
void ObjArrayKlass::oop_follow_contents(ParCompactionManager* cm, oop obj,
                                        int index) {
  if (UseCompressedOops) {
    objarray_follow_contents<narrowOop>(cm, obj, index);
  } else {
    objarray_follow_contents<oop>(cm, obj, index);
  }
}

template <class T>
void ObjArrayKlass::objarray_follow_contents(ParCompactionManager* cm, oop obj,
                                             int index) {
  objArrayOop a = objArrayOop(obj);
  const size_t len = size_t(a->length());
  const size_t beg_index = size_t(index);
  assert(beg_index < len || len == 0, "index too large");

  const size_t stride = MIN2(len - beg_index, ObjArrayMarkingStride);
  const size_t end_index = beg_index + stride;
  T* const base = (T*)a->base();
  T* const beg = base + beg_index;
  T* const end = base + end_index;

  // Push the non-NULL elements of the next stride on the marking stack.
  for (T* e = beg; e < end; e++) {
    PSParallelCompact::mark_and_push<T>(cm, e);
  }

  if (end_index < len) {
    cm->push_objarray(a, end_index); // Push the continuation.
  }
}
#endif // INCLUDE_ALL_GCS

#endif // SHARE_VM_OOPS_OBJARRAYKLASS_INLINE_HPP
