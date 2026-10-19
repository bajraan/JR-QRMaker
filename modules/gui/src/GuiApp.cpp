#include "GuiApp.hpp"

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

enum class QrPart { Body, EyeFrame, EyeBall };

QrPart classifyModule(int x, int y, int size) {
    const int eyeLefts[] = {0, size - 7, 0};
    const int eyeTops[] = {0, 0, size - 7};
    for (int eye = 0; eye < 3; ++eye) {
        const int localX = x - eyeLefts[eye];
        const int localY = y - eyeTops[eye];
        if (localX < 0 || localX >= 7 || localY < 0 || localY >= 7) {
            continue;
        }
        if (localX == 0 || localX == 6 || localY == 0 || localY == 6) {
            return QrPart::EyeFrame;
        }
        if (localX >= 2 && localX <= 4 && localY >= 2 && localY <= 4) {
            return QrPart::EyeBall;
        }
    }
    return QrPart::Body;
}
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

    ImGui::TextUnformatted("MODULE SHAPES");
    const char* bodyShapes[] = {"Square", "Rounded", "Circles"};
    int bodyShape = static_cast<int>(style_.bodystyle);
    if (ImGui::Combo("Body", &bodyShape, bodyShapes, IM_ARRAYSIZE(bodyShapes))) {
        style_.bodystyle = static_cast<qrstyle::QrStyle::body>(bodyShape);
    }
    const char* eyeShapes[] = {"Square", "Rounded", "Circle", "Circles"};
    int frameShape = static_cast<int>(style_.eyeframestyle);
    if (ImGui::Combo("Eye frame", &frameShape, eyeShapes, IM_ARRAYSIZE(eyeShapes))) {
        style_.eyeframestyle = static_cast<qrstyle::QrStyle::eyeframe>(frameShape);
    }
    int ballShape = static_cast<int>(style_.eyeballstyle);
    if (ImGui::Combo("Eye center", &ballShape, eyeShapes, IM_ARRAYSIZE(eyeShapes))) {
        style_.eyeballstyle = static_cast<qrstyle::QrStyle::eyeball>(ballShape);
    }
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
        const ImU32 ink = IM_COL32(20, 27, 39, 255);
        const auto drawTile = [&](int x, int y, auto shape) {
            const ImVec2 p0(left + x * module, top + y * module);
            const ImVec2 p1(p0.x + module + 0.15f, p0.y + module + 0.15f);
            if (shape == decltype(shape)::CIRCLES) {
                drawList->AddCircleFilled(ImVec2(p0.x + module * 0.5f, p0.y + module * 0.5f), module * 0.48f, ink);
            } else if (shape == decltype(shape)::ROUNDED) {
                drawList->AddRectFilled(p0, p1, ink, module * qrstyle::QrStyle::kDefaultCornerRadius);
            } else {
                drawList->AddRectFilled(p0, p1, ink);
            }
        };
        const auto drawRoundedPart = [&](QrPart part) {
            for (int y = 0; y < count; ++y) {
                for (int x = 0; x < count; ++x) {
                    if (!qr_->getModule(x, y) || classifyModule(x, y, count) != part) {
                        continue;
                    }
                    const float tileX = left + x * module;
                    const float tileY = top + y * module;
                    drawList->AddCircleFilled(ImVec2(tileX + module * 0.5f, tileY + module * 0.5f), module * 0.5f, ink);
                    if (x + 1 < count && qr_->getModule(x + 1, y) && classifyModule(x + 1, y, count) == part) {
                        drawList->AddRectFilled(ImVec2(tileX + module * 0.5f, tileY),
                                                ImVec2(tileX + module * 1.5f, tileY + module), ink);
                    }
                    if (y + 1 < count && qr_->getModule(x, y + 1) && classifyModule(x, y + 1, count) == part) {
                        drawList->AddRectFilled(ImVec2(tileX, tileY + module * 0.5f),
                                                ImVec2(tileX + module, tileY + module * 1.5f), ink);
                    }
                }
            }
        };
        if (style_.bodystyle == qrstyle::QrStyle::body::ROUNDED) {
            drawRoundedPart(QrPart::Body);
        } else {
        for (int y = 0; y < count; ++y) {
            for (int x = 0; x < count; ++x) {
                if (qr_->getModule(x, y) && classifyModule(x, y, count) == QrPart::Body) {
                    drawTile(x, y, style_.bodystyle);
                }
            }
        }
        }
        if (style_.eyeframestyle == qrstyle::QrStyle::eyeframe::CIRCLE) {
            const ImVec2 centers[] = {{left + module * 3.5f, top + module * 3.5f},
                                      {left + module * (count - 3.5f), top + module * 3.5f},
                                      {left + module * 3.5f, top + module * (count - 3.5f)}};
            for (const ImVec2 center : centers) {
                drawList->AddCircle(center, module * 3.f, ink, 0, module * 0.9f);
            }
        } else if (style_.eyeframestyle == qrstyle::QrStyle::eyeframe::ROUNDED) {
            drawRoundedPart(QrPart::EyeFrame);
        } else {
            for (int y = 0; y < count; ++y) {
                for (int x = 0; x < count; ++x) {
                    if (qr_->getModule(x, y) && classifyModule(x, y, count) == QrPart::EyeFrame) {
                        drawTile(x, y, style_.eyeframestyle);
                    }
                }
            }
        }
        if (style_.eyeballstyle == qrstyle::QrStyle::eyeball::CIRCLE) {
            const ImVec2 centers[] = {{left + module * 3.5f, top + module * 3.5f},
                                      {left + module * (count - 3.5f), top + module * 3.5f},
                                      {left + module * 3.5f, top + module * (count - 3.5f)}};
            for (const ImVec2 center : centers) {
                drawList->AddCircleFilled(center, module * 1.5f, ink);
            }
        } else if (style_.eyeballstyle == qrstyle::QrStyle::eyeball::ROUNDED) {
            drawRoundedPart(QrPart::EyeBall);
        } else {
            for (int y = 0; y < count; ++y) {
                for (int x = 0; x < count; ++x) {
                    if (qr_->getModule(x, y) && classifyModule(x, y, count) == QrPart::EyeBall) {
                        drawTile(x, y, style_.eyeballstyle);
                    }
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
        qrstyle::QrStyle::writeFile(qr, path.string(), style_, colors_);
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
