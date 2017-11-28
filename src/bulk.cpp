/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/bulk.hpp>
#include <thallium/resolved_bulk.hpp>

namespace thallium {

bulk::bulk_segment bulk::select(std::size_t offset, std::size_t size) const {
    return bulk_segment(*this, offset, size);
}

bulk::bulk_segment bulk::operator()(std::size_t offset, std::size_t size) const {
    return select(offset, size);
}

resolved_bulk bulk::bulk_segment::on(const endpoint& ep) const {
    return resolved_bulk(*this, ep);
}

}
