/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REMOTE_BULK_HPP
#define __THALLIUM_REMOTE_BULK_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <margo.h>
#include <thallium/bulk.hpp>

namespace thallium {

class remote_bulk {

    friend class bulk;

private:

    const bulk::bulk_segment& m_segment;
    endpoint                  m_endpoint;

	remote_bulk(const bulk::bulk_segment& b, const endpoint& ep)
	: m_segment(b), m_endpoint(ep) {}

public:

    std::size_t operator>>(const bulk::bulk_segment& dest) const;

    std::size_t operator<<(const bulk::bulk_segment& src) const;
};

}

#endif
