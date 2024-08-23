/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP
#define __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP

#include <thallium/serialization/cereal/archives.hpp>
#include <thallium/engine.hpp>

namespace thallium {

    template<typename... CtxArg>
    inline engine proc_input_archive<CtxArg...>::get_engine() const {
        return engine(m_mid);
    }

}

#endif
