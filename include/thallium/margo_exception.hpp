/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MARGO_EXCEPTION_HPP
#define __THALLIUM_MARGO_EXCEPTION_HPP

#include <exception>
#include <iostream>
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
                    unsigned line, const std::string& message = std::string())
    : exception("[", file, ":", line, "][", function, "] ", message) {}
};

inline const char* translate_margo_error_code(hg_return_t ret) {
#ifdef HG_RETURN_VALUES
#define X(a) #a,
    static const char* const error_codes[] = {
        HG_RETURN_VALUES
    };
#undef X
    if(ret < HG_RETURN_MAX) {
        return error_codes[ret];
    }
#else
    switch(ret) {
    case HG_SUCCESS: /*!< operation succeeded */
        return "HG_SUCCESS";
    case HG_NA_ERROR: /*!< error in NA layer */
        return "HG_NA_ERROR";
    case HG_TIMEOUT: /*!< reached timeout */
        return "HG_TIMEOUT";
    case HG_INVALID_PARAM: /*!< invalid parameter */
        return "HG_INVALID_PARAM";
    case HG_SIZE_ERROR: /*!< size error */
        return "HG_SIZE_ERROR";
    case HG_NOMEM_ERROR: /*!< no memory error */
        return "HG_NOMEM_ERROR";
    case HG_PROTOCOL_ERROR: /*!< protocol does not match */
        return "HG_PROTOCOL_ERROR";
    case HG_NO_MATCH: /*!< no function match */
        return "HG_NO_MATCH";
    case HG_CHECKSUM_ERROR: /*!< checksum error */
        return "HG_CHECKSUM_ERROR";
    case HG_CANCELED: /*!< operation was canceled */
        return "HG_CANCELED";
    case HG_OTHER_ERROR: /*!< error from mercury_util or external to mercury */
        return "HG_OTHER_ERROR";
    }
#endif
    return "Unknown error";
}

#define MARGO_THROW(__fun__, __msg__)                                          \
    do {                                                                       \
        throw margo_exception(#__fun__, __FILE__, __LINE__, __msg__);          \
    } while(0)

#define MARGO_ASSERT(__ret__, __fun__)                                         \
    do {                                                                       \
        if(__ret__ != HG_SUCCESS) {                                            \
            std::stringstream msg;                                             \
            msg << "Function returned ";                                       \
            msg << translate_margo_error_code(__ret__);                        \
            std::cerr << msg.str() << std::endl;                               \
            MARGO_THROW(__fun__, msg.str());                                   \
        }                                                                      \
    } while(0)

#define MARGO_ASSERT_TERMINATE(__ret__, __fun__, __failcode__)                 \
    do {                                                                       \
        if(__ret__ != HG_SUCCESS) {                                            \
            std::stringstream msg;                                             \
            msg << "Function returned ";                                       \
            msg << translate_margo_error_code(__ret__);                        \
            std::cerr << #__fun__ << ":" << __FILE__ << ":" << __LINE__        \
                      << ": " << msg.str();                                    \
            exit(__failcode__);                                                \
        }                                                                      \
    } while(0)

#define THALLIUM_ASSERT_CONDITION(__cond__, __msg__)                           \
    do {                                                                       \
        if(!(__cond__)) {                                                      \
            std::cerr << "Condition " << #__cond__ << " failed (" << __FILE__  \
                      << __LINE__ << "), " << __msg__ << std::endl;            \
            exit(-1);                                                          \
        }                                                                      \
    } while(0)
} // namespace thallium

#endif
