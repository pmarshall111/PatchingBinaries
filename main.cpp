#include <iostream>
#include <unistd.h>
#include <string>

int main() {
    std::cout << "Enter a number, any number at all:" << std::endl;
    std::string line;
    std::getline(std::cin, line);
    int num = stoi(line);
    int threshold = 10;
    if (num > threshold) {
        std::cout << "Your number is above " << threshold << std::endl;
    } else {
        std::cout << "Your number is less than or equal to " << threshold << std::endl;
    }

    return 0;
}
