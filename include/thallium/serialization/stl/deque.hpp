#ifndef THALLIUM_DEQUE_SERIALIZATION_H
#define THALLIUM_DEQUE_SERIALIZATION_H

#include <deque>

namespace thallium {

template<class A, typename T, typename Alloc>
void save(A& ar, std::deque<T,Alloc>& l) {
	size_t size = l.size();
	ar.write(&size);
	for(auto& item : l) {
		ar & item;
	}
}

template<class A, typename T, typename Alloc>
void load(A& ar, std::deque<T,Alloc>& l) {
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
