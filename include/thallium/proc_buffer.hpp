#ifndef __THALLIUM_PROC_BUFFER_HPP
#define __THALLIUM_PROC_BUFFER_HPP

#include <vector>
#include <mercury_proc.h>
#include <thallium/buffer.hpp>

namespace thallium {

hg_return_t process_buffer(hg_proc_t proc, void* data);

} // namespace thallium

#endif
