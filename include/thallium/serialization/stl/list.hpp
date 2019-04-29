/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_LIST_SERIALIZATION_HPP
#define __THALLIUM_LIST_SERIALIZATION_HPP

#include <list>

namespace thallium {

template<class A, typename T, typename Alloc>
inline void save(A& ar, std::list<T,Alloc>& l) {
    size_t size = l.size();
    ar.write(&size);
    for(auto& item : l) {
        ar & item;
    }
}

template<class A, typename T, typename Alloc>
inline void load(A& ar, std::list<T,Alloc>& l) {
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
