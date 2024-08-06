#include "gc_implementation/teraHeap/teraHistoBackReferences.hpp"
//
//// Add a new entry to the histogram for 'obj'
//void TeraHistoBackReferences::update_back_ref_stats(bool is_old, bool is_tera_cache) {
//	std::tr1::tuple<int, int, int> val;
//	std::tr1::tuple<int, int, int> new_val;
//
//	val = histogram[back_ref_obj];
//	
//	if (is_old) {                         // Reference is in the old generation  
//		new_val = std::tr1::make_tuple(
//				std::tr1::get<0>(val),
//				std::tr1::get<1>(val) + 1,
//				std::tr1::get<2>(val));
//	}
//	else if (is_tera_cache) {             // Reference is in the tera cache
//		new_val = std::tr1::make_tuple(
//				std::tr1::get<0>(val),
//				std::tr1::get<1>(val),
//				std::tr1::get<2>(val) + 1);
//	} else {                              // Reference is in the new generation
//		new_val = std::tr1::make_tuple(
//				std::tr1::get<0>(val) + 1,
//				std::tr1::get<1>(val),
//				std::tr1::get<2>(val));
//	}
//	
//	histogram[back_ref_obj] = new_val;
//}
//
//// Enable traversal `obj` for backward references.
//void TeraHistoBackReferences::enable_back_ref_traversal(oop* obj) {
//	std::tr1::tuple<int, int, int> val;
//
//	val = std::tr1::make_tuple(0, 0, 0);
//
//	back_ref_obj = obj;
//  // Add entry to the histogram if does not exist
//	histogram[obj] = val;
//}
//
//// Print the histogram
//void TeraHistoBackReferences::print_back_ref_stats() {
//	std::map<oop *, std::tr1::tuple<int, int, int> >::const_iterator it;
//	
//	thlog_or_tty->print_cr("Start_Back_Ref_Statistics\n");
//
//	for(it = histogram.begin(); it != histogram.end(); ++it) {
//		if (std::tr1::get<0>(it->second) > 1000 || std::tr1::get<1>(it->second) > 1000) {
//			thlog_or_tty->print_cr("[HISTOGRAM] ADDR = %p | NAME = %s | NEW = %d | OLD = %d | TC = %d\n",
//					it->first, oop(it->first)->klass()->internal_name(), std::tr1::get<0>(it->second),
//					std::tr1::get<1>(it->second), std::tr1::get<2>(it->second));
//		}
//	}
//	
//	thlog_or_tty->print_cr("End_Back_Ref_Statistics\n");
//
//	// Empty the histogram at the end of each minor gc
//	histogram.clear();
//}
