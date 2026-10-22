#pragma once

#include "qrcodegen.hpp"
#include "QrStyle.hpp"

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

    void drawQrPreview();
    void tabContent_Style();
    void tabContent_Border();
    void tabContent_Label();
    void tabContent_Status();

    std::string text_;
    std::string outputText_{"qrcode.svg"};
    std::string status_{"Enter text or a link to create a QR code."};
    qrstyle::QrStyle::StyleShape style_{};
    qrstyle::QrStyle::StyleColor colors_{};
    std::optional<qrcodegen::QrCode> qr_;
    sf::RenderWindow window_;
};

}  // namespace gui
