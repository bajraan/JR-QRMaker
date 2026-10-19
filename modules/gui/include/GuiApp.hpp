#pragma once

#include "qrcodegen.hpp"

#include <SFML/Graphics.hpp>

#include <optional>
#include <string>

namespace gui {

class GuiApp {
public:
    explicit GuiApp(std::string initialText = "https://github.com/bajraan/JR-QRMaker");

    int run();

private:
    void handleEvent(const sf::Event& event);
    void generate();
    void pasteClipboard();
    void moveCursor(bool outputField, int direction, bool toEnd = false);
    void rebuildPreview();
    void drawPreview();

    std::string text_;
    std::optional<qrcodegen::QrCode> qr_;
    std::string status_;

    sf::RenderWindow window_;
    sf::VertexArray modules_{sf::PrimitiveType::Triangles};
    sf::RectangleShape previewFrame_;
    sf::FloatRect previewArea_{};
    sf::RectangleShape inputBox_;
    sf::RectangleShape outputBox_;
    sf::RectangleShape pasteButton_;
    sf::RectangleShape generateButton_;
    sf::Font font_;
    bool fontLoaded_{};
    enum class FocusedField { None, Input, Output };
    FocusedField focusedField_{FocusedField::Input};
    std::size_t textCursor_{};
    std::size_t outputCursor_{};
    std::string outputText_{"qrcode.svg"};
    std::string outputPath_{"qrcode.svg"};
};

}  // namespace gui
