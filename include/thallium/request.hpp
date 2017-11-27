#ifndef __THALLIUM_REQUEST_HPP
#define __THALLIUM_REQUEST_HPP

#include <margo.h>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>

namespace thallium {

class engine;

class request {

	friend class engine;

private:

	hg_handle_t m_handle;
    bool        m_disable_response;

	request(hg_handle_t h, bool disable_resp)
	: m_handle(h), m_disable_response(disable_resp) {}

public:

	request(const request& other)
	: m_handle(other.m_handle), m_disable_response(other.m_disable_response) {
		margo_ref_incr(m_handle);
	}

	request(request&& other) 
	: m_handle(other.m_handle), m_disable_response(other.m_disable_response) {
		other.m_handle = HG_HANDLE_NULL;
	}

	request& operator=(const request& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
		m_handle = other.m_handle;
        m_disable_response = other.m_disable_response;
		margo_ref_incr(m_handle);
		return *this;
	}

	request& operator=(request&& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
		m_handle = other.m_handle;
        m_disable_response = other.m_disable_response;
		other.m_handle = HG_HANDLE_NULL;
		return *this;
	}

	~request() {
		margo_destroy(m_handle);
	}

	template<typename ... T>
	void respond(T&&... t) const {
        if(m_disable_response) return; // XXX throwing an exception?
		if(m_handle != HG_HANDLE_NULL) {
            buffer b;
            buffer_output_archive arch(b);
            serialize_many(arch, std::forward<T>(t)...);
			margo_respond(m_handle, &b);
		}
	}
/*
    void respond(const buffer& output) const {
        if(m_disable_response) return; // XXX throwing an exception?
        if(m_handle != HG_HANDLE_NULL) {
            margo_respond(m_handle, const_cast<void*>(static_cast<const void*>(&output)));
        }
    }

    void respond(buffer& output) const {
        respond((const buffer&)output);
    }

    void respond(buffer&& output) const {
        respond((const buffer&)output);
    }
*/
};

}

#endif
