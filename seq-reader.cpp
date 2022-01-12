#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

constexpr long long D64_FILE_SIZE = 174848;
constexpr long long BAM_AREA_START = 0x16500;

std::vector<std::string> fileTypes{"DEL", "SEQ", "PRG", "USR", "REL"};
constexpr int DEL = 0, SEQ = 1, PRG = 2, USR = 3, REL = 4;

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

struct TrackSector {
    int track, sector;
};

struct DirEntry {
    TrackSector firstSector;
    int fileType;
    bool locked, closed;
    std::string fileName;
    int fileSize;
};

int countOnBits(unsigned char a) {
    int c = 0;
    while (a) {
        c += a & 1;
        a >>= 1;
    }
    return c;
}

int posInImage(int track, int sector) {
    if (track <= 17) { // 21 sectors per track
        return (track - 1) * 21 * 256 + sector * 256;
    } else if (track <= 24) { // 19 sectors per track
        return 0x16500 + (track - 18) * 19 * 256 + sector * 256;
    } else if (track <= 30) { // 18 sectors per track
        return 0x1EA00 + (track - 25) * 18 * 256 + sector * 256;
    } else { // 17 sectors per track
        return 0x25600 + (track - 31) * 17 * 256 + sector * 256;
    }
}

void posInImageTest() {
    int prev = -256, perTrack = 21;
    for (int track = 1; track <= 35; track++) {
        if (track == 18) {
            perTrack = 19;
        } else if (track == 25) {
            perTrack = 18;
        } else if (track == 31) {
            perTrack = 17;
        }
        for (int sector = 0; sector < perTrack; sector++) {
            int val = posInImage(track, sector);
            if (prev + 256 != val) {
                std::cerr << "Wrong at " << track << "/" << sector << std::endl;
            }
            prev = val;
        }
    }
}

int main(int argc, char** argv) {
    std::string filename;
    std::cin >> std::ws;
    std::getline(std::cin, filename);
    
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
        std::cout << "Warning: First direntry misaligned to " << (int) bam[0] << "/" << (int) bam[1] << " (Track/Sector in decimal)" << std::endl;
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
    std::cout << (freeSectors - bam[18 * 4]) << " blocks free" << std::endl; // directory track (18-as) nélkül (a C64-en is így láthatjuk a számot ha minden igaz)
    

    // TODO: a PETSCII betűkre valami mappingot kéne készíteni
    std::string diskName;
    for (int i = 0; i < 16; i++) {
        if (bam[i + 0x90] == 0xA0) break; // remove padding
        diskName += bam[i + 0x90];
    }

    std::cout << "Disk name: \"" << diskName << "\"" << std::endl;

    std::string diskID(bam.begin() + 0xA2, bam.begin() + 0xA7);
    std::cout << "Disk ID: \"" << diskID << "\"" << std::endl;

    std::cout << "Directory:" << std::endl;

    std::vector<DirEntry> directory;
    std::vector<unsigned char> dir(256); // TODO: rename
    int track = 18, sector = 1;
    do {
        is.seekg(posInImage(track, sector), std::ios::beg);
        for (int i = 0; i < 256; i++) {
            dir[i] = is.get();
        }

        track = dir[0];
        sector = dir[1];

        for (int i = 0; i < 256; i += 32) {
            DirEntry entry;
            entry.fileSize = dir[i + 0x1E] + dir[i + 0x1F] * 256;
            std::cout << std::setw(3) << std::left << entry.fileSize << "  \""; // SD2IEC esetében a 4 számjegyű számok után elcsúszik a szöveg

            for (int j = 0; j < 16; j++) {
                if (dir[i + 5 + j] == 0xA0) break; // remove padding
                entry.fileName += (char) dir[i + 5 + j];
            }

            std::cout << entry.fileName << "\"";
            for (int i = entry.fileName.size(); i < 17; i++) {
                std::cout << " ";
            }

            entry.locked = dir[i + 2] & 0b01000000;
            entry.closed = dir[i + 2] & 0b10000000;
            std::cout << "* "[entry.closed];

            entry.fileType = dir[i + 2] & 0b1111;
            if (entry.fileType <= 0b100) {
                std::cout << fileTypes[entry.fileType];
            } else {
                std::cout << "???";
            }

            std::cout << " <"[entry.locked];

            entry.firstSector.track = dir[i + 3];
            entry.firstSector.sector = dir[i + 4];

            std::cout << "   (Starts at " << entry.firstSector.track << "/" << entry.firstSector.sector << ")" << std::endl;

            directory.push_back(entry);
        }
    } while (track != 0);


    for (const DirEntry& entry : directory) {
        if (!(entry.fileType == SEQ || entry.fileType == PRG)) continue;

        std::cout << "Export \"" << entry.fileName << "\" ";
        for (int i = entry.fileName.size(); i < 16; i++) {
            std::cout << " ";
        }
        std::cout << "* "[entry.closed];

        if (entry.fileType <= 0b100) {
            std::cout << fileTypes[entry.fileType];
        } else {
            std::cout << "???";
        }

        std::cout << " <"[entry.locked] << " ? (Y/N) ";
        char yesorno ;
        std::cin >> yesorno;

        if (yesorno == 'Y' || yesorno == 'y') {
            if (entry.fileType == SEQ || entry.fileType == PRG) {
                std::string exportName;
                std::cout << "File name? ";
                std::cin >> std::ws;
                std::getline(std::cin, exportName);
                std::ofstream os(exportName, std::ios::out | std::ios::binary);

                char buf[256];
                unsigned track = entry.firstSector.track, sector = entry.firstSector.sector;
                do {
                    is.seekg(posInImage(track, sector), std::ios::beg);

                    is.readsome(buf, 256);
                    track = buf[0] & 0xFF, sector = buf[1] & 0xFF;
                    std::cout << track << ", " << sector << "   ";

                    if (track) {
                        os.write(buf + 2, 254);
                    } else {
                        os.write(buf + 2, sector);
                    }
                } while (track);
        
                os.close();

                std::cout << "Exported to " << exportName << std::endl;
            }
        }
    }

    is.close();
}
