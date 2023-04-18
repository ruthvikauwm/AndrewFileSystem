#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <unistd.h>
 
int main() {
    std::stringstream ss;
    ss << "HHello";
    std::string str;
    ss >> str;
    std::cout << ss.tellg() << std::endl;
    ss >> str;
    std::cout << ss.tellg() << std::endl;
}