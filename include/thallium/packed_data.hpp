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
 * @brief packed_data objects encapsulate data serialized
 * into an hg_handle_t, whether it is input or output data.
 */
template<typename ... CtxArg>
class packed_data {
    friend class callable_remote_procedure;
    friend class async_response;

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_handle_t m_handle = HG_HANDLE_NULL;
    hg_return_t (*m_unpack_fn)(hg_handle_t,void*) = nullptr;
    hg_return_t (*m_free_fn)(hg_handle_t,void*) = nullptr;
    mutable std::tuple<CtxArg...> m_context;

    /**
     * @brief Constructor. Made private since packed_data
     * objects are created by callable_remote_procedure only.
     *
     * @param h Handle containing the result of an RPC.
     * @param e Engine associated with the RPC.
     */
    packed_data(hg_return_t (*unpack_fn)(hg_handle_t,void*),
                hg_return_t (*free_fn)(hg_handle_t,void*),
                hg_handle_t h,
                std::weak_ptr<detail::engine_impl> e,
                const std::tuple<CtxArg...>& ctx = std::tuple<CtxArg...>())
    : m_engine_impl(std::move(e))
    , m_handle(h)
    , m_unpack_fn(unpack_fn)
    , m_free_fn(free_fn)
    , m_context(ctx) {
        hg_return_t ret = margo_ref_incr(h);
        MARGO_ASSERT(ret, margo_ref_incr);
    }

    packed_data() = default;

  public:
    ~packed_data() {
        if(m_handle != HG_HANDLE_NULL) {
            margo_destroy(m_handle);
        }
    }

    /**
     * @brief Create a new packed_data object but with a different
     * serialization context.
     *
     * @tparam NewCtxArg Types of the serialization context.
     * @param args Context.
     */
    template<typename ... NewCtxArg>
    auto with_serialization_context(NewCtxArg&&... args) {
        return packed_data<NewCtxArg...>(
            m_unpack_fn, m_free_fn, m_handle, m_engine_impl,
            std::make_tuple<NewCtxArg...>(std::forward<NewCtxArg>(args)...));
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
            return proc_object(proc, t, m_engine_impl, m_context);
        };
        hg_return_t ret = m_unpack_fn(m_handle, &mproc);
        MARGO_ASSERT(ret, m_unpack_fn);
        ret = margo_free_output(m_handle, &mproc);
        MARGO_ASSERT(ret, m_free_fn);
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
                   typename std::decay<Tn>::type...>
                     t;
        meta_proc_fn mproc = [this, &t](hg_proc_t proc) {
            return proc_object(proc, t, m_engine_impl, m_context);
        };
        hg_return_t ret = m_unpack_fn(m_handle, &mproc);
        MARGO_ASSERT(ret, m_unpack_fn);
        ret = m_free_fn(m_handle, &mproc);
        MARGO_ASSERT(ret, m_free_fn);
        return t;
    }

    /**
     * @brief Converts the content of the packed_data into
     * the desired object type. Allows to cast the packed_data
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
