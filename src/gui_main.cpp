#include "GuiApp.hpp"

#include <string>

int main(int argc, char* argv[]) {
    const std::string text = (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0')
                                 ? argv[1]
                                 : "https://github.com/bajraan/JR-QRMaker";

    gui::GuiApp app(text);
    return app.run();
}
