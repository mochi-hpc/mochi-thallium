/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_TIMEOUT_HPP
#define __THALLIUM_TIMEOUT_HPP

#include <exception>
#include <stdexcept>

namespace thallium {

/**
 * @brief This exception is thrown when an RPC invoked with
 * timed() or timed_async times out.
 */
class timeout : public std::exception {
  public:
    virtual const char* what() const throw() { return "Request timed out"; }
};

} // namespace thallium

#endif
