#include "../include/UI/CinemaUIApp.h"
#include <iostream>

int main() {
    UI::CinemaUIApp app;
    if (!app.init()) {
        std::cerr << "Khong the khoi tao giao dien Cinema UI!\n";
        return 1;
    }
    app.run();
    return 0;
}

