/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MARGO_EXCEPTION_HPP
#define __THALLIUM_MARGO_EXCEPTION_HPP

#include <exception>
#include <iostream>
#include <cstdlib>
#include <margo.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thallium/exception.hpp>

namespace thallium {

/**
 * @brief Exception class used when margo functions fail.
 */
class margo_exception : public exception {

    hg_return_t m_error_code;

  public:
    /**
     * @brief Constructor.
     *
     * @param function Name of the function that failed.
     * @param file Name of the file where the exception was thrown.
     * @param line Line in the file.
     * @param message Additional message.
     */
    margo_exception(const std::string& function, const std::string file,
                    unsigned line, hg_return_t ret,
                    const std::string& message = std::string())
    : exception("[", file, ":", line, "][", function, "] ", message)
    , m_error_code(ret) {}

    hg_return_t error() const {
        return m_error_code;
    }
};

inline const char* translate_margo_error_code(hg_return_t ret) {
    return HG_Error_to_string(ret);
}

#define MARGO_THROW(__fun__, __ret__, __msg__)                                 \
    do {                                                                       \
        throw margo_exception(#__fun__, __FILE__, __LINE__, __ret__, __msg__); \
    } while(0)

#define MARGO_ASSERT(__ret__, __fun__)                                         \
    do {                                                                       \
        if(__ret__ != HG_SUCCESS) {                                            \
            MARGO_THROW(__fun__, __ret__,                                      \
                        translate_margo_error_code(__ret__));                  \
        }                                                                      \
    } while(0)

#define MARGO_ASSERT_TERMINATE(__ret__, __fun__)                               \
    do {                                                                       \
        try { MARGO_ASSERT(__ret__, __fun__); }                                \
        catch(const margo_exception& ex) {                                     \
            std::cerr << "FATAL: " << ex.what() << std::endl;                  \
            std::terminate();                                                  \
        }                                                                      \
    } while(0)

#define THALLIUM_ASSERT_CONDITION(__cond__, __msg__)                           \
    do {                                                                       \
        if(!(__cond__)) {                                                      \
            std::cerr << "FATAL: Condition " << #__cond__ << " failed ("       \
                      << __FILE__ << __LINE__ << "), " << __msg__ << std::endl;\
            std::abort();                                                      \
        }                                                                      \
    } while(0)
} // namespace thallium

#endif
