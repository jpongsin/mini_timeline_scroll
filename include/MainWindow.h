// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "VideoWidget.h"
#include "SubtitleImports.h"
#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QSlider>
#include <QTimer>

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(int argc, char* argv[], QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:
  void openVideo();
  void openVideoCLI(const QString &fileName) ;
  void importSubtitle();

  void get_embed_subtitles(const VideoState &vs, int64_t current_sid);
  void check_empty_subtitles(int64_t current_sid);
  void get_imported_subtitles(const VideoState &vs, int64_t current_sid);

  void closeVideo();
  void updateUI();
  void updateWindowShortcuts();

  // Playback
  void togglePlayPause();
  void seekForward1();
  void seekBackward1();
  void seekForward5();
  void seekBackward5();
  void resetToBeginning();
  void changeSpeed(double delta);
  void changeVolume(double delta);
  void toggleMute();

  // View
  void takeScreenshot();
  void assignScreenshotFolder();
  void toggleUninterruptedSnap(bool checked);
  void toggleFullscreen();

private:
  void createMenus();
  void createInfoBar();
  void createToolbar();
  void setupShortcuts();
  void populateAudioMenu(const VideoState &vs);
  void populateSubtitleMenu(const VideoState &vs);
  void clearImportedCheckmarks();
  void clearImportedCheckmarksExcept(int exceptId);
  QString formatTrackLabel(const SubtitleTrack &track);
  QString formatTimecode(double seconds, double fps);

  VideoWidget *m_videoWidget;
  QMenu *m_audioMenu{};
  QMenu *m_subtitleMenu{};
  QMenu *m_accelMenu{};
  QMenu *m_playbackMenu{};
  QActionGroup *m_audioActionGroup;
  QActionGroup *m_subtitleActionGroup;
  QActionGroup *m_accelActionGroup;
  QAction *m_closeAction{};
  QAction *m_screenshotAction{};
  QAction *m_assignSnapFolderAction{};
  QAction *m_accelAuto{};
  QAction *m_accelHardware{};
  QAction *m_accelSoftware{};

  // Screenshot state
  QString m_screenshotFolder;
  QString m_lastVideoDir;
  bool m_uninterruptedSnap = false;
  bool m_isIdle = true;

  // Info Bar
  QWidget *m_infoBar{};
  QLabel *m_filenameLabel{};
  QLabel *m_fpsLabel{};
  QLabel *m_speedLabel{};
  QLabel *m_volumeLabel{};

  // Toolbar
  QWidget *m_bottomToolbar{};
  QLabel *m_timecodeLabel{};
  QSlider *m_seekSlider{};

  QTimer *m_uiTimer;
  mpv_handle *m_mpv;
  double m_duration = 0;
  double m_fps = 30;
  int m_lastSubtitleCount = -1;

};

#endif // MAINWINDOW_H
