#ifndef __THALLIUM_MAP_SERIALIZATION_HPP
#define __THALLIUM_MAP_SERIALIZATION_HPP

#include <map>

namespace thallium {

template<class A, typename K, typename V, class Compare, class Alloc>
void save(A& ar, std::map<K,V,Compare,Alloc>& m) {
	size_t size = m.size();
	ar.write(&size);
	for(auto& elem : m) {
		ar & elem.first;
		ar & elem.second;
	}
}

template<class A, typename K, typename V, class Compare, class Alloc>
void load(A& ar, std::map<K,V,Compare,Alloc>& m) {
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
