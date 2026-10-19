#pragma once

#include "qrcodegen.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <optional>
#include <string>

namespace gui {

class GuiApp {
public:
    explicit GuiApp(std::string initialText = "https://github.com/bajraan/JR-QRMaker");
    int run();

private:
    void generate();
    void rebuildPreview();
    void drawInterface();

    std::string text_;
    std::string outputText_{"qrcode.svg"};
    std::string status_{"Enter text or a link to create a QR code."};
    std::optional<qrcodegen::QrCode> qr_;
    sf::RenderWindow window_;
};

}  // namespace gui
