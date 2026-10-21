#pragma once

#include "qrcodegen.hpp"

#include <string>
#include <utility>

namespace qrstyle {

class QrStyle {
public:
    enum class body { SQUARE, ROUNDED, CIRCLES };
    enum class eyeball { SQUARE, ROUNDED, CIRCLE, CIRCLES };
    enum class eyeframe { SQUARE, ROUNDED, CIRCLE, CIRCLES };

    struct StyleShape {
        body bodystyle;
        eyeball eyeballstyle;
        eyeframe eyeframestyle;

        constexpr StyleShape(body bodyStyle = body::CIRCLES, eyeball ballStyle = eyeball::CIRCLES,
                             eyeframe frameStyle = eyeframe::CIRCLES)
            : bodystyle(bodyStyle), eyeballstyle(ballStyle), eyeframestyle(frameStyle) {}
    };

    struct StyleColor {
        std::string background;
        std::string body;
        std::string eyeball;
        std::string eyeframe;
        float backgroundOpacity;
        float bodyOpacity;
        float eyeballOpacity;
        float eyeframeOpacity;

        StyleColor(std::string backgroundColor = "FFFFFF", std::string bodyColor = "D35400",
                   std::string eyeBallColor = "D35400", std::string eyeFrameColor = "D35400",
                   float backgroundAlpha = 1.f, float bodyAlpha = 1.f, float eyeBallAlpha = 1.f,
                   float eyeFrameAlpha = 1.f)
            : background(std::move(backgroundColor)), body(std::move(bodyColor)),
              eyeball(std::move(eyeBallColor)), eyeframe(std::move(eyeFrameColor)),
              backgroundOpacity(backgroundAlpha), bodyOpacity(bodyAlpha),
              eyeballOpacity(eyeBallAlpha), eyeframeOpacity(eyeFrameAlpha) {}
    };

    static constexpr int kDefaultBorder = 4;
    static constexpr float kDefaultCornerRadius = 0.2f;

    static std::string toSvgString(const qrcodegen::QrCode& qr,
                                   StyleShape shape = StyleShape(),
                                   StyleColor color = StyleColor(),
                                   int border = kDefaultBorder,
                                   float cornerRadius = kDefaultCornerRadius);
    static void writeFile(const qrcodegen::QrCode& qr,
                          const std::string& path,
                          StyleShape shape = StyleShape(),
                          StyleColor color = StyleColor(),
                          int border = kDefaultBorder,
                          float cornerRadius = kDefaultCornerRadius);
};

}  // namespace qrstyle
