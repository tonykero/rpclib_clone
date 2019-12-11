#include "server.hpp"

void prout(uint32_t a, uint32_t b) { std::cout << a + b << std::endl; };

int main() {
    
    Server srv(4242);

    srv.bind("prout", &prout);

    srv.run();
}