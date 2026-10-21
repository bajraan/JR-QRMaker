#include "QrStyle.hpp"

#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace qrstyle {
namespace {
enum class Part { Body, EyeFrame, EyeBall };

Part classify(int x, int y, int size) {
    constexpr std::array<std::pair<int, int>, 3> origins{{{0, 0}, {-1, 0}, {0, -1}}};
    for (const auto [ox, oy] : origins) {
        const int left = ox == -1 ? size - 7 : ox;
        const int top = oy == -1 ? size - 7 : oy;
        const int localX = x - left;
        const int localY = y - top;
        if (localX < 0 || localX >= 7 || localY < 0 || localY >= 7) {
            continue;
        }
        if (localX == 0 || localX == 6 || localY == 0 || localY == 6) {
            return Part::EyeFrame;
        }
        if (localX >= 2 && localX <= 4 && localY >= 2 && localY <= 4) {
            return Part::EyeBall;
        }
    }
    return Part::Body;
}

std::string normalizeColor(std::string_view value) {
    if (!value.empty() && value.front() == '#') {
        value.remove_prefix(1);
    }
    if (value.size() != 6) {
        throw std::invalid_argument("QR colors must use six hexadecimal digits (RRGGBB)");
    }
    std::string result;
    result.reserve(7);
    result.push_back('#');
    for (const char ch : value) {
        if (!std::isxdigit(static_cast<unsigned char>(ch))) {
            throw std::invalid_argument("QR colors must use six hexadecimal digits (RRGGBB)");
        }
        result.push_back(ch);
    }
    return result;
}

template <typename Shape>
void appendTile(std::string& svg, int x, int y, int border, Shape shape, const std::string& color,
                float radius, float opacity) {
    const float left = static_cast<float>(x + border);
    const float top = static_cast<float>(y + border);
    if (shape == Shape::CIRCLES) {
        svg += "<circle cx=\"" + std::to_string(left + 0.5f) + "\" cy=\"" +
               std::to_string(top + 0.5f) + "\" r=\"0.48\" fill=\"" + color + "\" fill-opacity=\"" +
               std::to_string(opacity) + "\"/>\n";
    } else if (shape == Shape::ROUNDED) {
        svg += "<rect x=\"" + std::to_string(left) + "\" y=\"" + std::to_string(top) +
               "\" width=\"1\" height=\"1\" rx=\"" + std::to_string(radius) + "\" fill=\"" + color +
               "\" fill-opacity=\"" + std::to_string(opacity) + "\"/>\n";
    } else {
        svg += "<rect x=\"" + std::to_string(left) + "\" y=\"" + std::to_string(top) +
               "\" width=\"1\" height=\"1\" fill=\"" + color + "\" fill-opacity=\"" +
               std::to_string(opacity) + "\"/>\n";
    }
}

void appendRoundedPart(std::string& svg, const qrcodegen::QrCode& qr, int border, const std::string& color,
                       float opacity, Part part) {
    const int size = qr.getSize();
    svg += "<g fill=\"" + color + "\" opacity=\"" + std::to_string(opacity) + "\">\n";
    // Join neighboring modules with full-width strokes. Circular module centers
    // keep isolated cells round and smooth joins at straight runs and turns.
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (!qr.getModule(x, y) || classify(x, y, size) != part) {
                continue;
            }
            const float left = static_cast<float>(x + border);
            const float top = static_cast<float>(y + border);
            svg += "<circle cx=\"" + std::to_string(left + 0.5f) + "\" cy=\"" +
                   std::to_string(top + 0.5f) + "\" r=\"0.5\"/>\n";
            if (x + 1 < size && qr.getModule(x + 1, y) && classify(x + 1, y, size) == part) {
                svg += "<rect x=\"" + std::to_string(left + 0.5f) + "\" y=\"" +
                       std::to_string(top) + "\" width=\"1\" height=\"1\"/>\n";
            }
            if (y + 1 < size && qr.getModule(x, y + 1) && classify(x, y + 1, size) == part) {
                svg += "<rect x=\"" + std::to_string(left) + "\" y=\"" +
                       std::to_string(top + 0.5f) + "\" width=\"1\" height=\"1\"/>\n";
            }
        }
    }
    svg += "</g>\n";
}

template <typename Shape>
void appendEyeFrame(std::string& svg, int size, int border, Shape shape, const std::string& color, float radius,
                    float opacity) {
    if (shape == Shape::CIRCLE) {
        const std::array<std::pair<float, float>, 3> positions{{
            {3.5f + border, 3.5f + border},
            {static_cast<float>(size) - 3.5f + border, 3.5f + border},
            {3.5f + border, static_cast<float>(size) - 3.5f + border},
        }};
        for (const auto [cx, cy] : positions) {
            svg += "<circle cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) +
                   "\" r=\"3\" fill=\"none\" stroke=\"" + color + "\" stroke-opacity=\"" +
                   std::to_string(opacity) + "\" stroke-width=\"0.9\"/>\n";
        }
        return;
    }

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (classify(x, y, size) == Part::EyeFrame) {
                appendTile(svg, x, y, border, shape, color, radius, opacity);
            }
        }
    }
}

template <typename Shape>
void appendEyeBall(std::string& svg, int size, int border, Shape shape, const std::string& color, float radius,
                   float opacity) {
    if (shape == Shape::CIRCLE) {
        const std::array<std::pair<float, float>, 3> positions{{
            {3.5f + border, 3.5f + border},
            {static_cast<float>(size) - 3.5f + border, 3.5f + border},
            {3.5f + border, static_cast<float>(size) - 3.5f + border},
        }};
        for (const auto [cx, cy] : positions) {
            svg += "<circle cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) +
                   "\" r=\"1.5\" fill=\"" + color + "\" fill-opacity=\"" +
                   std::to_string(opacity) + "\"/>\n";
        }
        return;
    }

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (classify(x, y, size) == Part::EyeBall) {
                appendTile(svg, x, y, border, shape, color, radius, opacity);
            }
        }
    }
}
}  // namespace

std::string QrStyle::toSvgString(const qrcodegen::QrCode& qr, StyleShape shape, StyleColor color,
                                 int border, float cornerRadius) {
    if (border < 0) {
        throw std::invalid_argument("SVG border must be non-negative");
    }
    if (!std::isfinite(cornerRadius) || cornerRadius < 0.f || cornerRadius > 0.5f) {
        throw std::invalid_argument("QR corner radius must be between 0 and 0.5 modules");
    }
    const std::array<float, 4> opacities{{color.backgroundOpacity, color.bodyOpacity,
                                          color.eyeballOpacity, color.eyeframeOpacity}};
    for (const float opacity : opacities) {
        if (!std::isfinite(opacity) || opacity < 0.f || opacity > 1.f) {
            throw std::invalid_argument("QR region opacity must be between 0 and 1");
        }
    }

    const int size = qr.getSize();
    const int dimension = size + border * 2;
    const std::string background = normalizeColor(color.background);
    const std::string bodyColor = normalizeColor(color.body);
    const std::string eyeBallColor = normalizeColor(color.eyeball);
    const std::string eyeFrameColor = normalizeColor(color.eyeframe);

    std::string svg;
    svg.reserve(static_cast<std::size_t>(size * size * 100));
    svg += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    svg += std::to_string(dimension) + " " + std::to_string(dimension) + "\">\n";
    svg += "<rect width=\"100%\" height=\"100%\" fill=\"" + background + "\" fill-opacity=\"" +
           std::to_string(color.backgroundOpacity) + "\"/>\n";

    if (shape.bodystyle == body::ROUNDED) {
        appendRoundedPart(svg, qr, border, bodyColor, color.bodyOpacity, Part::Body);
    } else {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                if (qr.getModule(x, y) && classify(x, y, size) == Part::Body) {
                    appendTile(svg, x, y, border, shape.bodystyle, bodyColor, cornerRadius, color.bodyOpacity);
                }
            }
        }
    }
    if (shape.eyeframestyle == eyeframe::ROUNDED) {
        appendRoundedPart(svg, qr, border, eyeFrameColor, color.eyeframeOpacity, Part::EyeFrame);
    } else {
        appendEyeFrame(svg, size, border, shape.eyeframestyle, eyeFrameColor, cornerRadius, color.eyeframeOpacity);
    }
    if (shape.eyeballstyle == eyeball::ROUNDED) {
        appendRoundedPart(svg, qr, border, eyeBallColor, color.eyeballOpacity, Part::EyeBall);
    } else {
        appendEyeBall(svg, size, border, shape.eyeballstyle, eyeBallColor, cornerRadius, color.eyeballOpacity);
    }
    svg += "</svg>\n";
    return svg;
}

void QrStyle::writeFile(const qrcodegen::QrCode& qr, const std::string& path, StyleShape shape,
                        StyleColor color, int border, float cornerRadius) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + path);
    }
    out << toSvgString(qr, shape, std::move(color), border, cornerRadius);
    if (!out) {
        throw std::runtime_error("Failed to write output file: " + path);
    }
}

}  // namespace qrstyle
