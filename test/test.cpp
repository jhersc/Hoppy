#include <iostream>
#include <string>

// #include "global_objects.h"
// #include "DebugMacros.h"


int main() {

    std::string raw = "msg||2024-06-01 12:00:00||msg123||nodeA||chan1||Node A||Channel 1||Hello, World!";
    std::cout << raw.substr(5) << std::endl;
    std::cout << raw << std::endl;
    return 0;
}
