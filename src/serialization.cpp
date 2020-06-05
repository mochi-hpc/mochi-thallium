#include "thallium/serialization/proc_input_archive.hpp"
#include "thallium/serialization/proc_output_archive.hpp"
#include "thallium/engine.hpp"

namespace thallium {

    engine proc_input_archive::get_engine() const {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        return engine(engine_impl);
    }

    engine proc_output_archive::get_engine() const {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        return engine(engine_impl);
    }

}
