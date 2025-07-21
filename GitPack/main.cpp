#include <iostream>
#include "packer.h"
#include "unpacker.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage:\n";
        std::cout << "  gitpack pack <folder>\n";
        std::cout << "  gitpack unpack <file.gpk>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "pack") {
        return create_packfile(argv[2], "build/output/packed.gpk");
    } else if (command == "unpack") {
        return extract_packfile(argv[2], "build/output/unpacked/");
    }

    std::cerr << "Unknown command: " << command << "\n";
    return 1;
}
