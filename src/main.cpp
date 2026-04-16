// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/MainWindow.h"
#include <QApplication>
#include <clocale>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  // libmpv requires LC_NUMERIC to be "C". QT prefers their own system locale
  // since libmpv is being used, overrule them
  setlocale(LC_NUMERIC, "C");

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

  MainWindow w(argc, argv);
  w.show();

  return a.exec();
}