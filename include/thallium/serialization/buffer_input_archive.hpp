#ifndef __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP
#define __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP

#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <thallium/serialization/serialize.hpp>
#include <thallium/buffer.hpp>

namespace thallium {

/**
 * buffer_input_archive wraps a buffer object and
 * offers the functionalities to deserialize its content
 * into C++ objects. It inherits from the input_archive
 * trait so that serialization methods know they have to
 * take data out of the buffer and into C++ objects.
 */
class buffer_input_archive : public input_archive {

private:

	const buffer& buffer_;
	std::size_t pos;

	template<typename T, bool b>
	inline void read_impl(T& t, const std::integral_constant<bool, b>&) {
		load(*this,t);
	}

	template<typename T>
	inline void read_impl(T& t, const std::true_type&) {
		read(&t);
	}

public:

	/**
	 * Constructor.
	 *
	 * \param b : reference to a buffer from which to read.
	 * \warning The buffer is held by reference so the life span of
	 * the buffer_input_archive instance should be shorter than that
	 * of the buffer.
	 */
	buffer_input_archive(const buffer& b) : buffer_(b), pos(0) {}

	/**
	 * Operator to get C++ objects of type T from the archive.
	 * The object should either be a basic type, or an STL container
	 * (in which case the appropriate hgcxx/hg_stl/stl_* header should
	 * be included for this function to be properly instanciated), or
	 * any object for which either a serialize member function or
	 * a load member function has been provided.
	 */
	template<typename T>
	buffer_input_archive& operator&(T& obj) {
		read_impl(obj, std::is_arithmetic<T>());
		return *this;
	}

	/**
	 * Operator >> is equivalent to operator &.
	 * \see operator&
	 */
	template<typename T>
	buffer_input_archive& operator>>(T& obj) {
		return (*this) & obj;
	}

	/**
	 * Basic function to read count objects of type T from the buffer.
	 * A memcopy is performed from the buffer to the object, so the
	 * object should either be a basic type or an object that can be
	 * memcopied instead of calling a more elaborate serialize function.
	 */
	template<typename T>
	inline void read(T* t, std::size_t count=1) {
		if(pos + count*sizeof(T) > buffer_.size()) {
			throw std::runtime_error("Reading beyond buffer size");
		}
		std::memcpy((void*)t,(const void*)(&buffer_[pos]),count*sizeof(T));
		pos += count*sizeof(T);
	}
};

}
#endif
