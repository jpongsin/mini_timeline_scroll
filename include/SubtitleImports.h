//
// Created by jop on 4/18/26.
//

#ifndef MINI_TIMELINE_SCROLL_SUBTITLEIMPORTS_H
#define MINI_TIMELINE_SCROLL_SUBTITLEIMPORTS_H
#include <QPushButton>
#include <QRadioButton>
#include <QWidget>
#include <QtGui>
#include "VideoWidget.h"

class SubtitleImports : public QWidget {
    Q_OBJECT

public:
    explicit SubtitleImports(const QString &text, int id, QWidget *parent = nullptr);

    void setChecked(bool checked);

    int getId() const;

signals:
    void deleteRequested(int id);

    void selected(int id);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRadioButton *m_radioButton;
    QPushButton *m_deleteButton;
    int m_id;
};

#endif //MINI_TIMELINE_SCROLL_SUBTITLEIMPORTS_H
