#include <iostream>
#include <fstream>
#include <string>

constexpr long long FILE_SIZE = 174848;

class FileHandler {
    public:
        // returns -1 if file cannot be opened
        static long long getLength(std::string const& filename) {
            std::ifstream is(filename, std::ios::in | std::ios::binary);

            if (!is.is_open()) {
                return -1;
            }

            long long count = 0;
            char buf;
            while (is.get(buf)) {
                count++;
            }
            return count;
        }
};

int main(int argc, char** argv) {
    std::string filename;
    std::cin >> filename;
    std::cout << FileHandler::getLength(filename) << std::endl;
}
