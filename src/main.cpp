//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#include "../include/video_window.h"

#ifdef __linux__

#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease

#endif

//wrapper for app to reinforce hotkeys
class HotkeyApp : public QApplication {
public:
    //constructor
    HotkeyApp(int &argc, char **argv) : QApplication(argc, argv), videoWin(nullptr) {
    }

    VideoWindow *videoWin;

    //if a hotkey was pressed while videowin is active
    //ensure the keys are accepted
    bool notify(QObject *receiver, QEvent *event) override {
        if (event->type() == QEvent::KeyPress && videoWin) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (handle_hotkeys(keyEvent, &videoWin->player, videoWin)) {
                event->accept();
                return true;
            }
        }
        return QApplication::notify(receiver, event);
    }
};

//main program
int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
#ifdef __linux__
    XInitThreads();
#endif
    HotkeyApp a(argc, argv);

    //  set color of app
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    a.setPalette(darkPalette);
    // ---------------------------

    VideoWindow window(argc, argv);
    a.videoWin = &window;

    window.show();
    return a.exec();
}
