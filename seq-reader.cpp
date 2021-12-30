#include <iostream>
#include <fstream>
#include <string>
#include <vector>

constexpr long long D64_FILE_SIZE = 174848;
constexpr long long BAM_AREA_START = 0x16500;

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

            is.close();

            return count;
        }
};

int countOnBits(unsigned char a) {
    int c = 0;
    while (a) {
        c += a & 1;
        a >>= 1;
    }
    return c;
}

int main(int argc, char** argv) {
    std::string filename;
    std::cin >> filename;
    
    long long fileSize = FileHandler::getLength(filename);
    if (fileSize != D64_FILE_SIZE) {
        std::cout << "Nem támogatott fájltípus! (fájlméret nem " << D64_FILE_SIZE << ", hanem " << fileSize << ")" << std::endl;
        return 1;
    }

    std::ifstream is(filename, std::ios::in | std::ios::binary);

    is.seekg(BAM_AREA_START, std::ios::beg);

    std::vector<unsigned char> bam(256);
    for (int i = 0; i < 256; i++) {
        bam[i] = is.get();
    }

    if (bam[0] != 18 || bam[1] != 1) {
        std::cout << "Warning: First direntry misaligned to " << bam[0] << "/" << bam[1] << " (Track/Sector in decimal)" << std::endl;
    }

    std::cout << "DOS Version: ";
    if (bam[2] == 0x41) {
        std::cout << "CBM DOS V 2.6 1541 (";
    } else {
        std::cout << "unknown (";
    }
    std::cout << bam[2] << ")" << std::endl;

    int freeSectors = 0;
    for (int i = 0; i < 35; i++) {
        freeSectors += bam[i * 4 + 4];
        // bitek számolásával bonyolultabban is meglehet oldani
        /*freeSectors += countOnBits(bam[i * 4 + 5]) + countOnBits(bam[i * 4 + 6]);
        if (i <= 17) {
            freeSectors += countOnBits(bam[i * 4 + 7] & 0b00011111);
        } else if (i <= 24) {
            freeSectors += countOnBits(bam[i * 4 + 7] & 0b00000111);
        } else if (i <= 30) {
            freeSectors += countOnBits(bam[i * 4 + 7] & 0b00000011);
        } else {
            freeSectors += countOnBits(bam[i * 4 + 7] & 0b00000001);
        }*/
    }

    std::cout << freeSectors << " szabad szektor" << std::endl;
    std::cout << (freeSectors - bam[18 * 4]) << " blocks free" << std::endl; // directory szektor (18-as) nélkül (a C64-en is így láthatjuk a számot ha minden igaz)
    

    // TODO: a PETSCII betűkre valami mappingot kéne készíteni
    std::string diskName;
    for (int i = 0; i < 16; i++) {
        if (bam[i + 0x90] == 0xA0) break; // remove padding
        diskName += bam[i + 0x90];
    }

    std::cout << "Disk name: \"" << diskName << "\"" << std::endl;

    std::string diskID(bam.begin() + 0xA2, bam.begin() + 0xA7);
    std::cout << "Disk ID: \"" << diskID << "\"" << std::endl;
}
