/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP
#define __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP

#include <tuple>
#include <cstdint>
#include <utility>
#include <margo.h>
#include <thallium/buffer.hpp>
#include <thallium/packed_response.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>

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

	auto forward(const buffer& buf) const {
		margo_forward(m_handle, const_cast<void*>(static_cast<const void*>(&buf)));
        buffer output;
        if(m_ignore_response) return packed_response(std::move(output));
        margo_get_output(m_handle, &output);
        margo_free_output(m_handle, &output); // won't do anything on a buffer type
        return packed_response(std::move(output));
	}

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
	auto operator()(T&& ... t) const {
		buffer b;
        buffer_output_archive arch(b);
        serialize_many(arch, std::forward<T>(t)...);
		return forward(b);
	}

    auto operator()() const {
        buffer b;
        return forward(b);
    }
};

}

#endif
