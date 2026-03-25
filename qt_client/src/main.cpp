#include <QApplication>
#include <QStyle>
#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Set application style to avoid macOS native dialogs
    app.setStyle("Fusion");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
