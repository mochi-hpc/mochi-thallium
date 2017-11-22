#ifndef THALLIUM_LIST_SERIALIZATION_H
#define THALLIUM_LIST_SERIALIZATION_H

#include <list>

namespace thallium {

template<class A, typename T, typename Alloc>
void save(A& ar, std::list<T,Alloc>& l) {
	size_t size = l.size();
	ar.write(&size);
	for(auto& item : l) {
		ar & item;
	}
}

template<class A, typename T, typename Alloc>
void load(A& ar, std::list<T,Alloc>& l) {
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
