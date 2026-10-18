#pragma once

#include "qrcodegen.hpp"

#include <string>

namespace svggen {

class SvgWriter {
public:
    // Quiet zone in modules around the QR (ISO recommends at least 4).
    static constexpr int kDefaultBorder = 4;

    static std::string toSvgString(const qrcodegen::QrCode& qr, int border = kDefaultBorder);

    static void writeFile(const qrcodegen::QrCode& qr,
                          const std::string& path,
                          int border = kDefaultBorder);
};

}  // namespace svggen
