#include "thallium/serialization/proc_input_archive.hpp"
#include "thallium/serialization/proc_output_archive.hpp"
#include "thallium/engine.hpp"

namespace thallium {

    engine proc_input_archive::get_engine() const {
        // TODO check if weak_ptr is valid
        return engine(m_engine_impl.lock());
    }

    engine proc_output_archive::get_engine() const {
        // TODO check if weak_ptr is valid
        return engine(m_engine_impl.lock());
    }

}
