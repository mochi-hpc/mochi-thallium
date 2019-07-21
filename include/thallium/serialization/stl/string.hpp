#ifndef __THALLIUM_STRING_SERIALIZATION_HPP
#define __THALLIUM_STRING_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/string.hpp>
#else

#include <string>
#include <iostream>

namespace thallium {

template<class A>
inline void save(A& ar, std::string& s) {
    size_t size = s.size();
    ar.write(&size);
    ar.write((const char*)(&s[0]), size);
}

template<class A>
inline void load(A& ar, std::string& s) {
    size_t size;
    s.clear();
    ar.read(&size);
    s.resize(size);
    ar.read((char*)(&s[0]),size);
}

}
#endif
#endif
