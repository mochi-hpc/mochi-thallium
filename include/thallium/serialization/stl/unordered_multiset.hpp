#ifndef THALLIUM_UNORDERED_MULTISET_SERIALIZATION_H
#define THALLIUM_UNORDERED_MULTISET_SERIALIZATION_H

#include <utility>
#include <unordered_set>

namespace thallium {

template<class A, typename V, class Hash, class Pred, class Alloc>
void save(A& ar, std::unordered_multiset<V,Hash,Pred,Alloc>& s) {
	size_t size = s.size();
	ar.write(&size);
	for(auto& elem : s) {
		ar & elem;
	}
}

template<class A, typename V, class Hash, class Pred, class Alloc>
void load(A& ar, std::unordered_multiset<V,Hash,Pred,Alloc>& s) {
	size_t size;
	ar.read(&size);
	s.clear();
	s.reserve(size);
	for(unsigned int i=0; i<size; i++) {
		V item;;
		ar & item;
		s.insert(std::move(item));
	}
}

}

#endif
