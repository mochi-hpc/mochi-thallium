#ifndef THALLIUM_SET_SERIALIZATION_H
#define THALLIUM_SET_SERIALIZATION_H

#include <utility>
#include <set>

namespace thallium {

template<class A, typename T, class Compare, class Alloc>
void save(A& ar, std::set<T,Compare,Alloc>& s) {
	size_t size = s.size();
	ar.write(&size);
	for(auto& elem : s) {
		ar & elem;
	}
}

template<class A, typename T,  class Compare, class Alloc>
void load(A& ar, std::set<T,Compare,Alloc>& s) {
	size_t size;
	s.clear();
	ar.read(&size);
	for(int i=0; i<size; i++) {
		T item;
		ar & item;
		s.insert(std::move(item));
	}
}

} // namespace

#endif
