#include <iostream>
#include <unistd.h>
#include <deque>
#include <mutex> // to use std::lock_guard
#include <algorithm>
#include <thallium.hpp>

#define NUM_XSTREAMS 1
#define NUM_THREADS  16

namespace tl = thallium;

class my_unit;
class my_pool;
class my_sched;

class my_unit {
       
    tl::thread          m_thread;
    tl::task            m_task;
    tl::unit_type m_type;
    bool                m_in_pool;

    friend class my_pool;

    public:
                
    my_unit(const tl::thread& t)
    : m_thread(t), m_type(tl::unit_type::thread), m_in_pool(false) {}

    my_unit(const tl::task& t)
    : m_task(t), m_type(tl::unit_type::task), m_in_pool(false) {}

    tl::unit_type get_type() const {
        return m_type;
    }

    const tl::thread& get_thread() const {
        return m_thread;
    }

    const tl::task& get_task() const {
        return m_task;
    }

    bool is_in_pool() const {
        return m_in_pool;
    }

    ~my_unit() {}
};

class my_pool {

    mutable tl::mutex    m_mutex;
    std::deque<my_unit*> m_units;
        
    public:

    static const tl::pool::access access_type = tl::pool::access::mpmc;
                
    my_pool() {}

    size_t get_size() const {
        std::lock_guard<tl::mutex> lock(m_mutex);
        return m_units.size();
    }

    void push(my_unit* u) {
        std::lock_guard<tl::mutex> lock(m_mutex);
        u->m_in_pool = true;
        m_units.push_back(u);
    }

    my_unit* pop() {
        std::lock_guard<tl::mutex> lock(m_mutex);
        if(m_units.empty())
            return nullptr;
        my_unit* u = m_units.front();
        m_units.pop_front();
        u->m_in_pool = false;
        return u;
    }

    void remove(my_unit* u) {
        std::lock_guard<tl::mutex> lock(m_mutex);
        auto it = std::find(m_units.begin(), m_units.end(), u);
        if(it != m_units.end()) {
            (*it)->m_in_pool = false;
            m_units.erase(it);
        }
    }
    
    ~my_pool() {
        std::cerr << "Pool destructor " << std::endl;
    }
};

class my_scheduler : private tl::scheduler {

    public:
                
    template<typename ... Args>
    my_scheduler(Args&&... args)
    : tl::scheduler(std::forward<Args>(args)...) {}
                    
    void run() {

        int n = num_pools();
        my_unit* unit;
        int target;
        unsigned seed = time(NULL);

        while (1) {
            /* Execute one work unit from the scheduler's pool */
            unit = get_pool(0).pop<my_unit>();
            if(unit != nullptr) {
                get_pool(0).run_unit(unit);
            } else if (n > 1) {
                /* Steal a work unit from other pools */
                target = (n == 2) ? 1 : (rand_r(&seed) % (n-1) + 1);
                unit = get_pool(target).pop<my_unit>();
                if(unit != nullptr)
                    get_pool(target).run_unit(unit);
            }
            
            if(has_to_stop()) break;

            tl::xstream::check_events(*this);
        }
    }
                        
    tl::pool get_migr_pool() const {
        return get_pool(0);
    }

    ~my_scheduler() {
        std::cerr << "scheduler destructor "<< std::endl;
    }
};

void hello() {
    tl::xstream es = tl::xstream::self();
    std::cout << "Hello World from ES " 
        << es.get_rank() << ", ULT " 
        << tl::thread::self_id() 
        << std::endl;
}

int main(int argc, char** argv) {

    tl::abt scope;

    // create pools
    std::vector<tl::managed<tl::pool>> pools;
    for(int i=0; i < NUM_XSTREAMS; i++) {
        pools.push_back(tl::pool::create<my_pool, my_unit>());
    }

    // create schedulers
    std::vector<tl::managed<tl::scheduler>> scheds;
    for(int i=0; i < NUM_XSTREAMS; i++) {
        std::vector<tl::pool> pools_for_sched_i;
        for(int j=0; j < pools.size(); j++) {
            pools_for_sched_i.push_back(*pools[j+i % pools.size()]);
        }
        scheds.push_back(tl::scheduler::create<my_scheduler>(pools_for_sched_i.begin(), pools_for_sched_i.end()));
    }

    std::vector<tl::managed<tl::xstream>> ess;

    for(int i=0; i < NUM_XSTREAMS; i++) {
        tl::managed<tl::xstream> es = tl::xstream::create(*scheds[i]);
        ess.push_back(std::move(es));
    }

    std::vector<tl::managed<tl::thread>> ths;
    for(int i=0; i < NUM_THREADS; i++) {
        tl::managed<tl::thread> th 
            = ess[i % ess.size()]->make_thread([]() {
                    hello();
        });
        ths.push_back(std::move(th));
    }

    for(auto& mth : ths) {
        mth->join();
    }

    for(int i=0; i < NUM_XSTREAMS; i++) {
        ess[i]->join();
    }

    ess.clear();
    scheds.clear();
    pools.clear();

    return 0;
}
