#ifndef THALLIUM_PAIR_SERIALIZE_H
#define THALLIUM_PAIR_SERIALIZE_H

#include <utility>

namespace thallium {

template<typename A, typename T1, typename T2>
void serialize(A& a, std::pair<T1,T2>& p) {
	a & p.first;
	a & p.second;
}

}

#endif
