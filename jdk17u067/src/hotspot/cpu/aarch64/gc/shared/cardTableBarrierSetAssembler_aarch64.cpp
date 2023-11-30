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
#include "interpreter/interp_masm.hpp"

#define __ masm->

void CardTableBarrierSetAssembler::store_check(MacroAssembler* masm, Register obj, Address dst) {

  BarrierSet* bs = BarrierSet::barrier_set();
  assert(bs->kind() == BarrierSet::CardTableBarrierSet, "Wrong barrier set kind");

  __ lsr(obj, obj, CardTable::card_shift);

  assert(CardTable::dirty_card_val() == 0, "must be");

  __ load_byte_map_base(rscratch1);

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

#if 1//perpap
#ifdef TERA_INTERPRETER
void CardTableBarrierSetAssembler::store_check_part1(MacroAssembler* masm, Register obj) {
    __ shrptr(obj, CardTable::card_shift);
}

void CardTableBarrierSetAssembler::h2_store_check_part1(MacroAssembler* masm, Register obj) {
    __ shrptr(obj, CardTable::th_card_shift);
}

void CardTableBarrierSetAssembler::store_check_part2(MacroAssembler* masm, Register obj) {
  // Does a store check for the oop in register obj. The content of
  // register obj is destroyed afterwards.
  BarrierSet* bs = BarrierSet::barrier_set();

  CardTableBarrierSet* ctbs = barrier_set_cast<CardTableBarrierSet>(bs);
  CardTable* ct = ctbs->card_table();
  Address card_addr;

  // The calculation for byte_map_base is as follows:
  // byte_map_base = _byte_map - (uintptr_t(low_bound) >> card_shift);
  // So this essentially converts an address to a displacement and it will
  // never need to be relocated. On 64bit however the value may be too
  // large for a 32bit displacement.
  intptr_t byte_map_base = (intptr_t)ct->byte_map_base();
  if (__ is_simm32(byte_map_base)) {
    card_addr = Address(noreg, obj, Address::times_1, byte_map_base);
  } else {
    // By doing it as an ExternalAddress 'byte_map_base' could be converted to a rip-relative
    // displacement and done in a single instruction given favorable mapping and a
    // smarter version of as_Address. However, 'ExternalAddress' generates a relocation
    // entry and that entry is not properly handled by the relocation code.
    AddressLiteral cardtable((address)byte_map_base, relocInfo::none);
    Address index(noreg, obj, Address::times_1);
    card_addr = __ as_Address(ArrayAddress(cardtable, index));
  }

  int dirty = CardTable::dirty_card_val();
  if (UseCondCardMark) {
    Label L_already_dirty;
    __ cmpb(card_addr, dirty);
    __ jcc(Assembler::equal, L_already_dirty);
    __ movb(card_addr, dirty);
    __ bind(L_already_dirty);
  } else {
    __ movb(card_addr, dirty);
  }
}

void CardTableBarrierSetAssembler::h2_store_check_part2(MacroAssembler* masm, Register obj) {
  // Does a store check for the oop in register obj. The content of
  // register obj is destroyed afterwards.
  BarrierSet* bs = BarrierSet::barrier_set();

  CardTableBarrierSet* ctbs = barrier_set_cast<CardTableBarrierSet>(bs);
  CardTable* ct = ctbs->card_table();
  Address card_addr;

  // The calculation for byte_map_base is as follows:
  // byte_map_base = _byte_map - (uintptr_t(low_bound) >> card_shift);
  // So this essentially converts an address to a displacement and it will
  // never need to be relocated. On 64bit however the value may be too
  // large for a 32bit displacement.
  intptr_t th_byte_map_base = (intptr_t)ct->th_byte_map_base();
  if (__ is_simm32(th_byte_map_base)) {
    card_addr = Address(noreg, obj, Address::times_1, th_byte_map_base);
  } else {
    // By doing it as an ExternalAddress 'byte_map_base' could be converted to a rip-relative
    // displacement and done in a single instruction given favorable mapping and a
    // smarter version of as_Address. However, 'ExternalAddress' generates a relocation
    // entry and that entry is not properly handled by the relocation code.
    AddressLiteral cardtable((address)th_byte_map_base, relocInfo::none);
    Address index(noreg, obj, Address::times_1);
    card_addr = __ as_Address(ArrayAddress(cardtable, index));
  }

  int dirty = CardTable::dirty_card_val();
  if (UseCondCardMark) {
    Label L_already_dirty;
    __ cmpb(card_addr, dirty);
    __ jcc(Assembler::equal, L_already_dirty);
    __ movb(card_addr, dirty);
    __ bind(L_already_dirty);
  } else {
    __ movb(card_addr, dirty);
  }
}

#endif // TERA_INTERPRETER
#endif//perpap

void CardTableBarrierSetAssembler::gen_write_ref_array_post_barrier(MacroAssembler* masm, DecoratorSet decorators,
                                                                    Register start, Register count, Register scratch, RegSet saved_regs) {
  Label L_loop, L_done;
  const Register end = count;

  __ cbz(count, L_done); // zero count - nothing to do

  __ lea(end, Address(start, count, Address::lsl(LogBytesPerHeapOop))); // end = start + count << LogBytesPerHeapOop
  __ sub(end, end, BytesPerHeapOop); // last element address to make inclusive
  __ lsr(start, start, CardTable::card_shift);
  __ lsr(end, end, CardTable::card_shift);
  __ sub(count, end, start); // number of bytes to copy

  __ load_byte_map_base(scratch);
  __ add(start, start, scratch);
  __ bind(L_loop);
  __ strb(zr, Address(start, count));
  __ subs(count, count, 1);
  __ br(Assembler::GE, L_loop);
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
