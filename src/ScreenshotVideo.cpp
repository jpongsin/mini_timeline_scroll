// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/MainWindow.h"
#include "../include/SubtitleImports.h"

#include <QFileDialog>
#include <QStatusBar>

void MainWindow::assignScreenshotFolder() {
  //get available directory
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Screenshot Folder", m_screenshotFolder);
  //if directory exists
  if (!dir.isEmpty()) {m_screenshotFolder = dir;}
}

//change bool on a private variable
void MainWindow::toggleUninterruptedSnap(bool checked) {m_uninterruptedSnap = checked;}

void MainWindow::takeScreenshot() {
  //taking screenshots on an idle video
  //disable.
  if (!m_mpv || m_isIdle)
    return;

  // Pause if playing for a clean capture
  int paused = 0;
  mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &paused);

  //not paused; start pause.
  if (!paused) {
    int p = 1;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &p);
  }

  //screenshot folder will be usually the same as imported video's directory
  QString defaultDir = m_screenshotFolder;

  //directory will revert to home if not the case.
  if (defaultDir.isEmpty()) {
    defaultDir = m_lastVideoDir.isEmpty() ? QDir::homePath() : m_lastVideoDir;
  }

  //distract-free snapping
  if (m_uninterruptedSnap) {
    // auto-generate filename
    QString timestamp =
        QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString filename = QString("Screenshot_%1.png").arg(timestamp);
    QString fullPath = QDir(defaultDir).absoluteFilePath(filename);

    QString cmd = QString("screenshot-to-file \"%1\" video").arg(fullPath);
    mpv_command_string(m_mpv, cmd.toUtf8().constData());
    statusBar()->showMessage("Screenshot saved to: " + filename, 3000);
  } else {
    // prompt mode
    QString filter = "PNG Image (*.png);;JPEG Image (*.jpg);;WebP Image "
                     "(*.webp);;TIFF Image (*.tif)";
    QString selectedFilter;
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Screenshot", defaultDir, filter, &selectedFilter);

    if (!filePath.isEmpty()) {
      QString cmd = QString("screenshot-to-file \"%1\" video").arg(filePath);
      mpv_command_string(m_mpv, cmd.toUtf8().constData());
    }
  }
}