#ifndef __THALLIUM_REQUEST_HPP
#define __THALLIUM_REQUEST_HPP

#include <margo.h>

namespace thallium {

class margo_engine;

class request {

	friend class margo_engine;

private:

	hg_handle_t m_handle;

	request(hg_handle_t h)
	: m_handle(h) {}

public:

	request(const request& other)
	: m_handle(other.m_handle) {
		margo_ref_incr(m_handle);
	}

	request(request&& other) 
	: m_handle(other.m_handle) {
		other.m_handle = HG_HANDLE_NULL;
	}

	request& operator=(const request& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
		m_handle = other.m_handle;
		margo_ref_incr(m_handle);
		return *this;
	}

	request& operator=(request&& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
		m_handle = other.m_handle;
		other.m_handle = HG_HANDLE_NULL;
		return *this;
	}

	~request() {
		margo_destroy(m_handle);
	}

	template<typename T>
	void respond(T&& t) {
		// TODO serialize
		if(m_handle != HG_HANDLE_NULL) {
			margo_respond(m_handle, nullptr);
		}
	}
};

}

#endif
