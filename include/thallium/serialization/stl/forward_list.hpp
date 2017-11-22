#ifndef THALLIUM_FORWARD_LIST_SERIALIZATION_H
#define THALLIUM_FORWARD_LIST_SERIALIZATION_H

#include <forward_list>

namespace thallium {

template<class A, typename T, typename Alloc>
void save(A& ar, std::forward_list<T,Alloc>& l) {
	size_t size = l.size();
	ar.write(&size);
	for(auto& item : l) {
		ar & item;
	}
}

template<class A, typename T, typename Alloc>
void load(A& ar, std::forward_list<T,Alloc>& l) {
	size_t size;
	l.clear();
	ar.read(&size);
	l.resize(size);
	for(auto& item : l) {
		ar & item;
	}
}

}

#endif
