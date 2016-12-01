
#include <atomic>
#include <cstdio>

struct RefP {
        void *p;
        size_t ref;
};

int main() {
        std::atomic<RefP> p;
        p.store({nullptr, 0});
        RefP p2{nullptr, 0};
        RefP p3{nullptr, 1};

        bool ret = p.compare_exchange_strong(p2, p3);
        printf("ret = %d\n", ret);
        ret = p.compare_exchange_strong(p2, p3);
        printf("ret = %d\n", ret);

        return 0;

}

// g++ -std=c++11 -o ./test3 -mcx16 ./test3.cpp
