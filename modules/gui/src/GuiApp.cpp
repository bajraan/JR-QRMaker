#include "GuiApp.hpp"

#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Window/Event.hpp>
#include <SFML/System/Time.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>

namespace gui {
namespace {
constexpr unsigned kWindowWidth = 1420;
constexpr unsigned kWindowHeight = 800;
constexpr std::size_t kInputCapacity = 4096;
constexpr char kPaletteColorPayload[] = "JR_QRMAKER_COLOR";

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

std::array<float, 4> toColorEdit(const std::string& hex, float alpha) {
    const std::string digits = hex.size() == 7 && hex.front() == '#' ? hex.substr(1) : hex;
    const unsigned long value = std::stoul(digits, nullptr, 16);
    return {static_cast<float>((value >> 16) & 0xFF) / 255.f,
            static_cast<float>((value >> 8) & 0xFF) / 255.f,
            static_cast<float>(value & 0xFF) / 255.f, alpha};
}

std::string fromColorEdit(const std::array<float, 4>& color, float& alpha) {
    char hex[7]{};
    const auto channel = [](float value) { return static_cast<unsigned>(std::clamp(value, 0.f, 1.f) * 255.f + 0.5f); };
    std::snprintf(hex, sizeof(hex), "%02X%02X%02X", channel(color[0]), channel(color[1]), channel(color[2]));
    alpha = std::clamp(color[3], 0.f, 1.f);
    return hex;
}

ImU32 toImGuiColor(const std::string& hex, float alpha = 1.f) {
    const auto color = toColorEdit(hex, alpha);
    return IM_COL32(static_cast<int>(color[0] * 255.f + 0.5f), static_cast<int>(color[1] * 255.f + 0.5f),
                    static_cast<int>(color[2] * 255.f + 0.5f), static_cast<int>(color[3] * 255.f + 0.5f));
}
}

GuiApp::GuiApp(std::string initialText)
    : text_(std::move(initialText)),
      window_(sf::VideoMode({kWindowWidth, kWindowHeight}), "JR-QRMaker", sf::Style::Titlebar | sf::Style::Close) {
    window_.setFramerateLimit(60);
    loadColorPalettes();
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
    
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /*
     *  SECTION 1 - PASTE / TEXT
     *
     *
     */
  {
    // Rozpoczynamy grupę dla całej linii, żeby łatwo kontrolować pozycję
    ImGui::BeginGroup();
    
    // Układ dwukolumnowy (lewa strona: przycisk, prawa strona: tekst i input)
    // Używamy tabeli bez ramek (ImGuiTableFlags_None) tylko jako układu pozycjonującego
    if (ImGui::BeginTable("##content_table", 2, ImGuiTableFlags_SizingFixedFit, ImVec2(-1.f, 0.f))) {
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 92.f);
        ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
        
        ImGui::TableNextRow();
        
        // Kolumna 0: Przycisk Paste dopasowany wysokością do pola input + etykiet
        ImGui::TableSetColumnIndex(0);
        // Obliczamy wysokość: etykieta (~15px) + odstęp + InputText (~24px) + marginesy = np. ~50-54px
        if (ImGui::Button("Paste", ImVec2(92.f, 52.f))) {
            const char* clipboard = ImGui::GetClipboardText();
            text_ = clipboard != nullptr ? clipboard : "";
            rebuildPreview();
            status_ = text_.empty() ? "Clipboard is empty." : "Text pasted from clipboard.";
        }
        
        // Kolumna 1: Etykieta, opis i pole tekstowe
        ImGui::TableSetColumnIndex(1);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextUnformatted("CONTENT");
        ImGui::SameLine();
        ImGui::TextDisabled("Paste a URL or any text you want to encode.");
        ImGui::Spacing();
        
        ImGui::SetNextItemWidth(-1.f);
        text_.resize(kInputCapacity, '\0');
        const bool contentEdited = ImGui::InputText("##content", text_.data(), text_.size(),
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
        text_.resize(std::strlen(text_.c_str()));
        if (contentEdited || ImGui::IsItemEdited()) {
            rebuildPreview();
        }
        ImGui::SetItemDefaultFocus();
        
        ImGui::EndTable();
    }
    ImGui::EndGroup();
  }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /*
     *  SECTION 2 - GENERATE / FILE NAME
     *
     *
     */
  {
    ImGui::BeginGroup();
    
    if (ImGui::BeginTable("##output_table", 2, ImGuiTableFlags_SizingFixedFit, ImVec2(-1.f, 0.f))) {
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 92.f);
        ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthStretch);
        
        ImGui::TableNextRow();
        
        // Kolumna 0: Przycisk Generate
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Button("Generate", ImVec2(92.f, 52.f)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            generate();
        }
        
        // Kolumna 1: Etykieta i input pliku wyjściowego
        ImGui::TableSetColumnIndex(1);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextUnformatted("OUTPUT FILE");
        ImGui::SameLine();
        ImGui::TextDisabled("The .svg extension is added automatically when omitted.");
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-1.f);
        outputText_.resize(512, '\0');
        ImGui::InputText("##output", outputText_.data(), outputText_.size());
        outputText_.resize(std::strlen(outputText_.c_str()));
        
        ImGui::EndTable();
    }
    ImGui::EndGroup();
  }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /*
     *  SECTION 3 - Configuration tabs // SECTION 4 - QR Preview
     *
     * 
     */

    constexpr float kSettingsWidth = 500.f;
    if (!ImGui::BeginTable ( "##editor-layout", 2,
                             ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp,
                             ImVec2(-1.f, -1.f)
                           )
    ) 
    {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthFixed, kSettingsWidth);
    ImGui::TableSetupColumn("QR preview", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);


        enum class Tab { style, border, label };
        static Tab tab = Tab::style; // Pamiętaj o inicjalizacji!


        // Pomocnicza lambda lub funkcja do rysowania przycisku zakładki z podświetleniem
        auto tabButton = [](const char* label, bool isActive) {
            if (isActive) {
                // Kolor dla aktywnego przycisku (np. wyraźny niebieski)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
            }

            bool clicked = ImGui::Button(label, ImVec2(50.f, 25.f));

            if (isActive) {
                ImGui::PopStyleColor(2); // Zawsze zdejmuj tyle stylów, ile nałożyłeś!
            }

            return clicked;
        };

        // --- Rysowanie przycisków w jednej linii ---
        if (tabButton("style", tab == Tab::style)) {
            tab = Tab::style;
        }
        ImGui::SameLine();
        if (tabButton("border", tab == Tab::border)) {
            tab = Tab::border;
        }
        ImGui::SameLine();
        if (tabButton("label", tab == Tab::label)) {
            tab = Tab::label;
        }

        // --- Obsługa widoku zakładki ---
        switch (tab) {
            case Tab::style  : tabContent_Style();  break;
            case Tab::border : tabContent_Border(); break;
            case Tab::label  : tabContent_Label();  break;
        }


    ImGui::TableSetColumnIndex(1);

    drawQrPreview();    // QR Preview

    ImGui::EndTable();
    ImGui::PopStyleColor();
    ImGui::End();

}

void GuiApp::drawQrPreview()
{

    ImGui::TextUnformatted("QR PREVIEW");
    ImGui::SameLine();
    if (qr_) {
        ImGui::TextDisabled("%d × %d modules", qr_->getSize(), qr_->getSize());
    } else {
        ImGui::TextDisabled("Enter content to see a preview");
    }

    ImGui::BeginChild("##preview", ImVec2(-1.f, -1.f), ImGuiChildFlags_Borders);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    constexpr float checkerSize = 18.f;
    for (float y = origin.y; y < origin.y + size.y; y += checkerSize) {
        for (float x = origin.x; x < origin.x + size.x; x += checkerSize) {
            const bool light = (static_cast<int>((x - origin.x) / checkerSize) +
                                static_cast<int>((y - origin.y) / checkerSize)) % 2 == 0;
            const ImU32 checkerColor = light ? IM_COL32(244, 245, 247, 255) : IM_COL32(219, 222, 226, 255);
            drawList->AddRectFilled(ImVec2(x, y),
                                    ImVec2(std::min(x + checkerSize, origin.x + size.x),
                                           std::min(y + checkerSize, origin.y + size.y)),
                                    checkerColor);
        }
    }
    if (qr_) {
        const int count = qr_->getSize();
        const float quietZoneModules = static_cast<float>(quietZoneModules_);
        const float totalModules = static_cast<float>(count) + quietZoneModules * 2.f + borderWidthModules_;
        const float module = std::max(1.f, std::min(size.x - 32.f, size.y - 32.f) / totalModules);
        const float qrSize = module * static_cast<float>(count);
        const float left = origin.x + (size.x - qrSize) * 0.5f;
        const float top = origin.y + (size.y - qrSize) * 0.5f;
        const float quietZone = quietZoneModules * module;
        const ImU32 backgroundInk = toImGuiColor(colors_.background, colors_.backgroundOpacity);
        const ImVec2 quietZoneMin(left - quietZone, top - quietZone);
        const ImVec2 quietZoneMax(left + qrSize + quietZone, top + qrSize + quietZone);
        if (roundedBorderCorners_ && borderWidthModules_ > 0.f) {
            const float halfBorderWidth = borderWidthModules_ * module * 0.5f;
            const float innerRadius = std::max(0.f, (quietZoneModules + 3.5f) * module - halfBorderWidth);
            drawList->AddRectFilled(ImVec2(quietZoneMin.x + halfBorderWidth, quietZoneMin.y + halfBorderWidth),
                                    ImVec2(quietZoneMax.x - halfBorderWidth, quietZoneMax.y - halfBorderWidth),
                                    backgroundInk, innerRadius, ImDrawFlags_RoundCornersAll);
        } else {
            drawList->AddRectFilled(quietZoneMin, quietZoneMax, backgroundInk);
        }
        const ImU32 borderInk = toImGuiColor(colors_.border, colors_.borderOpacity);
        const ImU32 bodyInk = toImGuiColor(colors_.body, colors_.bodyOpacity);
        const ImU32 frameInk = toImGuiColor(colors_.eyeframe, colors_.eyeframeOpacity);
        const ImU32 ballInk = toImGuiColor(colors_.eyeball, colors_.eyeballOpacity);
        const auto drawBorder = [&]() {
            if (borderWidthModules_ <= 0.f) {
                return;
            }
            const float borderWidth = borderWidthModules_ * module;
            const ImVec2 borderMin(left - quietZone, top - quietZone);
            const ImVec2 borderMax(left + qrSize + quietZone, top + qrSize + quietZone);
            if (!roundedBorderCorners_) {
                drawList->AddRect(borderMin, borderMax, borderInk, 0.f, ImDrawFlags_None, borderWidth);
                return;
            }

            const float radius = (quietZoneModules + 3.5f) * module;
            const float right = borderMax.x;
            const float bottom = borderMax.y;
            constexpr float pi = 3.14159265358979323846f;
            std::vector<ImVec2> borderPath;
            borderPath.reserve(4 * 18 + 4);
            const auto appendPoint = [&borderPath](ImVec2 point) {
                if (borderPath.empty()) {
                    borderPath.push_back(point);
                    return;
                }
                const ImVec2 last = borderPath.back();
                const float dx = point.x - last.x;
                const float dy = point.y - last.y;
                if (dx * dx + dy * dy > 0.0001f) {
                    borderPath.push_back(point);
                }
            };
            const auto appendArc = [&appendPoint](ImVec2 center, float arcRadius,
                                                  float startAngle, float endAngle) {
                constexpr int segments = 32;
                for (int i = 0; i <= segments; ++i) {
                    const float t = static_cast<float>(i) / static_cast<float>(segments);
                    const float angle = startAngle + (endAngle - startAngle) * t;
                    appendPoint(ImVec2(center.x + std::cos(angle) * arcRadius,
                                       center.y + std::sin(angle) * arcRadius));
                }
            };
            appendPoint(ImVec2((borderMin.x + right) * 0.5f, borderMin.y));
            appendPoint(ImVec2(right - radius, borderMin.y));
            appendArc(ImVec2(right - radius, borderMin.y + radius), radius, -pi * 0.5f, 0.f);
            appendPoint(ImVec2(right, bottom - radius));
            appendArc(ImVec2(right - radius, bottom - radius), radius, 0.f, pi * 0.5f);
            appendPoint(ImVec2(borderMin.x + radius, bottom));
            appendArc(ImVec2(borderMin.x + radius, bottom - radius), radius, pi * 0.5f, pi);
            appendPoint(ImVec2(borderMin.x, borderMin.y + radius));
            appendArc(ImVec2(borderMin.x + radius, borderMin.y + radius), radius, pi, pi * 1.5f);
            drawList->AddPolyline(borderPath.data(), static_cast<int>(borderPath.size()),
                                  borderInk, ImDrawFlags_Closed, borderWidth);
        };
        drawBorder();
        const auto drawTile = [&](int x, int y, auto shape, ImU32 ink) {
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
        const auto drawRoundedPart = [&](QrPart part, ImU32 ink) {
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
        if (style_.eyeframestyle == qrstyle::QrStyle::eyeframe::CIRCLE) {
            const ImVec2 centers[] = {
                                        {left + module * 3.5f, top + module * 3.5f},
                                        {left + module * (count - 3.5f), top + module * 3.5f},
                                        {left + module * 3.5f, top + module * (count - 3.5f)}
                                     };
            for (const ImVec2 center : centers) {
                drawList->AddCircle(center, module * 3.f, frameInk, 0, module * 0.9f);
            }
        } else if (style_.eyeframestyle == qrstyle::QrStyle::eyeframe::ROUNDED) {
            drawRoundedPart(QrPart::EyeFrame, frameInk);
        } else {
            for (int y = 0; y < count; ++y) {
                for (int x = 0; x < count; ++x) {
                    if (qr_->getModule(x, y) && classifyModule(x, y, count) == QrPart::EyeFrame) {
                        drawTile(x, y, style_.eyeframestyle, frameInk);
                    }
                }
            }
        }
        if (style_.eyeballstyle == qrstyle::QrStyle::eyeball::CIRCLE) {
            const ImVec2 centers[] = {{left + module * 3.5f, top + module * 3.5f},
                                      {left + module * (count - 3.5f), top + module * 3.5f},
                                      {left + module * 3.5f, top + module * (count - 3.5f)}};
            for (const ImVec2 center : centers) {
                drawList->AddCircleFilled(center, module * 1.5f, ballInk);
            }
        } else if (style_.eyeballstyle == qrstyle::QrStyle::eyeball::ROUNDED) {
            drawRoundedPart(QrPart::EyeBall, ballInk);
        } else {
            for (int y = 0; y < count; ++y) {
                for (int x = 0; x < count; ++x) {
                    if (qr_->getModule(x, y) && classifyModule(x, y, count) == QrPart::EyeBall) {
                        drawTile(x, y, style_.eyeballstyle, ballInk);
                    }
                }
            }
        }
        if (style_.bodystyle == qrstyle::QrStyle::body::ROUNDED) {
            drawRoundedPart(QrPart::Body, bodyInk);
        } else {
            for (int y = 0; y < count; ++y) {
                for (int x = 0; x < count; ++x) {
                    if (qr_->getModule(x, y) && classifyModule(x, y, count) == QrPart::Body) {
                        drawTile(x, y, style_.bodystyle, bodyInk);
                    }
                }
            }
        }
    }
    ImGui::EndChild();
}

void GuiApp::tabContent_Style()
{
    // 1. Area Style / Frame, Background
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.18f, 1.0f)); // Gray Background 
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));     // White Frame
    
    // 2. PopUps background
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));  

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);   // disable white frame in PopUps

    if (ImGui::BeginChild("##StyleContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (ImGui::BeginTable("##StyleLayoutTable", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings)) 
        {
            ImGui::TableSetupColumn("Style", ImGuiTableColumnFlags_WidthFixed, 100.f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 80.f);
            ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 140.f);
            ImGui::TableSetupColumn("Transparency", ImGuiTableColumnFlags_WidthStretch);

            const char* bodyShapes[] = {"Square", "Rounded", "Circles"};
            const char* eyeShapes[] = {"Square", "Rounded", "Circle", "Circles"};



            // --- Row 1: Body ---
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            int bodyShape = static_cast<int>(style_.bodystyle);
            ImGui::SetNextItemWidth(90.f);
            if (ImGui::Combo("##BodyCombo", &bodyShape, bodyShapes, IM_ARRAYSIZE(bodyShapes))) {
                style_.bodystyle = static_cast<qrstyle::QrStyle::body>(bodyShape);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Body");

            ImGui::TableSetColumnIndex(2);
            auto bodyColor = toColorEdit(colors_.body, colors_.bodyOpacity);
            ImGui::SetNextItemWidth(112.f);
            if ( ImGui::ColorEdit4 (  "##body-color", bodyColor.data(), 
                                      ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder
                                   )
               ) 
            {
                colors_.body = fromColorEdit(bodyColor, colors_.bodyOpacity);
            }
            acceptPaletteColorDrop(colors_.body, "Body");

            ImGui::TableSetColumnIndex(3);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##bodyOpacity", &colors_.bodyOpacity, 0.0f, 1.0f, "Opacity: %.2f")) 
            {
            }



            // --- Row 2: Eye frame ---
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            int frameShape = static_cast<int>(style_.eyeframestyle);
            ImGui::SetNextItemWidth(90.f);
            if (ImGui::Combo("##FrameCombo", &frameShape, eyeShapes, IM_ARRAYSIZE(eyeShapes))) {
                style_.eyeframestyle = static_cast<qrstyle::QrStyle::eyeframe>(frameShape);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Eye Frame");

            ImGui::TableSetColumnIndex(2);
            auto frameColor = toColorEdit(colors_.eyeframe, colors_.eyeframeOpacity);
            ImGui::SetNextItemWidth(112.f);
            if (ImGui::ColorEdit4("##eye-frame-color", frameColor.data(), 
                                   ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoLabel |  ImGuiColorEditFlags_NoBorder)){
                colors_.eyeframe = fromColorEdit(frameColor, colors_.eyeframeOpacity);
            }
            acceptPaletteColorDrop(colors_.eyeframe, "Eye Frame");

            ImGui::TableSetColumnIndex(3);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##eye-frame-Opacity", &colors_.eyeframeOpacity, 0.0f, 1.0f, "Opacity: %.2f")) 
            {
            }



            // --- Row 3: Eye Ball ---
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            int ballShape = static_cast<int>(style_.eyeballstyle);
            ImGui::SetNextItemWidth(90.f);
            if (ImGui::Combo("##BallCombo", &ballShape, eyeShapes, IM_ARRAYSIZE(eyeShapes))) {
                style_.eyeballstyle = static_cast<qrstyle::QrStyle::eyeball>(ballShape);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Eye Ball");

            ImGui::TableSetColumnIndex(2);
            auto ballColor = toColorEdit(colors_.eyeball, colors_.eyeballOpacity);
            ImGui::SetNextItemWidth(112.f);
            if (ImGui::ColorEdit4("##eye-center-color", ballColor.data(), ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder)) {
                colors_.eyeball = fromColorEdit(ballColor, colors_.eyeballOpacity);
            }
            acceptPaletteColorDrop(colors_.eyeball, "Eye Ball");

            ImGui::TableSetColumnIndex(3);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##eye-ball-Opacity", &colors_.eyeballOpacity, 0.0f, 1.0f, "Opacity: %.2f")) 
            {
            }



            // --- Row 4: Background ---
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(" N/A ");

            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Background");

            ImGui::TableSetColumnIndex(2);
            ImGui::AlignTextToFramePadding();
            auto backgroundColor = toColorEdit(colors_.background, colors_.backgroundOpacity);
            ImGui::SetNextItemWidth(112.f);
            if (ImGui::ColorEdit4("##background-color", backgroundColor.data(), ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder)) {
                colors_.background = fromColorEdit(backgroundColor, colors_.backgroundOpacity);
            }
            acceptPaletteColorDrop(colors_.background, "Background");

            ImGui::TableSetColumnIndex(3);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##background-Opacity", &colors_.backgroundOpacity, 0.0f, 1.0f, "Opacity: %.2f")) 
            {
            }

            // --- Row 5: Border ---
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(" N/A ");

            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Border");

            ImGui::TableSetColumnIndex(2);
            ImGui::AlignTextToFramePadding();
            auto borderColor = toColorEdit(colors_.border, colors_.borderOpacity);
            ImGui::SetNextItemWidth(112.f);
            if (ImGui::ColorEdit4("##border-color", borderColor.data(),
                                  ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoLabel |
                                      ImGuiColorEditFlags_NoBorder)) {
                colors_.border = fromColorEdit(borderColor, colors_.borderOpacity);
            }
            acceptPaletteColorDrop(colors_.border, "Border");

            ImGui::TableSetColumnIndex(3);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SliderFloat("##border-opacity", &colors_.borderOpacity, 0.0f, 1.0f, "Opacity: %.2f");

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    // Zdejmujemy dokładnie tyle samo stylów, ile wrzuciliśmy (3 zmienne i 3 kolory)
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    ImGui::Spacing();

    drawColorPaletteViewer();

    ImGui::Spacing();
    ImGui::Spacing();

    //ImGui::TextUnformatted("STATUS");
    //ImGui::TextWrapped("%s", status_.c_str());

    
    // --- BOX 2: Sekcja Statusu (opcjonalnie w osobnym, mniejszym boxie) ---
    tabContent_Status();



}

void GuiApp::loadColorPalettes() {
    std::vector<std::filesystem::path> searchPaths;
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    searchPaths.emplace_back(workingDirectory / "resources" / "colorpallets");
    searchPaths.emplace_back(workingDirectory / ".." / "resources" / "colorpallets");
    searchPaths.emplace_back(workingDirectory / ".." / "share" / "jr-qrmaker" / "resources" / "colorpallets");
    searchPaths.emplace_back(workingDirectory / "share" / "jr-qrmaker" / "resources" / "colorpallets");
#ifdef JR_QRMAKER_BUILD_RESOURCE_DIR
    searchPaths.emplace_back(std::filesystem::path(JR_QRMAKER_BUILD_RESOURCE_DIR) / "colorpallets");
#endif
#ifdef JR_QRMAKER_SOURCE_RESOURCE_DIR
    searchPaths.emplace_back(std::filesystem::path(JR_QRMAKER_SOURCE_RESOURCE_DIR) / "colorpallets");
#endif

    std::filesystem::path paletteDirectory;
    for (const auto& candidate : searchPaths) {
        std::error_code error;
        if (std::filesystem::is_directory(candidate, error)) {
            paletteDirectory = candidate;
            break;
        }
    }
    if (paletteDirectory.empty()) {
        paletteLoadMessage_ = "Palette resources were not found.";
        return;
    }

    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (std::filesystem::directory_iterator it(paletteDirectory, error), end; !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && it->path().extension() == ".gpl") {
            files.push_back(it->path());
        }
    }
    std::sort(files.begin(), files.end());

    for (const auto& filePath : files) {
        std::ifstream file(filePath);
        if (!file) {
            continue;
        }

        ColorPalette palette;
        palette.name = filePath.stem().string();
        std::string line;
        while (std::getline(file, line)) {
            if (line.rfind("Name:", 0) == 0) {
                const std::size_t first = line.find_first_not_of(" \t", 5);
                if (first != std::string::npos) {
                    palette.name = line.substr(first);
                }
                continue;
            }
            if (line.empty() || line[0] == '#' || line.rfind("GIMP Palette", 0) == 0 ||
                line.rfind("Columns:", 0) == 0) {
                continue;
            }

            std::istringstream colorLine(line);
            int red = 0;
            int green = 0;
            int blue = 0;
            if (!(colorLine >> red >> green >> blue) || red < 0 || red > 255 || green < 0 || green > 255 ||
                blue < 0 || blue > 255) {
                continue;
            }

            char hex[7]{};
            std::snprintf(hex, sizeof(hex), "%02X%02X%02X", red, green, blue);
            palette.colors.emplace_back(hex);
        }
        if (!palette.colors.empty()) {
            palettes_.push_back(std::move(palette));
        }
    }

    paletteLoadMessage_ = palettes_.empty() ? "No readable .gpl color palettes were found." : "";
}

void GuiApp::drawColorPaletteViewer() {
    const float remainingHeight = ImGui::GetContentRegionAvail().y;
    const float viewerHeight = std::max(90.f, std::min(400.f, remainingHeight - 76.f));
    if (!ImGui::BeginChild("##PaletteViewer", ImVec2(0.f, viewerHeight))) {
        ImGui::EndChild();
        return;
    }

    if (palettes_.empty()) {
        ImGui::TextWrapped("%s", paletteLoadMessage_.c_str());
    } else {
        for (std::size_t i = 0; i < palettes_.size(); ++i) {
            const ColorPalette& palette = palettes_[i];
            ImGui::PushID(static_cast<int>(i));

            // Całkowicie wyłączamy style podświetlenia po najechaniu i kliknięciu
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.f, 0.f, 0.f, 0.f));
            
            // Przekazujemy 'false' jako stan zaznaczenia, żeby ImGui ignorowało ten stan wizualnie
            const bool clicked = ImGui::Selectable("##palette-card", false, ImGuiSelectableFlags_None,
                                                 ImVec2(0.f, 38.f));
            
            ImGui::PopStyleColor(2);

            const bool hovered = ImGui::IsItemHovered();
            const ImVec2 itemMin = ImGui::GetItemRectMin();
            const ImVec2 itemMax = ImGui::GetItemRectMax();
            const float swatchStart = itemMin.x + 0.f;
            const float swatchEnd = itemMax.x - 8.f;
            const float swatchArea = std::max(0.f, swatchEnd - swatchStart);
            const float swatchWidth = swatchArea / static_cast<float>(palette.colors.size());
            const float colorPosition = std::clamp((ImGui::GetIO().MousePos.x - swatchStart) /
                                                   std::max(1.f, swatchArea),
                                                 0.f, 0.999999f);
            const std::size_t pointerColorIndex = std::min(
                static_cast<std::size_t>(colorPosition * palette.colors.size()), palette.colors.size() - 1);
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                dragCandidatePalette_ = static_cast<int>(i);
                dragCandidateColorIndex_ = pointerColorIndex;
            }

            if (ImGui::BeginDragDropSource()) {
                const std::size_t lockedColorIndex = dragCandidatePalette_ == static_cast<int>(i)
                                                         ? std::min(dragCandidateColorIndex_, palette.colors.size() - 1)
                                                         : pointerColorIndex;
                const std::string& pointedColor = palette.colors[lockedColorIndex];
                ImGui::SetDragDropPayload(kPaletteColorPayload, pointedColor.c_str(), pointedColor.size() + 1,
                                          ImGuiCond_Once);
                std::string colorHex = pointedColor;
                if (const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                    payload != nullptr && payload->IsDataType(kPaletteColorPayload) && payload->Data != nullptr) {
                    colorHex = static_cast<const char*>(payload->Data);
                }
                const auto color = toColorEdit(colorHex, 1.f);
                ImGui::ColorButton("##drag-color-preview", ImVec4(color[0], color[1], color[2], 1.f),
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                                   ImVec2(34.f, 22.f));
                ImGui::SameLine();
                ImGui::Text("%s: #%s", palette.name.c_str(), colorHex.c_str());
                ImGui::EndDragDropSource();
            }

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRect(itemMin, itemMax, true);
            const float swatchHeight = 40.f;
            const float swatchY = itemMin.y + (itemMax.y - itemMin.y - swatchHeight) * 0.5f;
            for (std::size_t colorIndex = 0; colorIndex < palette.colors.size(); ++colorIndex) {
                const float left = swatchStart + colorIndex * swatchWidth;
                const ImVec2 min(left, swatchY);
                const float right = colorIndex + 1 == palette.colors.size() ? swatchEnd : left + swatchWidth;
                const ImVec2 max(right, swatchY + swatchHeight);
                const ImU32 color = toImGuiColor(palette.colors[colorIndex]);
                drawList->AddRectFilled(min, max, color);
            }
            drawList->PopClipRect();

            if (hovered && ImGui::GetDragDropPayload() == nullptr) {
                const auto& hex = palette.colors[pointerColorIndex];
                const auto color = toColorEdit(hex, 1.f);
                ImGui::BeginTooltip();
                ImGui::ColorButton("##picked-color-preview",
                                   ImVec4(color[0], color[1], color[2], 1.f),
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                                   ImVec2(34.f, 22.f));
                ImGui::SameLine();
                ImGui::Text("%s: #%s", palette.name.c_str(), hex.c_str());
                ImGui::EndTooltip();
            }
            if (clicked) {
                applyColorPalette(i);
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void GuiApp::applyColorPalette(std::size_t paletteIndex) {
    if (paletteIndex >= palettes_.size()) {
        return;
    }
    const ColorPalette& palette = palettes_[paletteIndex];
    colors_.body = palette.colors[0];
    colors_.border = palette.colors[0];
    colors_.eyeframe = palette.colors[std::min<std::size_t>(1, palette.colors.size() - 1)];
    colors_.eyeball = palette.colors[std::min<std::size_t>(2, palette.colors.size() - 1)];
    if (palette.colors.size() >= 4) {
        colors_.background = palette.colors.back();
    }
    selectedPalette_ = static_cast<int>(paletteIndex);
    status_ = "Applied color palette: " + palette.name;
}

void GuiApp::acceptPaletteColorDrop(std::string& regionColor, const char* regionName) {
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }

    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPaletteColorPayload)) {
        if (payload->Data != nullptr && payload->DataSize >= 7) {
            const auto* hex = static_cast<const char*>(payload->Data);
            regionColor.assign(hex, 6);
            status_ = "Applied palette color to " + std::string(regionName) + ".";
            selectedPalette_ = -1;
        }
    }
    ImGui::EndDragDropTarget();
}

void GuiApp::tabContent_Label(){
    tabContent_Status();
}

void GuiApp::tabContent_Border(){
    ImGui::TextUnformatted("BORDER SETTINGS");
    ImGui::Separator();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderInt("Quiet zone (modules)", &quietZoneModules_, 0, 32);
    ImGui::TextDisabled("Recommended quiet zone: 4 modules.");
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("Border width (modules)", &borderWidthModules_, 0.f, 2.f, "%.1f modules")) {
        borderWidthModules_ = std::round(borderWidthModules_ * 10.f) / 10.f;
    }
    ImGui::TextDisabled("0 disables it; width is in modules and centered on the QR edge.");
    ImGui::Checkbox("Rounded corners", &roundedBorderCorners_);
    ImGui::TextDisabled("Radius follows the virtual eye centers (quiet zone + 3.5 modules).");
    tabContent_Status();
}

void GuiApp::tabContent_Status()
{
    // --- BOX 2: Sekcja Statusu (opcjonalnie w osobnym, mniejszym boxie) ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

    if (ImGui::BeginChild("##StatusContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::TextUnformatted("STATUS");
        ImGui::Separator();
        ImGui::TextWrapped("%s", status_.c_str());
    }
    ImGui::EndChild();
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
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
        qrstyle::QrStyle::writeFile(qr, path.string(), style_, colors_, quietZoneModules_,
                                    qrstyle::QrStyle::kDefaultCornerRadius, borderWidthModules_,
                                    roundedBorderCorners_);
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
