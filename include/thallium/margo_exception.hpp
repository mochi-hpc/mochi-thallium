/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MARGO_EXCEPTION_HPP
#define __THALLIUM_MARGO_EXCEPTION_HPP

#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>
#include <margo.h>

namespace thallium {

/**
 * @brief Exception class used when margo functions fail.
 */
class margo_exception : public std::exception {

private:

    std::ostringstream m_content;

public:

    /**
     * @brief Constructor.
     *
     * @param function Name of the function that failed.
     * @param file Name of the file where the exception was thrown.
     * @param line Line in the file.
     * @param message Additional message.
     */
    margo_exception(const std::string& function, const std::string file, unsigned line,
                    const std::string& message = std::string()) {
        m_content << "[" << file << ":" << line << "][" << function << "] "
                  << message;
    }

    /**
     * @brief Return the error message.
     *
     * @return error message associated with the exception.
     */
    virtual const char* what() const throw() {
        return m_content.str().c_str();
    }
};

std::string translate_margo_error_code(hg_return_t ret);

#define MARGO_THROW(__fun__,__msg__) do {\
    throw margo_exception(#__fun__,__FILE__,__LINE__,__msg__); \
    } while(0)

#define MARGO_ASSERT(__ret__, __fun__) do {\
    if(__ret__ != HG_SUCCESS) { \
        std::stringstream msg; \
        msg << "Function returned "; \
        msg << translate_margo_error_code(__ret__); \
        MARGO_THROW(__fun__, msg.str()); \
    }\
    } while(0)

#define MARGO_ASSERT_TERMINATE(__ret__, __fun__, __failcode__) do {\
    if(__ret__ != HG_SUCCESS) { \
        std::stringstream msg; \
        msg << "Function returned "; \
        msg << translate_margo_error_code(__ret__); \
        std::cerr << #__fun__ << ":" << __FILE__ << ":" << __LINE__ << ": " << msg.str(); \
        exit(__failcode__);\
    }\
    } while(0)

#define THALLIUM_ASSERT_CONDITION(__cond__, __msg__) do {\
    if(!(__cond__)) { \
        std::stringstream msg; \
        msg << "Condition " << #__cond__ << " failed (" << __FILE__ << __LINE__ \
            << ", " << __msg__; \
        throw std::runtime_error(msg.str()); \
    }\
    } while(0)
}

#endif
