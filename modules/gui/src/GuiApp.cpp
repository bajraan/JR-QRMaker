#include "GuiApp.hpp"
#include "SvgWriter.hpp"

#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Window/Event.hpp>
#include <SFML/System/Time.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <filesystem>

namespace gui {
namespace {
constexpr unsigned kWindowWidth = 960;
constexpr unsigned kWindowHeight = 700;
constexpr std::size_t kInputCapacity = 4096;
}

GuiApp::GuiApp(std::string initialText)
    : text_(std::move(initialText)),
      window_(sf::VideoMode({kWindowWidth, kWindowHeight}), "JR-QRMaker", sf::Style::Titlebar | sf::Style::Close) {
    window_.setFramerateLimit(60);
    rebuildPreview();
}

int GuiApp::run() {
    if (!ImGui::SFML::Init(window_)) {
        return 1;
    }

    auto previousFrame = std::chrono::steady_clock::now();
    while (window_.isOpen()) {
        while (const std::optional<sf::Event> event = window_.pollEvent()) {
            ImGui::SFML::ProcessEvent(window_, *event);
            if (event->is<sf::Event::Closed>()) {
                window_.close();
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<float>(now - previousFrame);
        previousFrame = now;
        ImGui::SFML::Update(window_, sf::seconds(elapsed.count()));
        drawInterface();

        window_.clear(sf::Color(18, 22, 31));
        ImGui::SFML::Render(window_);
        window_.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

void GuiApp::drawInterface() {
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(window_.getSize().x), static_cast<float>(window_.getSize().y)));
    ImGui::Begin("JR-QRMaker", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.93f, 0.95f, 0.98f, 1.f));
    ImGui::TextUnformatted("QR CODE STUDIO");
    ImGui::SameLine();
    ImGui::TextDisabled("  Create a QR code and save it as SVG");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("CONTENT");
    ImGui::SetNextItemWidth(-1.f);
    text_.resize(kInputCapacity, '\0');
    const bool contentEdited = ImGui::InputText("##content", text_.data(), text_.size(), ImGuiInputTextFlags_EnterReturnsTrue);
    text_.resize(std::strlen(text_.c_str()));
    if (contentEdited || ImGui::IsItemEdited()) {
        rebuildPreview();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::TextDisabled("Paste a URL or any text you want to encode.");
    ImGui::Spacing();

    ImGui::TextUnformatted("OUTPUT FILE");
    ImGui::SetNextItemWidth(-1.f);
    outputText_.resize(512, '\0');
    ImGui::InputText("##output", outputText_.data(), outputText_.size());
    outputText_.resize(std::strlen(outputText_.c_str()));
    ImGui::TextDisabled("The .svg extension is added automatically when omitted.");
    ImGui::Spacing();

    if (ImGui::Button("Paste from clipboard", ImVec2(170.f, 34.f))) {
        const char* clipboard = ImGui::GetClipboardText();
        text_ = clipboard != nullptr ? clipboard : "";
        rebuildPreview();
        status_ = text_.empty() ? "Clipboard is empty." : "Text pasted from clipboard.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Generate SVG", ImVec2(140.f, 34.f)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        generate();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted("PREVIEW");
    ImGui::SameLine();
    if (qr_) {
        ImGui::TextDisabled("%d × %d modules", qr_->getSize(), qr_->getSize());
    } else {
        ImGui::TextDisabled("Enter content to see a preview");
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float previewHeight = std::max(160.f, available.y - 76.f);
    ImGui::BeginChild("##preview", ImVec2(-1.f, previewHeight), ImGuiChildFlags_Borders);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(255, 255, 255, 255));
    if (qr_) {
        const int count = qr_->getSize();
        const float module = std::max(1.f, std::min(size.x - 32.f, size.y - 32.f) / static_cast<float>(count));
        const float qrSize = module * static_cast<float>(count);
        const float left = origin.x + (size.x - qrSize) * 0.5f;
        const float top = origin.y + (size.y - qrSize) * 0.5f;
        for (int y = 0; y < count; ++y) {
            for (int x = 0; x < count; ++x) {
                if (qr_->getModule(x, y)) {
                    const ImVec2 p0(left + x * module, top + y * module);
                    drawList->AddRectFilled(p0, ImVec2(p0.x + module + 0.15f, p0.y + module + 0.15f),
                                            IM_COL32(15, 19, 27, 255));
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::TextWrapped("%s", status_.c_str());
    ImGui::PopStyleColor();
    ImGui::End();
}

void GuiApp::generate() {
    if (text_.empty()) {
        status_ = "Enter a link or some text first.";
        return;
    }
    if (outputText_.empty()) {
        status_ = "Enter an output file name.";
        return;
    }

    try {
        const auto qr = qrcodegen::QrCode::encodeText(text_.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        std::filesystem::path path(outputText_);
        if (path.extension().empty()) {
            path += ".svg";
        }
        svggen::SvgWriter::writeFile(qr, path.string());
        qr_ = qr;
        status_ = "Saved SVG: " + std::filesystem::absolute(path).string();
    } catch (const std::exception& ex) {
        status_ = ex.what();
    }
}

void GuiApp::rebuildPreview() {
    qr_.reset();
    if (text_.empty()) {
        return;
    }
    try {
        qr_ = qrcodegen::QrCode::encodeText(text_.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    } catch (const std::exception& ex) {
        status_ = ex.what();
    }
}

}  // namespace gui
