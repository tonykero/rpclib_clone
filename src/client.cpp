#include "client.hpp"

int main() {


    Client cl("0.0.0.0", 4242);
    cl.call("prout", 2, 3);

    cl.close();
}