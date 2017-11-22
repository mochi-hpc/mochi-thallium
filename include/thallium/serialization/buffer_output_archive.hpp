#ifndef __THALLIUM_BUFFER_OUTPUT_ARCHIVE_HPP
#define __THALLIUM_BUFFER_OUTPUT_ARCHIVE_HPP

#include <type_traits>
#include <thallium/serialization/serialize.hpp>
#include <thallium/buffer.hpp>

namespace thallium {

/**
 * buffer_output_archive wraps and hg::buffer object and
 * offers the functionalities to serialize C++ objects into
 * the buffer. The buffer is resized down to 0 when creating
 * the archive and will be extended back to an appropriate size
 * as C++ objects are serialized into it.
 */
class buffer_output_archive : public output_archive {

private:

	buffer& buffer_;
	std::size_t pos;

	template<typename T, bool b>
	inline void write_impl(T& t, const std::integral_constant<bool, b>&) {
		save(*this,t);
	}

	template<typename T>
	inline void write_impl(T& t, const std::true_type&) {
		write((char*)&t,sizeof(T));
	}

public:

	/**
	 * Constructor.
	 * 
	 * \param b : reference to a buffer into which to write.
	 * \warning The buffer is held by reference so the life span
	 * of the buffer_output_archive instance should be shorter than
	 * that of the buffer itself.
	 */
	buffer_output_archive(buffer& b) : buffer_(b), pos(0) {
		buffer_.resize(0);
	}

	/**
	 * Operator to add a C++ object of type T into the archive.
	 * The object should either be a basic type, or an STL container
	 * (in which case the appropriate hgcxx/hg_stl/stl_* header should
	 * be included for this function to be properly instanciated), or
	 * any object for which either a serialize member function or
	 * a load member function has been provided.
	 */
	template<typename T>
	buffer_output_archive& operator&(T& obj) {
		write_impl(obj, std::is_arithmetic<T>());
		return *this;
	}

	/**
	 * Operator << is equivalent to operator &.
	 * \see operator&
	 */
	template<typename T>
	buffer_output_archive& operator<<(T& obj) {
		return (*this) & obj;
	}

	/**
	 * Basic function to write count objects of type T into the buffer.
	 * A memcopy is performed from the object's address to the buffer, so
	 * the object should either be basic type or an object that can be
	 * memcopied instead of calling a more elaborate serialize function.
	 */
	template<typename T>
	inline void write(T* const t, size_t count=1) {
		size_t s = count*sizeof(T);
		if(pos+s > buffer_.size()) {
			if(pos+s > buffer_.capacity()) {
				buffer_.reserve(buffer_.capacity()*2);
			}
			buffer_.resize(pos+s);
		}
		memcpy((void*)(&buffer_[pos]),(void*)t,s);
		pos += s;
	}
};

}

#endif
