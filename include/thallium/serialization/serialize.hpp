/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <utility>
#include <type_traits>

namespace thallium {

template<typename... T>
using void_t = void;

/**
 * Type trait for input archives.
 */
struct input_archive {};
/**
 * Type trait for output archives.
 */
struct output_archive {};

/**
 * Check at compile-time if the type T is an input archive
 * (the type T has the type trait input_archive).
 */
template<typename T>
struct is_input_archive : public std::is_base_of<input_archive,T> {};

/**
 * Check at compile-time if the type T is an output archive
 * (the type T has the type trait output_archive).
 */
template<typename T>
struct is_output_archive : public std::is_base_of<output_archive,T> {};

/**
 * Compile-time check for a serialize member method in a type T, and
 * appropriate for an archive of type A.
 */
template <typename A, typename T, typename = void_t<>>
struct has_serialize_method {
	constexpr static bool value = false;
};

/**
 * Compile-time check for a serialize member method in a type T, and
 * appropriate for an archive of type A.
 */
template <typename A, typename T>
struct has_serialize_method<A, T, 
		void_t<decltype(std::declval<T&>().serialize(std::declval<A&>()))>> {
	constexpr static bool value = true;
};
	
/**
 * Compile-time check for a load method in a type T, and
 * appropriate for an archive of type A.
 */
template <typename A, typename T, typename = void_t<>>
struct has_load_method {
	constexpr static bool value = false;
};

/**
 * Compile-time check for a load method in a type T, and
 * appropriate for an archive of type A.
 */
template <typename A, typename T>
struct has_load_method<A, T, 
		void_t<decltype(std::declval<T&>().load(std::declval<A&>()))>> {
	constexpr static bool value = true;
};

/**
 * Compile-time check for a save method in a type T, and
 * appropriate for an archive of type A.
 */
template <typename A, typename T, typename = void_t<>>
struct has_save_method {
	constexpr static bool value = false;
};

/**
 * Compile-time check for a save method in a type T, and
 * appropriate for an archive of type A.
 */
template <typename A, typename T>
struct has_save_method<A, T, 
		void_t<decltype(std::declval<T&>().save(std::declval<A&>()))>> {
	constexpr static bool value = true;
};

/**
 * Serializer structure, will provide an apply method that
 * depends on the presence of a serialize function in the provided type.
 */
template<class A, typename T, bool has_serialize>
struct serializer;

template<class A, typename T>
struct serializer<A,T,true> {
	static void apply(A& ar, T&& t) {
		t.serialize(ar);
	}
};

template<class A, typename T>
struct serializer<A,T,false> {
	static void apply(A& ar, T&& t) {
		static_assert(has_serialize_method<A,T>::value, 
			"Undefined \"serialize\" member function");
	}
};

/**
 * Generic serialize method calling apply on a serializer.
 */
template<class A, typename T>
void serialize(A& ar, T&& t) {
	serializer<A,T,has_serialize_method<A,T>::value>::apply(ar,std::forward<T>(t));
}

/**
 * Saver structure, will provide an apply method that depends
 * on the presence of a save function in the provided type.
 */
template<class A, typename T, bool has_save>
struct saver;

template<class A, typename T> 
struct saver<A,T,true> {
	static void apply(A& ar, T& t) {
		t.save(ar);
	}
};

template<class A, typename T>
struct saver<A,T,false> {
	static void apply(A& ar, T& t) {
		serialize(ar,std::forward<T>(t));
	}
};

/**
 * Generic save method calling apply on a saver.
 */
template<class A, typename T>
inline void save(A& ar, T& t) {
	saver<A,T,has_save_method<A,T>::value>::apply(ar,t);
}

template<class A, typename T>
inline void save(A& ar, T&& t) {
    save(ar, t);
}

template<class A, typename T>
inline void save(A& ar, const T&& t) {
    save(ar, const_cast<T&>(t));
}

/**
 * Loader structure, will provide an apply method that depends
 * on the presence of a load function in the provided type.
 */
template<class A, typename T, bool has_load>
struct loader;

template<class A, typename T>
struct loader<A,T,true> {
	static void apply(A& ar, T& t) {
		t.load(ar);
	}
};

template<class A, typename T>
struct loader<A,T,false> {
	static void apply(A& ar, T& t) {
		serialize(ar,std::forward<T>(t));
	}
};

/**
 * Generic load method calling allpy on a loader.
 */
template<class A, typename T>
inline void load(A& ar, T& t) {
	loader<A,T,has_load_method<A,T>::value>::apply(ar,t);
}

template<class A, typename T>
inline void load(A& ar, T&& t) {
    load(ar, t);
}

/**
 * Helper function that serializes an arbitrary number of
 * objects passed as arguments.
 */
template<class A, typename T1, typename... Tn>
void serialize_many(A& ar, T1&& t1, Tn&&... rest) {
	ar & std::forward<T1>(t1);
	serialize_many(ar, std::forward<Tn>(rest)...);
}

template<class A, typename T>
void serialize_many(A& ar, T&& t) {
	ar & std::forward<T>(t);
}

}

#endif
