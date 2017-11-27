/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MULTIMAP_SERIALIZATION_HPP
#define __THALLIUM_MULTIMAP_SERIALIZATION_HPP

#include <map>

namespace thallium {

template<class A, typename K, typename V, class Compare, class Alloc>
void save(A& ar, std::multimap<K,V,Compare,Alloc>& m) {
	size_t size = m.size();
	ar.write(&size);
	for(auto& elem : m) {
		ar & elem.first;
		ar & elem.second;
	}
}

template<class A, typename K, typename V, class Compare, class Alloc>
void load(A& ar, std::multimap<K,V,Compare,Alloc>& m) {
	size_t size;
	ar.read(&size);
	m.clear();
	for(unsigned int i=0; i<size; i++) {
		K k;
		ar & k;
		ar & m[k];
	}
}

}

#endif
