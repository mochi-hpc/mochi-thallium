#ifndef THALLIUM_STRING_SERIALIZATION_H
#define THALLIUM_STRING_SERIALIZATION_H

#include <string>
#include <iostream>

namespace thallium {

template<class A>
void save(A& ar, std::string& s) {
	size_t size = s.size();
	ar.write(&size);
	ar.write((const char*)(&s[0]), size);
}

template<class A>
void load(A& ar, std::string& s) {
	size_t size;
	s.clear();
	ar.read(&size);
	s.resize(size);
	ar.read((char*)(&s[0]),size);
}

}

#endif
