#include "GuiApp.hpp"
#include "SvgWriter.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>

namespace gui {
namespace {
constexpr unsigned kWindowWidth = 960;
constexpr unsigned kWindowHeight = 640;
constexpr float kMargin = 48.f;

sf::Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) { return {r, g, b}; }
}

GuiApp::GuiApp(std::string initialText)
    : text_(std::move(initialText)),
      window_(sf::VideoMode({kWindowWidth, kWindowHeight}), "JR-QRMaker", sf::Style::Titlebar | sf::Style::Close) {
    window_.setFramerateLimit(60);
    inputBox_.setPosition({kMargin, 54.f}); inputBox_.setSize({650.f, 40.f});
    inputBox_.setFillColor(sf::Color::White); inputBox_.setOutlineColor(rgb(110,110,125)); inputBox_.setOutlineThickness(2.f);
    pasteButton_.setPosition({710.f,54.f}); pasteButton_.setSize({90.f,40.f}); pasteButton_.setFillColor(rgb(65,105,170));
    generateButton_.setPosition({810.f,54.f}); generateButton_.setSize({102.f,40.f}); generateButton_.setFillColor(rgb(55,145,90));
    outputBox_.setPosition({kMargin,112.f}); outputBox_.setSize({864.f,36.f});
    outputBox_.setFillColor(sf::Color::White); outputBox_.setOutlineColor(rgb(110,110,125)); outputBox_.setOutlineThickness(2.f);
    for (const char* path : {"C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/segoeui.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"})
        if (font_.openFromFile(path)) { fontLoaded_ = true; break; }

    previewArea_ = sf::FloatRect({kMargin,160.f},{static_cast<float>(kWindowWidth)-2*kMargin,440.f});
    previewFrame_.setPosition(previewArea_.position); previewFrame_.setSize(previewArea_.size);
    previewFrame_.setFillColor(rgb(245,245,245)); previewFrame_.setOutlineColor(rgb(60,60,70)); previewFrame_.setOutlineThickness(2.f);
    rebuildPreview();
}

int GuiApp::run() {
    while (window_.isOpen()) {
        while (const std::optional<sf::Event> event = window_.pollEvent()) handleEvent(*event);
        window_.clear(rgb(28,28,34));
        window_.draw(inputBox_); window_.draw(pasteButton_); window_.draw(generateButton_); window_.draw(outputBox_);
        if (fontLoaded_) {
            sf::Text caption(font_, "QR content", 13); caption.setPosition({kMargin,35.f}); caption.setFillColor(sf::Color::White); window_.draw(caption);
            sf::Text content(font_, text_, 16); content.setPosition({kMargin+9.f,64.f}); content.setFillColor(rgb(30,30,35));
            if (focusedField_ == FocusedField::Input) { content.setString(text_.substr(0,textCursor_) + "|" + text_.substr(textCursor_)); }
            window_.draw(content);
            sf::Text paste(font_, "Paste", 16); paste.setPosition({728.f,63.f}); paste.setFillColor(sf::Color::White); window_.draw(paste);
            sf::Text generateLabel(font_, "Generate", 16); generateLabel.setPosition({821.f,63.f}); generateLabel.setFillColor(sf::Color::White); window_.draw(generateLabel);
            sf::Text outputCaption(font_, "Output SVG file name", 13); outputCaption.setPosition({kMargin,93.f}); outputCaption.setFillColor(sf::Color::White); window_.draw(outputCaption);
            sf::Text output(font_, outputText_, 15); output.setPosition({kMargin+9.f,120.f}); output.setFillColor(rgb(30,30,35));
            if (focusedField_ == FocusedField::Output) output.setString(outputText_.substr(0,outputCursor_) + "|" + outputText_.substr(outputCursor_));
            window_.draw(output);
            sf::Text status(font_, status_, 14); status.setPosition({kMargin,610.f}); status.setFillColor(sf::Color::White); window_.draw(status);
        }
        window_.draw(previewFrame_); drawPreview(); window_.display();
    }
    return 0;
}

void GuiApp::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) { window_.close(); return; }
    if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>(); mouse && mouse->button == sf::Mouse::Button::Left) {
        const sf::Vector2f p{static_cast<float>(mouse->position.x),static_cast<float>(mouse->position.y)};
        focusedField_ = inputBox_.getGlobalBounds().contains(p) ? FocusedField::Input :
                        outputBox_.getGlobalBounds().contains(p) ? FocusedField::Output : FocusedField::None;
        if (focusedField_ == FocusedField::Input) textCursor_ = text_.size();
        if (focusedField_ == FocusedField::Output) outputCursor_ = outputText_.size();
        if (pasteButton_.getGlobalBounds().contains(p)) pasteClipboard();
        if (generateButton_.getGlobalBounds().contains(p)) generate();
        return;
    }
    if (const auto* entered = event.getIf<sf::Event::TextEntered>()) {
        if (entered->unicode < 32 || entered->unicode > 126) return;
        if (focusedField_ == FocusedField::Input) { text_.insert(textCursor_,1,static_cast<char>(entered->unicode)); ++textCursor_; rebuildPreview(); }
        if (focusedField_ == FocusedField::Output) { outputText_.insert(outputCursor_,1,static_cast<char>(entered->unicode)); ++outputCursor_; }
        return;
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (focusedField_ == FocusedField::Input && key->control && key->code == sf::Keyboard::Key::V) { pasteClipboard(); return; }
        if (key->code == sf::Keyboard::Key::Left) { moveCursor(focusedField_ == FocusedField::Output,-1); return; }
        if (key->code == sf::Keyboard::Key::Right) { moveCursor(focusedField_ == FocusedField::Output,1); return; }
        if (key->code == sf::Keyboard::Key::Home) { moveCursor(focusedField_ == FocusedField::Output,-1,true); return; }
        if (key->code == sf::Keyboard::Key::End) { moveCursor(focusedField_ == FocusedField::Output,1,true); return; }
        if (key->code == sf::Keyboard::Key::Enter) { generate(); return; }
        if (key->code == sf::Keyboard::Key::Backspace) {
            if (focusedField_ == FocusedField::Input && textCursor_ > 0) { text_.erase(--textCursor_,1); rebuildPreview(); }
            if (focusedField_ == FocusedField::Output && outputCursor_ > 0) outputText_.erase(--outputCursor_,1);
        }
    }
}

void GuiApp::pasteClipboard() {
    text_ = sf::Clipboard::getString().toAnsiString();
    textCursor_ = text_.size();
    focusedField_ = FocusedField::Input;
    if (text_.empty()) { qr_.reset(); modules_.clear(); status_ = "Clipboard is empty"; }
    else { rebuildPreview(); status_ = "Text pasted from clipboard"; }
}

void GuiApp::moveCursor(bool outputField, int direction, bool toEnd) {
    if (focusedField_ == FocusedField::None) return;
    std::string& value = outputField ? outputText_ : text_;
    std::size_t& cursor = outputField ? outputCursor_ : textCursor_;
    cursor = toEnd ? (direction < 0 ? 0 : value.size()) :
        (direction < 0 ? (cursor > 0 ? cursor - 1 : 0) : std::min(cursor + 1, value.size()));
}

void GuiApp::generate() {
    if (text_.empty()) { status_ = "Enter a link or text first"; return; }
    if (outputText_.empty()) { status_ = "Enter an output file name"; return; }
    try {
        const auto qr = qrcodegen::QrCode::encodeText(text_.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        std::filesystem::path path(outputText_);
        if (path.extension().empty()) path += ".svg";
        svggen::SvgWriter::writeFile(qr, path.string());
        qr_ = qr; outputPath_ = path.string(); status_ = "SVG generated: " + std::filesystem::absolute(path).string();
        rebuildPreview();
    } catch (const std::exception& ex) { status_ = ex.what(); }
}

void GuiApp::rebuildPreview() {
    modules_.clear(); qr_.reset();
    if (text_.empty()) return;
    try { qr_ = qrcodegen::QrCode::encodeText(text_.c_str(), qrcodegen::QrCode::Ecc::MEDIUM); }
    catch (const std::exception& ex) { status_ = ex.what(); return; }
    const auto& qr = *qr_; const int size = qr.getSize();
    const float module = std::min(previewArea_.size.x-32.f,previewArea_.size.y-32.f)/static_cast<float>(size);
    const float ox=previewArea_.position.x+(previewArea_.size.x-module*size)*.5f, oy=previewArea_.position.y+(previewArea_.size.y-module*size)*.5f;
    for(int y=0;y<size;++y) for(int x=0;x<size;++x) if(qr.getModule(x,y)) {
        const float l=ox+x*module,t=oy+y*module,r=l+module,b=t+module; const sf::Color black=sf::Color::Black;
        modules_.append(sf::Vertex{{l,t},black}); modules_.append(sf::Vertex{{r,t},black}); modules_.append(sf::Vertex{{r,b},black});
        modules_.append(sf::Vertex{{l,t},black}); modules_.append(sf::Vertex{{r,b},black}); modules_.append(sf::Vertex{{l,b},black});
    }
}

void GuiApp::drawPreview() { window_.draw(modules_); }
} // namespace gui
