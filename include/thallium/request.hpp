/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REQUEST_HPP
#define __THALLIUM_REQUEST_HPP

#include <margo.h>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>

namespace thallium {

class engine;
class endpoint;

class request {

	friend class engine;

private:

    engine*     m_engine;
	hg_handle_t m_handle;
    bool        m_disable_response;

	request(engine& e, hg_handle_t h, bool disable_resp)
	: m_engine(&e), m_handle(h), m_disable_response(disable_resp) {}

public:

	request(const request& other)
	: m_engine(other.m_engine), m_handle(other.m_handle), m_disable_response(other.m_disable_response) {
		margo_ref_incr(m_handle);
	}

	request(request&& other)
	: m_engine(other.m_engine), m_handle(other.m_handle), m_disable_response(other.m_disable_response) {
		other.m_handle = HG_HANDLE_NULL;
	}

	request& operator=(const request& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
        m_engine           = other.m_engine;
		m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
		margo_ref_incr(m_handle);
		return *this;
	}

	request& operator=(request&& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
        m_engine           = other.m_engine;
		m_handle           = other.m_handle;
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
            buffer_output_archive arch(b, *m_engine);
            serialize_many(arch, std::forward<T>(t)...);
			margo_respond(m_handle, &b);
		}
	}

    endpoint get_endpoint() const;
};

}

#endif
