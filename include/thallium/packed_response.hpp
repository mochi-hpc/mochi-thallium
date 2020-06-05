/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PACKED_RESPONSE_HPP
#define __THALLIUM_PACKED_RESPONSE_HPP

#include <thallium/margo_exception.hpp>
#include <thallium/proc_object.hpp>
#include <thallium/serialization/proc_input_archive.hpp>
#include <thallium/serialization/serialize.hpp>

namespace thallium {

class callable_remote_procedure;
class async_response;

namespace detail {
    struct engine_impl;
}

/**
 * @brief packed_response objects are created as a reponse to
 * an RPC. They can be used to extract the response from the
 * RPC if the RPC sent one.
 */
class packed_response {
    friend class callable_remote_procedure;
    friend class async_response;

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_handle_t m_handle = HG_HANDLE_NULL;

    /**
     * @brief Constructor. Made private since packed_response
     * objects are created by callable_remote_procedure only.
     *
     * @param h Handle containing the result of an RPC.
     * @param e Engine associated with the RPC.
     */
    packed_response(hg_handle_t h, const std::weak_ptr<detail::engine_impl>& e)
    : m_engine_impl(e)
    , m_handle(h) {
        hg_return_t ret = margo_ref_incr(h);
        MARGO_ASSERT(ret, margo_ref_incr);
    }

    packed_response() = default;

  public:
    ~packed_response() {
        if(m_handle != HG_HANDLE_NULL) {
            margo_destroy(m_handle);
        }
    }

    /**
     * @brief Converts the handle's content into the requested object.
     *
     * @tparam T Type into which to convert the content of the buffer.
     *
     * @return Buffer converted into the desired type.
     */
    template <typename T> T as() const {
        if(m_handle == HG_HANDLE_NULL) {
            throw exception(
                "Cannot unpack data from handle. Are you trying to "
                "unpack data from an RPC that does not return any?");
        }
        std::tuple<T> t;
        meta_proc_fn  mproc = [this, &t](hg_proc_t proc) {
            return proc_object(proc, t, m_engine_impl);
        };
        hg_return_t ret = margo_get_output(m_handle, &mproc);
        MARGO_ASSERT(ret, margo_get_output);
        ret = margo_free_output(m_handle, &mproc);
        MARGO_ASSERT(ret, margo_free_output);
        return std::get<0>(t);
    }

    /**
     * @brief Converts the content of the buffer into a std::tuple
     * of types T1, T2, ... Tn.
     *
     * This function allows to do something like the following:
     * int x;
     * double y;
     * std::tie(x,y) = pack.as<int,double>();
     *
     * @tparam T1 First type of the tuple.
     * @tparam T2 Second type of the tuple.
     * @tparam Tn Other types of the tuple.
     *
     * @return buffer content converted into the desired std::tuple.
     */
    template <typename T1, typename T2, typename... Tn> auto as() const {
        if(m_handle == HG_HANDLE_NULL) {
            throw exception(
                "Cannot unpack data from handle. Are you trying to "
                "unpack data from an RPC that does not return any?");
        }
        std::tuple<typename std::decay<T1>::type, typename std::decay<T2>::type,
                   typename std::decay_t<Tn>::type...>
                     t;
        meta_proc_fn mproc = [this, &t](hg_proc_t proc) {
            return proc_object(proc, t, m_engine_impl);
        };
        hg_return_t ret = margo_get_output(m_handle, &mproc);
        MARGO_ASSERT(ret, margo_get_output);
        ret = margo_free_output(m_handle, &mproc);
        MARGO_ASSERT(ret, margo_free_output);
        return t;
    }

    /**
     * @brief Converts the content of the packed_response into
     * the desired object type. Allows to cast the packed_response
     * into the desired object type.
     *
     * @tparam T Type into which to convert the response.
     *
     * @return An object of the desired type.
     */
    template <typename T> operator T() const { return as<T>(); }
};

} // namespace thallium

#endif
