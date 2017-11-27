/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/proc_buffer.hpp>

namespace thallium {

hg_return_t process_buffer(hg_proc_t proc, void* data) {
	buffer *vec = static_cast<buffer*>(data);
	std::size_t num_T = 0;
	hg_return_t hret;
	switch(hg_proc_get_op(proc)) {
		case HG_ENCODE:
			num_T = vec->size();
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			if (num_T > 0)
				hret = hg_proc_memcpy(proc, vec->data(), num_T);
			if (hret != HG_SUCCESS) return hret;
            break;
		case HG_DECODE:
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			if (num_T > 0) {
				vec->resize(num_T);
				hret = hg_proc_memcpy(proc, vec->data(), num_T);
				if (hret != HG_SUCCESS) return hret;
			}
			break;
		case HG_FREE:
			return HG_SUCCESS;
	} 
	return HG_SUCCESS;
}

} // namespace thallium

