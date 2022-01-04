/*
 * Megnézi, hogy egy std::string-ben 0-255-ig mindent lehet-e tárolni
 */

#include <iostream>
#include <string>

int main() {
    std::string s(" ");
    bool good = true;

    for (int i = 0; i < 256; i++) {
        s[0] = i;
        if (s[0] != (char) i) {
            good = false;
            std::cout << i << " gives " << (int) s[0] << " back..." << std::endl;
        }
    }

    if (good) {
        std::cout << "Evenrything is fine with std::string" << std::endl;
    }
}
