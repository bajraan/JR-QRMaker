#include "SvgWriter.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace svggen {

std::string SvgWriter::toSvgString(const qrcodegen::QrCode& qr, int border) {
    if (border < 0) {
        throw std::invalid_argument("SVG border must be non-negative");
    }

    const int size = qr.getSize();
    const int dimension = size + border * 2;

    std::ostringstream svg;
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
        << "viewBox=\"0 0 " << dimension << " " << dimension << "\" "
        << "shape-rendering=\"crispEdges\">\n"
        << "  <rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n"
        << "  <path fill=\"#000000\" d=\"";

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y)) {
                svg << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }

    svg << "\"/>\n</svg>\n";
    return svg.str();
}

void SvgWriter::writeFile(const qrcodegen::QrCode& qr,
                          const std::string& path,
                          int border) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + path);
    }

    out << toSvgString(qr, border);
    if (!out) {
        throw std::runtime_error("Failed to write SVG file: " + path);
    }
}

}  // namespace svggen
