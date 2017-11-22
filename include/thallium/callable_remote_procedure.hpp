#ifndef __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP
#define __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP

#include <tuple>
#include <cstdint>
#include <margo.h>
#include <thallium/buffer.hpp>

namespace thallium {

class engine;
class remote_procedure;
class endpoint;

class callable_remote_procedure {

	friend class remote_procedure;

private:
	hg_handle_t m_handle;
    bool        m_ignore_response;

	callable_remote_procedure(hg_id_t id, const endpoint& ep, bool ignore_resp);

public:

	callable_remote_procedure(const callable_remote_procedure& other) {
		if(m_handle != HG_HANDLE_NULL) {
			margo_destroy(m_handle);
		}
		m_handle = other.m_handle;
		if(m_handle != HG_HANDLE_NULL) {
			margo_ref_incr(m_handle);
		}
	}

	callable_remote_procedure(callable_remote_procedure&& other) {
		if(m_handle != HG_HANDLE_NULL) {
            margo_destroy(m_handle);
        }
		m_handle = other.m_handle;
		other.m_handle = HG_HANDLE_NULL;
	}

	callable_remote_procedure& operator=(const callable_remote_procedure& other) {
		if(&other == this) return *this;
		if(m_handle != HG_HANDLE_NULL) {
			margo_destroy(m_handle);
		}
		m_handle = other.m_handle;
		margo_ref_incr(m_handle);
		return *this;
	}

	callable_remote_procedure& operator=(callable_remote_procedure&& other) {
		if(&other == this) return *this;
		if(m_handle != HG_HANDLE_NULL) {
			margo_destroy(m_handle);
		}
		m_handle = other.m_handle;
		other.m_handle = HG_HANDLE_NULL;
		return *this;
	}

	~callable_remote_procedure() {
		if(m_handle != HG_HANDLE_NULL) {
			margo_destroy(m_handle);
		}
	}

	template<typename ... T>
	auto operator()(const T& ... t) const {
		// TODO throw an exception if handle is null
//		buffer input;
//		BufferOutputArchive arch(input);
//		serialize_many(arch, std::forward<T>(t)...);
//		serialize(std::forward<T>(t)
//		auto input = std::tie(t...);
		margo_forward(m_handle, nullptr);


//		Buffer output;

		//margo_get_output(m_handle, &output);
		return true;//Pack(std::move(output));
	}


	auto operator()(const buffer& buf) const {
		margo_forward(m_handle, const_cast<void*>(static_cast<const void*>(&buf)));
        buffer output;
        if(m_ignore_response) return output;
        margo_get_output(m_handle, &output);
        margo_free_output(m_handle, &output); // won't do anything on a buffer type
        return output;
	}
};

}

#endif
