/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_UNORDERED_MAP_SERIALIZATION_HPP
#define __THALLIUM_UNORDERED_MAP_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/unordered_map.hpp>
#else

#include <unordered_map>

namespace thallium {

template<class A, typename K, typename V, class Hash, class Pred, class Alloc>
inline void save(A& ar, std::unordered_map<K,V,Hash,Pred,Alloc>& m) {
    size_t size = m.size();
    ar.write(&size);
    for(auto& elem : m) {
        ar & elem.first;
        ar & elem.second;
    }
}

template<class A, typename K, typename V, class Hash, class Pred, class Alloc>
inline void load(A& ar, std::unordered_map<K,V,Hash,Pred,Alloc>& m) {
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
#endif
