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
#include <thallium/margo_exception.hpp>

namespace thallium {

class engine;
class remote_procedure;
class endpoint;

/**
 * @brief callable_remote_procedure objects represent an RPC
 * ready to be called (using the parenthesis operator).
 * It is created from a remote_procedure object using
 * remote_procedure::on(endpoint).
 */
class callable_remote_procedure {

	friend class remote_procedure;

private:
    engine*     m_engine;
	hg_handle_t m_handle;
    bool        m_ignore_response;

    /**
     * @brief Constructor. Made private since callable_remote_procedure can only
     * be created from remote_procedure::on().
     *
     * @param e engine used to create the remote_procedure.
     * @param id id of the RPC to call.
     * @param ep endpoint on which to call the RPC.
     * @param ignore_resp whether the response should be ignored.
     */
	callable_remote_procedure(engine& e, hg_id_t id, const endpoint& ep, bool ignore_resp);

    /**
     * @brief Sends the RPC to the endpoint (calls margo_forward), passing a buffer
     * in which the arguments have been serialized.
     *
     * @param buf Buffer containing a serialized version of the arguments.
     *
     * @return a packed_response object from which the returned value can be deserialized.
     */
     packed_response forward(const buffer& buf) const {
        hg_return_t ret;
        ret = margo_forward(m_handle, const_cast<void*>(static_cast<const void*>(&buf)));
        MARGO_ASSERT(ret, margo_forward);
        buffer output;
        if(m_ignore_response) return packed_response(std::move(output), *m_engine);
        ret = margo_get_output(m_handle, &output);
        MARGO_ASSERT(ret, margo_get_output);
        ret = margo_free_output(m_handle, &output); // won't do anything on a buffer type
        MARGO_ASSERT(ret, margo_free_output);
        return packed_response(std::move(output), *m_engine);
	}

public:

     /**
      * @brief Copy-constructor.
      */
	callable_remote_procedure(const callable_remote_procedure& other) {
        hg_return_t ret;
        if(m_handle != HG_HANDLE_NULL) {
			ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
		}
		m_handle = other.m_handle;
		if(m_handle != HG_HANDLE_NULL) {
            ret = margo_ref_incr(m_handle);
            MARGO_ASSERT(ret, margo_ref_incr);
		}
	}

    /**
     * @brief Move-constructor.
     */
	callable_remote_procedure(callable_remote_procedure&& other) {
		if(m_handle != HG_HANDLE_NULL) {
            hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
        }
		m_handle = other.m_handle;
		other.m_handle = HG_HANDLE_NULL;
	}

    /**
     * @brief Copy-assignment operator.
     */
	callable_remote_procedure& operator=(const callable_remote_procedure& other) {
        hg_return_t ret;
		if(&other == this) return *this;
		if(m_handle != HG_HANDLE_NULL) {
            ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
		}
		m_handle = other.m_handle;
		ret = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
		return *this;
	}

    
    /**
     * @brief Move-assignment operator.
     */
	callable_remote_procedure& operator=(callable_remote_procedure&& other) {
		if(&other == this) return *this;
		if(m_handle != HG_HANDLE_NULL) {
			hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
		}
		m_handle = other.m_handle;
		other.m_handle = HG_HANDLE_NULL;
		return *this;
	}

    /**
     * @brief Destructor.
     */
	~callable_remote_procedure()  {
		if(m_handle != HG_HANDLE_NULL) {
            hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
		}
	}

    /**
     * @brief Operator to call the RPC. Will serialize the arguments
     * in a buffer and send the RPC to the endpoint.
     *
     * @tparam T Types of the parameters.
     * @param t Parameters of the RPC.
     *
     * @return a packed_response object containing the returned value.
     */
	template<typename ... T>
	packed_response operator()(T&& ... t) const {
		buffer b;
        buffer_output_archive arch(b, *m_engine);
        serialize_many(arch, std::forward<T>(t)...);
		return forward(b);
	}

    /**
     * @brief Operator to call the RPC without any argument.
     *
     * @return a packed_response object containing the returned value.
     */
    packed_response operator()() const {
        buffer b;
        return forward(b);
    }
};

}

#endif
