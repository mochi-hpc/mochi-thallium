/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_ABT_HPP
#define __THALLIUM_ABT_HPP

#include <thallium/abt_errors.hpp>
#include <thallium/exception.hpp>

namespace thallium {

/**
 * Exception class thrown by the abt class.
 */
class abt_exception : public exception {
  public:
    template <typename... Args>
    abt_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_ABT_EXCEPTION(__fun, __ret)                                         \
    abt_exception(#__fun, " returned ", abt_error_get_name(__ret), " (",       \
                  abt_error_get_description(__ret), ") in ", __FILE__, ":",    \
                  __LINE__)

#define TL_ABT_ASSERT(__call)                                                  \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_ABT_EXCEPTION(__call, __ret);                             \
        }                                                                      \
    }

class abt {
  public:
    abt() { initialize(); }

    ~abt() { finalize(); }

    /**
     * @brief Initialize the Argobots execution environment.
     */
    static void initialize() { TL_ABT_ASSERT(ABT_init(0, nullptr)); }

    /**
     * @brief Check whether Argobots has been initialized.
     *
     * @return true if Argobots has been initialized.
     */
    static bool initialized() { return ABT_initialized() == ABT_TRUE; }

    /**
     * @brief Finalizes Argobots.
     */
    static void finalize() { TL_ABT_ASSERT(ABT_finalize()); }
};

} // namespace thallium

#undef TL_ABT_EXCEPTION
#undef TL_ABT_ASSERT

#endif
