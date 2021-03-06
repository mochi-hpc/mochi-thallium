/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_DEQUE_SERIALIZATION_HPP
#define __THALLIUM_DEQUE_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/deque.hpp>
#else

#include <deque>

namespace thallium {

template<class A, typename T, typename Alloc>
inline void save(A& ar, std::deque<T,Alloc>& l) {
    size_t size = l.size();
    ar.write(&size);
    for(auto& item : l) {
        ar & item;
    }
}

template<class A, typename T, typename Alloc>
inline void load(A& ar, std::deque<T,Alloc>& l) {
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
#endif
