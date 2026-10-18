#include <iostream>
#include <string>

#include "SvgWriter.hpp"
#include "qrcodegen.hpp"

namespace {

void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " <url> [output.svg]\n"
              << "  url         Text/URL encoded into the QR code\n"
              << "  output.svg  Optional output path (default: qrcode.svg)\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0] != nullptr ? argv[0] : "JR-QRMaker");
        return 1;
    }

    const std::string text = argv[1];
    if (text.empty()) {
        std::cerr << "[ERROR] URL/text argument must not be empty.\n";
        printUsage(argv[0]);
        return 1;
    }

    const std::string outputPath = (argc >= 3 && argv[2][0] != '\0')
                                       ? argv[2]
                                       : std::string("qrcode.svg");

    try {
        const qrcodegen::QrCode qr =
            qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);

        svggen::SvgWriter::writeFile(qr, outputPath);

        std::cout << "[INFO] Encoded: " << text << '\n'
                  << "[INFO] QR size: " << qr.getSize() << " modules "
                  << "(version " << qr.getVersion() << ")\n"
                  << "[INFO] Wrote: " << outputPath << '\n';
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }

    return 0;
}
