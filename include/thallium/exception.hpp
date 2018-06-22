/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_EXCEPTION_HPP
#define __THALLIUM_EXCEPTION_HPP

#include <exception>
#include <sstream>

namespace thallium {

class exception {

    std::string m_msg;

    public:

    template<typename Arg, typename ... Args>
    exception(Arg&& a, Args&&... args) {
        std::stringstream ss;
        ss << std::forward<Arg>(a);
        using expander = int[];
        (void)expander{0, (void(ss << std::forward<Args>(args)),0)...};
        m_msg = ss.str();
    }

    const char* what() const throw ()
    {
        return m_msg.c_str();
    }
};

}

#endif /* end of include guard */
