// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/SubtitleImports.h"

#include <qboxlayout.h>
#include <QFileDialog>
#include <QMenu>
#include <QPushButton>

SubtitleImports::SubtitleImports(const QString &text, int id, QWidget *parent)
    : QWidget(parent), m_id(id)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 10, 2);

    m_radioButton = new QRadioButton(text, this);
    m_radioButton->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_deleteButton = new QPushButton("✕", this);

    // delete button
    m_deleteButton->setFixedSize(20, 20);

    layout->addWidget(m_radioButton, 1);
    layout->addWidget(m_deleteButton);

    // Connect signals
    connect(m_radioButton, &QRadioButton::clicked, this, [this]() {
        emit selected(m_id);
    });

    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_id);
    });
}


void SubtitleImports::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        setChecked(true);
        emit selected(m_id);
        // close after selection
        if (QMenu *menu = qobject_cast<QMenu*>(parentWidget())) {
            menu->close();
        }
    }
}

void SubtitleImports::setChecked(bool checked) {
    // blockSignals prevents an infinite loop
    m_radioButton->blockSignals(true);
    m_radioButton->setChecked(checked);
    m_radioButton->blockSignals(false);
}
int SubtitleImports::getId() const { return m_id; }
