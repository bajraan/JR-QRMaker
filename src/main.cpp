#include <iostream>
#include <string>
#include <vector>
#include "qrcodegen.hpp"
#include "SvgWriter.hpp" // Za chwilę dopniemy ten nagłówek, albo zrobimy bazową wersję zapisu

int main() {
    try {
        // Dane do zakodowania w QR (np. link do Twojego GitHub / portfolio)
        std::string text = "https://github.com/twoj-profil";
        
        // Wybór poziomu korekcji błędów (Medium)
        qrcodegen::QrCode::Ecc errCorLev = qrcodegen::QrCode::Ecc::MEDIUM;

        // Generowanie kodu QR za pomocą silnika Nayukiego
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.c_str(), errCorLev);

        std::cout << "[INFO] QR Code successfully generated!" << std::endl;
        std::cout << "[INFO] QR Code Version: " << qr.getSize() << std::endl;

        // Tutaj w kolejnym kroku podepniemy nasz moduł SvgWriter do eksportu do pliku .svg
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}