#ifndef __THALLIUM_PROC_BUFFER_HPP
#define __THALLIUM_PROC_BUFFER_HPP

#include <vector>
#include <mercury_proc.h>
#include <thallium/buffer.hpp>

namespace thallium {

namespace proc {

hg_return_t process_buffer(hg_proc_t proc, void* data, std::size_t& size);

} // namespace proc

} // namespace thallium

#endif
