#include <thallium/proc_buffer.hpp>

namespace thallium {

namespace proc {

hg_return_t process_buffer(hg_proc_t proc, void* data, std::size_t& size) {
	buffer *vec = static_cast<buffer*>(data);
	std::size_t num_T = 0;
	size = 0;
	hg_return_t hret;
	switch(hg_proc_get_op(proc)) {
		case HG_ENCODE:
			num_T = vec->size();
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			size += sizeof(num_T);
			if (num_T > 0)
				hret = hg_proc_memcpy(proc, vec->data(), num_T);
			if (hret != HG_SUCCESS) return hret;
			size += num_T;
            break;
		case HG_DECODE:
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			size += sizeof(num_T);
			if (num_T > 0) {
				vec->resize(num_T);
				hret = hg_proc_memcpy(proc, vec->data(), num_T);
				if (hret != HG_SUCCESS) return hret;
				size += num_T;
			}
			break;
		case HG_FREE:
			return HG_SUCCESS;
	} 
	return HG_SUCCESS;
}

} // namespace proc

} // namespace thallium

