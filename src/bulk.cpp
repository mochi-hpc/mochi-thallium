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

resolved_bulk bulk::on(const endpoint& ep) const {
    return resolved_bulk(*this, ep);
}

std::size_t bulk::bulk_segment::operator>>(const resolved_bulk& b) const {
    return b << *this;
}

std::size_t bulk::bulk_segment::operator<<(const resolved_bulk& b) const {
    return b >> *this;
}

std::size_t bulk::operator>>(const resolved_bulk& b) const {
    return b << (this->select(0,size()));
}

std::size_t bulk::operator<<(const resolved_bulk& b) const {
    return b >> (this->select(0,size()));
}
}
