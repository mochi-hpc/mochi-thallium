/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MULTISET_SERIALIZATION_HPP
#define __THALLIUM_MULTISET_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/set.hpp>
#else

#include <utility>
#include <set>

namespace thallium {

template<class A, typename T, class Compare, class Alloc>
inline void save(A& ar, std::multiset<T,Compare,Alloc>& s) {
    size_t size = s.size();
    ar.write(&size);
    for(auto& elem : s) {
        ar & elem;
    }
}

template<class A, typename T,  class Compare, class Alloc>
inline void load(A& ar, std::multiset<T,Compare,Alloc>& s) {
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
#endif
