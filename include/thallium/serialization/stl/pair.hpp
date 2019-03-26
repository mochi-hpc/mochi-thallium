/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PAIR_SERIALIZE_HPP
#define __THALLIUM_PAIR_SERIALIZE_HPP

#include <utility>

namespace thallium {

template<typename A, typename T1, typename T2>
void serialize(A& a, std::pair<T1,T2>& p) {
    a & p.first;
    a & p.second;
}

}

#endif
