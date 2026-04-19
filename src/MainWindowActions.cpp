// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/MainWindow.h"
#include "../include/SubtitleImports.h"

#include <QActionGroup>
#include <QWidgetAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QStatusBar>
#include <algorithm>

void MainWindow::openVideo() {
    //check for known formats
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open Video", "",
        "Video Files (*.mp4 *.mkv "
        "*.mov *.avi "
        "*.webm *.flv "
        "*.vob *.ogv "
        "*.ogg *.wmv "
        "*.mpg *.mpeg "
        "*.m4v *.3gp "
        "*.3g2 *.mxf "
        "*.f4v)");

    //confirm loading video...
    if (!fileName.isEmpty()) {
        closeVideo();
        m_playbackMenu->setEnabled(true);
        updateWindowShortcuts();

        //reset speed and volume
        mpv_set_property_string(m_mpv, "volume", "100");
        mpv_set_property_string(m_mpv, "speed", "1.0");
        mpv_set_option_string(m_mpv, "pause", "yes");

        mpv_set_property_string(m_mpv, "video-aspect-override", "-1");
        m_lastVideoDir = QFileInfo(fileName).absolutePath();
        m_videoWidget->loadFile(fileName);

        QTimer::singleShot(500, this, [this]() {
            int64_t tf = 0;

            //account for unusual codecs for display
            if (mpv_get_property(m_mpv, "estimated-frame-count",
                                 MPV_FORMAT_INT64, &tf) >= 0 && tf > 0) {
                m_totalFrames = tf;
            } else if (m_duration > 0 && m_fps > 0) {
                m_totalFrames = (int64_t) (m_duration * m_fps);
            } else {
                return;
            }

            m_seekSlider->setRange(0, (int) m_totalFrames);
            int64_t sf = 0;
            mpv_get_property(m_mpv, "estimated-frame-number", MPV_FORMAT_INT64, &sf);
            m_startFrame = sf;
            m_metadataRefresh=true;
        });

        m_audioMenu->setEnabled(true);
        m_subtitleMenu->setEnabled(true);

        m_fpsLabel->setText("FPS: N/A"); //intermediate, before fps finally loaded
    }
}

void MainWindow::openVideoCLI(const QString &fileName) {
    if (fileName.isEmpty()) return;

    QByteArray utf8 = fileName.toUtf8();
    const char *cmd[] = {"loadfile", utf8.constData(), "replace", nullptr};
    mpv_command(m_mpv, cmd);

    m_playbackMenu->setEnabled(true);
    updateWindowShortcuts();
    mpv_set_property_string(m_mpv, "video-aspect-override", "-1");
    m_lastVideoDir = QFileInfo(fileName).absolutePath();
    m_videoWidget->loadFile(fileName);

    QTimer::singleShot(500, this, [this]() {
        int64_t tf = 0;

        //try normalizing for unusual codecs
        if (mpv_get_property(m_mpv, "estimated-frame-count",
                             MPV_FORMAT_INT64, &tf) >= 0 && tf > 0) {
            m_totalFrames = tf;
        } else if (m_duration > 0 && m_fps > 0) {
            m_totalFrames = (int64_t) (m_duration * m_fps);
        } else {
            return;
        }
        m_seekSlider->setRange(0, (int) m_totalFrames);
        int64_t sf = 0;
        mpv_get_property(m_mpv, "estimated-frame-number", MPV_FORMAT_INT64, &sf);
        m_startFrame = sf;
        m_metadataRefresh=true;
    });

    m_audioMenu->setEnabled(true);
    m_subtitleMenu->setEnabled(true);
    m_fpsLabel->setText("FPS: N/A"); // Reset until metadata probed
}

void MainWindow::populateAudioMenu(const VideoState &vs) {
    //clear audio
    m_audioMenu->clear();

    if (m_audioActionGroup) {
        delete m_audioActionGroup;
    }
    m_audioActionGroup = new QActionGroup(this);
    m_audioActionGroup->setExclusive(true);

    int64_t current_aid = 0;
    //no stream
    if (mpv_get_property(m_mpv, "aid", MPV_FORMAT_INT64, &current_aid) < 0) {
        current_aid = -1;
    }
    //populate audio track
    for (int i = 0; i < vs.nb_audio_tracks; i++) {
        const AudioTrack &tempTrack = vs.audio_tracks[i];

        // string to qstring
        QString lang = QString::fromUtf8(tempTrack.language).trimmed();
        QString title = QString::fromUtf8(tempTrack.title).trimmed();

        //if title track empty, leave empty
        QString label = QString("Track %1").arg(tempTrack.index);
        if (!lang.isEmpty()) label += QString(" [%1]").arg(lang);
        if (!title.isEmpty()) label += QString(" - %1").arg(title);

        //make sure the audio menu corresponds
        QAction *action = m_audioMenu->addAction(label);
        action->setCheckable(true);
        m_audioActionGroup->addAction(action);


        // Check the action if it matches the current active aid
        if (tempTrack.index == (int) current_aid) {
            action->setChecked(true);
        }
        int id = tempTrack.index;
        //reinforce
        connect(action, &QAction::triggered, this,
                [this, id]() { set_audio_track(m_mpv, id); });
    }
}

void MainWindow::importSubtitle() {
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Import Subtitles"), "",
                                                    tr(
                                                        "Subtitle Files (*.srt *.ass *.ssa *.vtt *.sub);;All Files (*)"));

    if (!fileName.isEmpty()) {
        QByteArray utf8 = fileName.toUtf8();
        const char *cmd[] = {"sub-add", utf8.constData(), "select", nullptr};
        mpv_command(m_mpv, cmd);
        m_metadataRefresh = true;

        // Clear the menu in order to repopulate.
        m_subtitleMenu->clear();
    }
}

//try to get embedded subtitles; soft subtitle
void MainWindow::get_embed_subtitles(const VideoState &vs, int64_t current_sid) {
    bool hasEmbedded = false;
    for (int i = 0; i < vs.nb_subtitle_tracks; i++) {
        if (vs.subtitle_tracks[i].is_external) continue;

        if (!hasEmbedded) {
            m_subtitleMenu->addSection("Embedded");
            hasEmbedded = true;
        }

        const auto &track = vs.subtitle_tracks[i];
        QString label = formatTrackLabel(track);

        QAction *action = m_subtitleMenu->QWidget::addAction(label);
        action->setCheckable(true);
        m_subtitleActionGroup->addAction(action);
        if (track.index == (int) current_sid) action->setChecked(true);

        int id = track.index;
        connect(action, &QAction::triggered, this, [this, id]() {
            // Clear all imported selections
            for (auto *row: m_subtitleMenu->findChildren<SubtitleImports *>()) {
                row->setChecked(false);
            }
            set_subtitle_track(m_mpv, id);
        });
    }
}

//check conditions for empty subtitles
void MainWindow::check_empty_subtitles(int64_t current_sid) {
    QAction *noneAction = m_subtitleMenu->QWidget::addAction("None");
    noneAction->setCheckable(true);
    m_subtitleActionGroup->addAction(noneAction);
    if (current_sid <= 0) noneAction->setChecked(true);
    connect(noneAction, &QAction::triggered, this, [this]() {
        // Clear all imported selections
        for (auto *row: m_subtitleMenu->findChildren<SubtitleImports *>()) {
            row->setChecked(false);
        }
        set_subtitle_track(m_mpv, -1);
    });
}

//after importing srt subtitles
void MainWindow::get_imported_subtitles(const VideoState &vs, int64_t current_sid) {
    bool hasImported = false;
    for (int i = 0; i < vs.nb_subtitle_tracks; i++) {
        if (!vs.subtitle_tracks[i].is_external) continue;

        if (!hasImported) {
            m_subtitleMenu->addSection("Imported");
            hasImported = true;
        }

        const SubtitleTrack &track = vs.subtitle_tracks[i];
        QString label = formatTrackLabel(track);

        QWidgetAction *widgetAction = new QWidgetAction(m_subtitleMenu);
        SubtitleImports *row = new SubtitleImports(label, track.index, m_subtitleMenu);

        // Handle Delete
        connect(row, &SubtitleImports::deleteRequested, this, [this, widgetAction](int id) {
            //remove from mpv
            QString cmd = QString("sub-remove %1").arg(id);
            mpv_command_string(m_mpv, cmd.toUtf8().constData());

            //remove from qmenu
            m_subtitleMenu->removeAction(widgetAction);
            widgetAction->deleteLater();
        });

        // Handle Selection
        connect(row, &SubtitleImports::selected, this, [this, id = track.index, row]() {
            // find custom rows
            for (auto *otherRow: m_subtitleMenu->findChildren<SubtitleImports *>()) {
                if (otherRow != row) {
                    otherRow->setChecked(false);
                }
            }
            // Clear embedded selection
            if (m_subtitleActionGroup->checkedAction()) {
                m_subtitleActionGroup->checkedAction()->setChecked(false);
            }
            row->setChecked(true);

            // Update mpv
            set_subtitle_track(m_mpv, id);
        });

        widgetAction->setDefaultWidget(row);
        m_subtitleMenu->QWidget::addAction(widgetAction);

        // Set initial checkmark
        if (track.index == (int) current_sid) {
            row->setChecked(true);
        }
    }
}

void MainWindow::populateSubtitleMenu(const VideoState &vs) {
    m_subtitleMenu->clear();

    if (m_subtitleActionGroup) delete m_subtitleActionGroup;
    m_subtitleActionGroup = new QActionGroup(this);
    m_subtitleActionGroup->setExclusive(true);

    int64_t current_sid = 0;
    if (mpv_get_property(m_mpv, "sid", MPV_FORMAT_INT64, &current_sid) < 0) {
        current_sid = -1;
    }

    QAction *importAction = m_subtitleMenu->addAction("Import Subtitles...");
    connect(importAction, &QAction::triggered, this, &MainWindow::importSubtitle);

    m_subtitleMenu->addSeparator();

    if (vs.nb_subtitle_tracks <= 0) {
        QAction *noTracks = m_subtitleMenu->addAction("No subtitles available");
        noTracks->setEnabled(false);
        return;
    }

    check_empty_subtitles(current_sid);
    get_embed_subtitles(vs, current_sid);
    get_imported_subtitles(vs, current_sid);

    //reinforce
    m_subtitleMenu->setEnabled(vs.nb_subtitle_tracks >= 0);
}

// Helper function to keep formatting consistent
QString MainWindow::formatTrackLabel(const SubtitleTrack &track) {
    QString lang = QString::fromUtf8(track.language).trimmed();
    QString title = QString::fromUtf8(track.title).trimmed();
    QString label = QString("Track %1").arg(track.index);
    if (!lang.isEmpty()) label += QString(" [%1]").arg(lang);
    if (!title.isEmpty()) label += QString(" - %1").arg(title);
    return label;
}


void MainWindow::closeVideo() {
    //if video already closed, dont do anything
    if (!m_mpv) return;

    if (m_audioActionGroup) {
        delete m_audioActionGroup;
        m_audioActionGroup = new QActionGroup(this);
        m_audioActionGroup->setExclusive(true);
    }
    if (m_subtitleActionGroup) {
        delete m_subtitleActionGroup;
        m_subtitleActionGroup = new QActionGroup(this);
        m_subtitleActionGroup->setExclusive(true);
    }

    //reset video and view
    m_videoWidget->resetVideo();
    m_videoWidget->resetView();
    m_playbackMenu->setEnabled(false);
    updateWindowShortcuts();

    //reset audio
    //need to depopulate it
    m_audioMenu->clear();
    m_audioMenu->setEnabled(false);

    m_subtitleMenu->clear();
    m_subtitleMenu->setEnabled(false);

    m_startFrame = 0;
    m_totalFrames = 0;
    m_seekSlider->setRange(0, 10000);
    //setup idle video again
    m_duration = 0;
    if (m_fps <= 0)
        m_fps = 30.0;

    //performance tweaking
    mpv_set_property_string(m_mpv, "volume", "100");
    mpv_set_property_string(m_mpv, "speed", "1.0");
    mpv_set_property_string(m_mpv, "video-aspect-override", "16:9");
    mpv_set_option_string(m_mpv, "pause", "yes");
    load_file(m_mpv, "av://lavfi:smptehdbars");
}

//mpv arguments
void MainWindow::changeSpeed(double delta) {
    double current = 1.0;
    mpv_get_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &current);
    double next = std::clamp(current + delta, 0.25, 2.0);
    mpv_set_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &next);
}

void MainWindow::changeVolume(double delta) {
    double current = 100.0;
    mpv_get_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &current);
    double next = std::clamp(current + delta, 0.0, 100.0);
    mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &next);
}

void MainWindow::toggleMute() {
    int mute = 0;
    mpv_get_property(m_mpv, "mute", MPV_FORMAT_FLAG, &mute);
    mute = !mute;
    mpv_set_property(m_mpv, "mute", MPV_FORMAT_FLAG, &mute);
}

//mpv commands
void MainWindow::togglePlayPause() { toggle_pause(m_mpv); }
void MainWindow::seekForward1() { mpv_command_string(m_mpv, "frame-step"); }
void MainWindow::seekBackward1() { mpv_command_string(m_mpv, "frame-back-step"); }
void MainWindow::seekForward5() { mpv_command_string(m_mpv, "seek 5 relative exact"); }
void MainWindow::seekBackward5() { mpv_command_string(m_mpv, "seek -5 relative exact"); }
void MainWindow::resetToBeginning() { seek_absolute(m_mpv, 0, 1); }
void MainWindow::toggleFullscreen() { if (isFullScreen()) { showNormal(); } else { showFullScreen(); } }
