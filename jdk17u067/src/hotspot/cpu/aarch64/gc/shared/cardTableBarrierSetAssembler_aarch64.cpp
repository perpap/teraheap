/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates. All rights reserved.
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

#include "precompiled.hpp"
#include "asm/macroAssembler.inline.hpp"
#include "gc/shared/barrierSet.hpp"
#include "gc/shared/cardTable.hpp"
#include "gc/shared/cardTableBarrierSet.hpp"
#include "gc/shared/cardTableBarrierSetAssembler.hpp"
#include "gc/shared/gc_globals.hpp"
#include "gc/teraHeap/teraHeap.hpp"
#include "interpreter/interp_masm.hpp"

#define __ masm->

void CardTableBarrierSetAssembler::store_check(MacroAssembler* masm, Register obj, Address dst) {
	BarrierSet* bs = BarrierSet::barrier_set();
	assert(bs->kind() == BarrierSet::CardTableBarrierSet, "Wrong barrier set kind");

#ifdef TERA_INTERPRETER
	if (EnableTeraHeap) {
		Label L_h1, L_done;
		// Load the TeraHeap's H2 start address in r11
		__ lea(r11, Address((address)Universe::teraHeap()->h2_start_addr(), relocInfo::none));
		__ cmp(obj, r11);
		__ br(Assembler::LT, L_h1);
    #if 0//FIXME
    // Load the TeraHeap's H2 end address in r12
    __ lea(r12, Address((address)Universe::teraHeap()->h2_end_addr(), relocInfo::none));
		__ cmp(obj, r12);
		__ br(Assembler::GT, L_h1);
    #endif//FIXME
		//Obj in H2
		__ lsr(obj, obj, CardTable::th_card_shift);
		assert(CardTable::dirty_card_val() == 0, "must be");
		__ load_th_byte_map_base(rscratch1);
		__ b(L_done);

		//obj in H1
		__ bind(L_h1);
		// Does a store check for the oop in register obj. The content of
		// register obj is destroyed afterwards.
		__ lsr(obj, obj, CardTable::card_shift);
		assert(CardTable::dirty_card_val() == 0, "must be");
		__ load_byte_map_base(rscratch1);

		__ bind(L_done);
	}else{
		// Does a store check for the oop in register obj. The content of
		// register obj is destroyed afterwards.
		__ lsr(obj, obj, CardTable::card_shift);
		assert(CardTable::dirty_card_val() == 0, "must be");
		__ load_byte_map_base(rscratch1);
	}
#else	//Vanilla JVM, obj is on H1
	// Does a store check for the oop in register obj. The content of
	// register obj is destroyed afterwards.
	__ lsr(obj, obj, CardTable::card_shift);
	assert(CardTable::dirty_card_val() == 0, "must be");
	__ load_byte_map_base(rscratch1);
#endif//TERA_INTERPRETER

	if (UseCondCardMark) {
		Label L_already_dirty;
		__ ldrb(rscratch2,  Address(obj, rscratch1));
		__ cbz(rscratch2, L_already_dirty);
		__ strb(zr, Address(obj, rscratch1));
		__ bind(L_already_dirty);
	} else {
		__ strb(zr, Address(obj, rscratch1));
	}
}

void CardTableBarrierSetAssembler::gen_write_ref_array_post_barrier(MacroAssembler* masm, DecoratorSet decorators,
		Register start, Register count, Register scratch, RegSet saved_regs) {
	Label L_h1, L_h1_loop, L_done;
	const Register end = count;

	__ cbz(count, L_done); // zero count - nothing to do
	__ lea(end, Address(start, count, Address::lsl(LogBytesPerHeapOop))); // end = start + count << LogBytesPerHeapOop
	__ sub(end, end, BytesPerHeapOop); // last element address to make inclusive
#ifdef TERA_INTERPRETER
	if (EnableTeraHeap) {
		Label L_h2_loop;
		// Load the TeraHeap's H2 start address in scratch
		__ lea(scratch, Address((address)Universe::teraHeap()->h2_start_addr(), relocInfo::none));
		// Check if array is in H1 or H2
		__ cmp(start, scratch);
		__ br(Assembler::LT, L_h1);
    #if 0//FIXME
    // Load the TeraHeap's H2 end address in r12
    __ lea(r12, Address((address)Universe::teraHeap()->h2_end_addr(), relocInfo::none));
		__ cmp(start, r12);
		__ br(Assembler::GT, L_h1);
    #endif//FIXME
		//Obj in H2
		__ lsr(start, start, CardTable::th_card_shift);
		__ lsr(end, end, CardTable::th_card_shift);
		__ sub(count, end, start); // number of bytes to copy
		__ load_th_byte_map_base(scratch);
		__ add(start, start, scratch);
		__ bind(L_h2_loop);
		__ strb(zr, Address(start, count));
		__ subs(count, count, 1);
		__ br(Assembler::GE, L_h2_loop);
		__ b(L_done);
	}
#endif// TERA_INTERPRETER
	//Obj in H1
	__ bind(L_h1);
	__ lsr(start, start, CardTable::card_shift);
	__ lsr(end, end, CardTable::card_shift);
	__ sub(count, end, start); // number of bytes to copy
	__ load_byte_map_base(scratch);
	__ add(start, start, scratch);
	__ bind(L_h1_loop);
	__ strb(zr, Address(start, count));
	__ subs(count, count, 1);
	__ br(Assembler::GE, L_h1_loop);
	__ bind(L_done);
}

void CardTableBarrierSetAssembler::oop_store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                                Address dst, Register val, Register tmp1, Register tmp2) {
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool is_array = (decorators & IS_ARRAY) != 0;
  bool on_anonymous = (decorators & ON_UNKNOWN_OOP_REF) != 0;
  bool precise = is_array || on_anonymous;

  bool needs_post_barrier = val != noreg && in_heap;
  BarrierSetAssembler::store_at(masm, decorators, type, dst, val, noreg, noreg);
  if (needs_post_barrier) {
    // flatten object address if needed
    if (!precise || (dst.index() == noreg && dst.offset() == 0)) {
      store_check(masm, dst.base(), dst);
    } else {
      __ lea(r3, dst);
      store_check(masm, r3, dst);
    }
  }
}
