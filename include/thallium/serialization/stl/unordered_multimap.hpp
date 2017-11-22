#ifndef THALLIUM_UNORDERED_MULTIMAP_SERIALIZATION_H
#define THALLIUM_UNORDERED_MULTIMAP_SERIALIZATION_H

#include <unordered_map>

namespace thallium {

template<class A, typename K, typename V, class Hash, class Pred, class Alloc>
void save(A& ar, std::unordered_multimap<K,V,Hash,Pred,Alloc>& m) {
	size_t size = m.size();
	ar.write(&size);
	for(auto& elem : m) {
		ar & elem.first;
		ar & elem.second;
	}
}

template<class A, typename K, typename V, class Hash, class Pred, class Alloc>
void load(A& ar, std::unordered_multimap<K,V,Hash,Pred,Alloc>& m) {
	size_t size;
	ar.read(&size);
	m.clear();
	m.reserve(size);
	for(unsigned int i=0; i<size; i++) {
		K k;
		ar & k;
		ar & m[k];
	}
}

}

#endif
