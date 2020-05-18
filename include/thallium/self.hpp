/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_SELF_HPP
#define __THALLIUM_SELF_HPP

#include <abt.h>
#include <thallium/abt_errors.hpp>
#include <thallium/exception.hpp>
#include <thallium/unit_type.hpp>

namespace thallium {

/**
 * Exception class thrown by the xstream_barrier class.
 */
class self_exception : public exception {
  public:
    template <typename... Args>
    self_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_SELF_EXCEPTION(__fun, __ret)                                        \
    self_exception(#__fun, " returned ", abt_error_get_name(__ret), " (",      \
                   abt_error_get_description(__ret), ") in ", __FILE__, ":",   \
                   __LINE__);

#define TL_SELF_ASSERT(__call)                                                 \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_SELF_EXCEPTION(__call, __ret);                            \
        }                                                                      \
    }

/**
 * @brief Wrapper for Argobots' ABT_xstream_barrier.
 */
class self {
  public:
    self()  = delete;
    ~self() = delete;

    /**
     * @brief Return the type of calling work unit.
     *
     * @return the type of calling work unit.
     */
    static unit_type get_unit_type() {
        ABT_unit_type type;
        TL_SELF_ASSERT(ABT_self_get_type(&type));
        return (unit_type)type;
    }

    /**
     * @brief Check if the caller is the primary ULT.
     *
     * @return true if the caller is the primary ULT.
     */
    static bool is_primary() {
        ABT_bool flag;
        TL_SELF_ASSERT(ABT_self_is_primary(&flag));
        return flag == ABT_TRUE;
    }

    /**
     * @brief Check if the caller's ES is the primary ES.
     *
     * @return true if the caller's ES is the primary ES.
     */
    static bool on_primary_xstream() {
        ABT_bool flag;
        TL_SELF_ASSERT(ABT_self_on_primary_xstream(&flag));
        return flag == ABT_TRUE;
    }

    /**
     * @brief Get the last pool's ID of calling work unit.
     *
     * @return the last pool's ID of calling work unit.
     */
    static int get_last_pool_id() {
        int pool_id;
        TL_SELF_ASSERT(ABT_self_get_last_pool_id(&pool_id));
        return pool_id;
    }

    /**
     * @brief Suspend the current ULT.
     */
    static void suspend() { TL_SELF_ASSERT(ABT_self_suspend()); }
};

} // namespace thallium

#undef TL_SELF_EXCEPTION
#undef TL_SELF_ASSERT

#endif /* end of include guard */
