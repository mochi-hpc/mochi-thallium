/*
 * Copyright (c) 2022 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_LOGGER_HPP
#define __THALLIUM_LOGGER_HPP

#include <margo-logging.h>
#include <margo.h>
#include <thallium/exception.hpp>

namespace thallium {

class engine;

/**
 * @brief The logger class is an abstract class that users
 * can inherit from to provide custom logging capabilities to
 * the margo instance underlying a thallium engine.
 *
 * An pointer to a logger can be passed to engine::set_logger.
 */
class logger {

    friend class engine;

    public:

    enum class level : int {
        external = MARGO_LOG_EXTERNAL,
        trace    = MARGO_LOG_TRACE,
        debug    = MARGO_LOG_DEBUG,
        info     = MARGO_LOG_INFO,
        warning  = MARGO_LOG_WARNING,
        error    = MARGO_LOG_ERROR,
        critical = MARGO_LOG_CRITICAL
    };

    virtual void trace(const char* msg) const = 0;
    virtual void debug(const char* msg) const = 0;
    virtual void info(const char* msg) const = 0;
    virtual void warning(const char* msg) const = 0;
    virtual void error(const char* msg) const = 0;
    virtual void critical(const char* msg) const = 0;

    static void set_global_logger(logger* l) {
        margo_logger ml = {
            static_cast<void*>(l),
            &log_trace,
            &log_debug,
            &log_info,
            &log_warning,
            &log_error,
            &log_critical
        };
        if(0 != margo_set_global_logger(&ml)) {
            throw exception("Cannot set global logger");
        }
    }

    static void set_global_log_level(level l) {
        if(0 != margo_set_global_log_level(static_cast<margo_log_level>(l))) {
            throw exception("Cannot set global log level");
        }
    }

    private:

#define FORWARD_LOG(__level__) \
    static void log_##__level__(void* uargs, const char* msg) { \
        auto the_logger = static_cast<logger*>(uargs); \
        the_logger->__level__(msg); \
    }

    FORWARD_LOG(trace);
    FORWARD_LOG(debug);
    FORWARD_LOG(info);
    FORWARD_LOG(warning);
    FORWARD_LOG(error);
    FORWARD_LOG(critical);
};

}

#endif
