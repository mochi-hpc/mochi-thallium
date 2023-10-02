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
#include <thallium/reference_util.hpp>

namespace thallium {

template<typename ... CtxArg> class callable_remote_procedure_with_context;
class async_response;
template<typename ... CtxArg> class request_with_context;
using request = request_with_context<>;

namespace detail {
    struct engine_impl;
}

/**
 * @brief packed_data objects encapsulate data serialized
 * into an hg_handle_t, whether it is input or output data.
 */
template<typename ... CtxArg>
class packed_data {
    friend class async_response;
    template<typename ... CtxArg2> friend class callable_remote_procedure_with_context;
    template<typename ... CtxArg2> friend class request_with_context;
    template<typename ... CtxArg2> friend class packed_data;;

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
                std::tuple<CtxArg...>&& ctx = std::tuple<CtxArg...>())
    : m_engine_impl(std::move(e))
    , m_handle(h)
    , m_unpack_fn(unpack_fn)
    , m_free_fn(free_fn)
    , m_context(std::move(ctx)) {
        hg_return_t ret = margo_ref_incr(h);
        MARGO_ASSERT(ret, margo_ref_incr);
    }

  public:
    packed_data() = default;
    packed_data(const packed_data&)            = delete;
    packed_data& operator=(const packed_data&) = delete;

    packed_data(packed_data&& rhs)
    : m_engine_impl(std::move(rhs.m_engine_impl)),
      m_context(std::move(rhs.m_context)) {
        m_handle        = rhs.m_handle;
        rhs.m_handle    = HG_HANDLE_NULL;
        m_unpack_fn     = rhs.m_unpack_fn;
        rhs.m_unpack_fn = nullptr;
        m_free_fn       = rhs.m_free_fn;
        rhs.m_free_fn   = nullptr;
    }

    packed_data& operator=(packed_data&& rhs) {

        if(&rhs == this) {
            return *this;
        }

        // the original members m_handle, m_context, and m_handle are being
        // replaced here by the ones from rhs. It may be necessary to release
        // their resources if `packed_data` has claimed ownership over them,
        // otherwise we would be leaking
        m_engine_impl = std::move(rhs.m_engine_impl);
        m_context = std::move(rhs.m_context);

        m_handle        = rhs.m_handle;
        rhs.m_handle    = HG_HANDLE_NULL;
        m_unpack_fn     = rhs.m_unpack_fn;
        rhs.m_unpack_fn = nullptr;
        m_free_fn       = rhs.m_free_fn;
        rhs.m_free_fn   = nullptr;
    }

    ~packed_data() {
        if(m_handle != HG_HANDLE_NULL) {
            margo_destroy(m_handle);
        }
    }

    /**
     * @brief Return the request's underlying hg_handle_t.
     */
    hg_handle_t native_handle() const {
        return m_handle;
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
        return packed_data<unwrap_decay_t<NewCtxArg>...>(
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
            return proc_object_decode(proc, t, m_engine_impl, m_context);
        };
        hg_return_t ret = m_unpack_fn(m_handle, &mproc);
        MARGO_ASSERT(ret, m_unpack_fn);
        ret = margo_free_output(m_handle, &mproc);
        MARGO_ASSERT(ret, m_free_fn);
        return std::get<0>(std::move(t));
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
            return proc_object_decode(proc, t, m_engine_impl, m_context);
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

    /**
     * @brief Unpack the data into a provided set of arguments.
     * This function is preferable to as<T> or to casting into
     * the resulting type in cases where the resulting type
     * cannot be easily moved, or the caller already has an instance
     * of it that needs to be set.
     *
     * @tparam T Types into which to unpack.
     * @param x Objects into which to unpack.
     */
    template <typename... T> void unpack(T&... x) const {
        if(m_handle == HG_HANDLE_NULL) {
            throw exception(
                "Cannot unpack data from handle. Are you trying to "
                "unpack data from an RPC that does not return any?");
        }
        auto t = std::make_tuple(std::ref(x)...);
        meta_proc_fn mproc = [this, &t](hg_proc_t proc) {
            return proc_object_decode(proc, t, m_engine_impl, m_context);
        };
        hg_return_t ret = m_unpack_fn(m_handle, &mproc);
        MARGO_ASSERT(ret, m_unpack_fn);
        ret = m_free_fn(m_handle, &mproc);
        MARGO_ASSERT(ret, m_free_fn);
    }
};

} // namespace thallium

#endif
