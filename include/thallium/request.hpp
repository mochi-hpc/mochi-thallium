/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REQUEST_HPP
#define __THALLIUM_REQUEST_HPP

#include <margo.h>
#include <thallium/margo_exception.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>

namespace thallium {

class engine;
class endpoint;

/**
 * @brief A request object is created whenever a server
 * receives an RPC. The object is passed as first argument to
 * the function associated with the RPC. The request allows
 * one to get information from the caller and to respond to
 * the RPC.
 */
class request {

	friend class engine;

private:

    engine*     m_engine;
	hg_handle_t m_handle;
    bool        m_disable_response;

    /**
     * @brief Constructor. Made private since request are only created
     * by the engine within RPC callbacks.
     *
     * @param e engine object that created the request.
     * @param h handle of the RPC that was received.
     * @param disable_resp whether responses are disabled.
     */
	request(engine& e, hg_handle_t h, bool disable_resp)
	: m_engine(&e), m_handle(h), m_disable_response(disable_resp) {}

public:

    /**
     * @brief Copy constructor.
     */
	request(const request& other)
	: m_engine(other.m_engine), m_handle(other.m_handle), m_disable_response(other.m_disable_response) {
        hg_return_t ret = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
	}

    /**
     * @brief Move constructor.
     */
	request(request&& other)
	: m_engine(other.m_engine), m_handle(other.m_handle), m_disable_response(other.m_disable_response) {
		other.m_handle = HG_HANDLE_NULL;
	}

    /**
     * @brief Copy-assignment operator.
     */
	request& operator=(const request& other) {
		if(m_handle == other.m_handle) return *this;
        hg_return_t ret;
        ret = margo_destroy(m_handle);
        MARGO_ASSERT(ret, margo_destroy);
        m_engine           = other.m_engine;
		m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
        ret = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
		return *this;
	}

    /**
     * @brief Move-assignment operator.
     */
	request& operator=(request&& other) {
		if(m_handle == other.m_handle) return *this;
		margo_destroy(m_handle);
        m_engine           = other.m_engine;
		m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
		other.m_handle = HG_HANDLE_NULL;
		return *this;
	}

    /**
     * @brief Destructor.
     */
	~request() throw(margo_exception) {
		hg_return_t ret = margo_destroy(m_handle);
        MARGO_ASSERT(ret, margo_destroy);
	}

    /**
     * @brief Responds to the sender of the RPC.
     * Serializes the series of arguments provided and
     * send the resulting buffer to the sender.
     *
     * @tparam T Types of parameters to serialize.
     * @param t Parameters to serialize.
     */
	template<typename ... T>
	void respond(T&&... t) const {
        if(m_disable_response) return; // XXX throwing an exception?
		if(m_handle != HG_HANDLE_NULL) {
            buffer b;
            buffer_output_archive arch(b, *m_engine);
            serialize_many(arch, std::forward<T>(t)...);
			hg_return_t ret = margo_respond(m_handle, &b);
            MARGO_ASSERT(ret, margo_respond);
		}
	}

    /**
     * @brief Get the endpoint corresponding to the sender of the RPC.
     *
     * @return endpoint corresponding to the sender of the RPC.
     */
    endpoint get_endpoint() const;
};

}

#endif
