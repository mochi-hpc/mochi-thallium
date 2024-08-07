#include <iostream>

class encoder {

    public:

    template<typename A>
    void save(A& ar) const {
        auto buffer = static_cast<char*>(ar.save_ptr(26));
        for(int i = 0; i < 26; ++i) {
            buffer[i] = 'A' + i;
        }
        ar.restore_ptr(buffer, 26);
    }

    template<typename A>
    void load(A& ar) {
        auto buffer = static_cast<char*>(ar.save_ptr(26));
        for(int i = 0; i < 26; ++i) {
            std::cout << buffer[i];
        }
        std::cout << std::endl;
        ar.restore_ptr(buffer, 26);
    }
};
