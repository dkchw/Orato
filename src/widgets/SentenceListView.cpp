#include "SentenceListView.h"
#include <QHBoxLayout>
#include <QScrollBar>
#include <QStyle>

SentenceItemWidget::SentenceItemWidget(const AudioSegment &segment, QWidget *parent)
    : QFrame(parent), m_segment(segment) {
    setObjectName("SentenceItemWidget");
    setFrameShape(QFrame::StyledPanel);
    setStyleSheet(
        "QFrame#SentenceItemWidget {"
        "  background-color: #23252d;"
        "  border: 1px solid #363945;"
        "  border-radius: 6px;"
        "  margin: 2px 4px;"
        "  padding: 4px;"
        "}"
    );

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(10);

    // Badge label (#1, #2...)
    m_badgeLabel = new QLabel(QString("#%1").arg(segment.id), this);
    m_badgeLabel->setStyleSheet(
        "background-color: #313543; color: #70a1ff; font-weight: bold; border-radius: 3px; padding: 2px 6px;"
    );
    mainLayout->addWidget(m_badgeLabel);

    // Time label (00:01.2 - 00:04.5)
    m_timeLabel = new QLabel(segment.formatTimeRange(), this);
    m_timeLabel->setStyleSheet("color: #a4b0be; font-family: monospace; font-size: 11px;");
    mainLayout->addWidget(m_timeLabel);

    // Text editor
    m_textEdit = new QLineEdit(segment.text, this);
    m_textEdit->setStyleSheet(
        "QLineEdit {"
        "  background-color: transparent;"
        "  color: #f1f2f6;"
        "  border: 1px solid transparent;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "  background-color: #1e2027;"
        "  border: 1px solid #70a1ff;"
        "}"
    );
    connect(m_textEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_segment.text = text;
        emit textEdited(m_segment.id, text);
    });
    mainLayout->addWidget(m_textEdit, 1);

    // Action buttons
    m_playBtn = new QPushButton(tr("Play"), this);
    m_playBtn->setIcon(QIcon(":/icons/play.svg"));
    m_playBtn->setToolTip(tr("Hear this sentence from recording"));
    m_playBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #059669; color: #ffffff; font-weight: 600; border-radius: 4px; padding: 4px 10px;"
        "}"
        "QPushButton:hover { background-color: #10b981; }"
    );
    connect(m_playBtn, &QPushButton::clicked, this, [this]() {
        emit playRequested(m_segment.id, m_segment.startMs, m_segment.endMs);
    });
    mainLayout->addWidget(m_playBtn);

    m_loopBtn = new QPushButton(tr("Loop"), this);
    m_loopBtn->setIcon(QIcon(":/icons/loop.svg"));
    m_loopBtn->setToolTip(tr("Loop this sentence repeatedly for shadowing practice"));
    m_loopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #4338ca; color: white; border-radius: 4px; padding: 4px 8px;"
        "}"
        "QPushButton:hover { background-color: #4f46e5; }"
    );
    connect(m_loopBtn, &QPushButton::clicked, this, [this]() {
        emit loopRequested(m_segment.id, m_segment.startMs, m_segment.endMs);
    });
    mainLayout->addWidget(m_loopBtn);

    m_ttsBtn = new QPushButton(tr("TTS"), this);
    m_ttsBtn->setIcon(QIcon(":/icons/volume.svg"));
    m_ttsBtn->setToolTip(tr("Generate or listen to native Pocket TTS pronunciation"));
    m_ttsBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #d97706; color: #ffffff; font-weight: 600; border-radius: 4px; padding: 4px 8px;"
        "}"
        "QPushButton:hover { background-color: #f59e0b; }"
    );
    connect(m_ttsBtn, &QPushButton::clicked, this, [this]() {
        emit ttsRequested(m_segment.id, m_segment.text);
    });
    mainLayout->addWidget(m_ttsBtn);

    m_noteBtn = new QPushButton(tr("Note"), this);
    m_noteBtn->setIcon(QIcon(":/icons/file-text.svg"));
    m_noteBtn->setToolTip(tr("Append sentence with timestamp into Markdown note"));
    m_noteBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #334155; color: white; border-radius: 4px; padding: 4px 8px;"
        "}"
        "QPushButton:hover { background-color: #475569; }"
    );
    connect(m_noteBtn, &QPushButton::clicked, this, [this]() {
        emit sendToNoteRequested(m_segment.id, m_segment.startMs, m_segment.text);
    });
    mainLayout->addWidget(m_noteBtn);
}

void SentenceItemWidget::setActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;

    if (active) {
        setStyleSheet(
            "QFrame#SentenceItemWidget {"
            "  background-color: #064e3b;"
            "  border: 1px solid #10b981;"
            "  border-radius: 6px;"
            "  margin: 2px 4px;"
            "  padding: 4px;"
            "}"
        );
        m_badgeLabel->setStyleSheet(
            "background-color: #10b981; color: #022c22; font-weight: bold; border-radius: 3px; padding: 2px 6px;"
        );
    } else {
        setStyleSheet(
            "QFrame#SentenceItemWidget {"
            "  background-color: #1e2027;"
            "  border: 1px solid #333846;"
            "  border-radius: 6px;"
            "  margin: 2px 4px;"
            "  padding: 4px;"
            "}"
        );
        m_badgeLabel->setStyleSheet(
            "background-color: #2b303c; color: #60a5fa; font-weight: bold; border-radius: 3px; padding: 2px 6px;"
        );
    }
}

void SentenceItemWidget::updateTtsStatus(bool hasAudio) {
    if (hasAudio) {
        m_ttsBtn->setText(tr("TTS (Ready)"));
        m_ttsBtn->setIcon(QIcon(":/icons/play-white.svg"));
        m_ttsBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #e11d48; color: white; font-weight: bold; border-radius: 4px; padding: 4px 8px;"
            "}"
            "QPushButton:hover { background-color: #f43f5e; }"
        );
    }
}

// -----------------------------------------------------------------------------
// SentenceListView
// -----------------------------------------------------------------------------

SentenceListView::SentenceListView(QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #18191e; }");

    m_containerWidget = new QWidget(m_scrollArea);
    m_containerWidget->setStyleSheet("background-color: #18191e;");
    m_containerLayout = new QVBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(4, 4, 4, 4);
    m_containerLayout->setSpacing(4);
    m_containerLayout->addStretch(1);

    m_scrollArea->setWidget(m_containerWidget);
    layout->addWidget(m_scrollArea);
}

void SentenceListView::clearList() {
    for (auto *item : m_itemWidgets) {
        m_containerLayout->removeWidget(item);
        item->deleteLater();
    }
    m_itemWidgets.clear();
    m_activeSegmentId = -1;
}

void SentenceListView::setSegments(const QList<AudioSegment> &segments) {
    clearList();

    for (const auto &seg : segments) {
        auto *item = new SentenceItemWidget(seg, m_containerWidget);

        connect(item, &SentenceItemWidget::playRequested, this, [this](int, qint64 startMs, qint64 endMs) {
            emit playSentenceRequested(startMs, endMs, false);
        });

        connect(item, &SentenceItemWidget::loopRequested, this, [this](int, qint64 startMs, qint64 endMs) {
            emit playSentenceRequested(startMs, endMs, true);
        });

        connect(item, &SentenceItemWidget::ttsRequested, this, [this](int id, const QString &text) {
            emit ttsSentenceRequested(id, text);
        });

        connect(item, &SentenceItemWidget::sendToNoteRequested, this, [this](int, qint64 startMs, const QString &text) {
            emit appendToNoteRequested(startMs, text);
        });

        connect(item, &SentenceItemWidget::textEdited, this, [this](int, const QString &) {
            emit segmentsChanged();
        });

        if (!seg.ttsAudioPath.isEmpty()) {
            item->updateTtsStatus(true);
        }

        // Insert before the bottom stretch
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, item);
        m_itemWidgets.append(item);
    }
}

QList<AudioSegment> SentenceListView::segments() const {
    QList<AudioSegment> list;
    for (const auto *item : m_itemWidgets) {
        list.append(item->segment());
    }
    return list;
}

void SentenceListView::updatePlaybackPosition(qint64 ms) {
    int currentId = -1;
    for (auto *item : m_itemWidgets) {
        bool inRange = (ms >= item->segment().startMs && ms <= item->segment().endMs);
        item->setActive(inRange);
        if (inRange) {
            currentId = item->segment().id;
        }
    }

    if (currentId != -1 && currentId != m_activeSegmentId) {
        m_activeSegmentId = currentId;
        // Scroll into view
        for (auto *item : m_itemWidgets) {
            if (item->segment().id == currentId) {
                m_scrollArea->ensureWidgetVisible(item, 50, 50);
                break;
            }
        }
    }
}

void SentenceListView::markTtsAvailable(int id, const QString &audioPath) {
    for (auto *item : m_itemWidgets) {
        if (item->segment().id == id) {
            item->updateTtsStatus(true);
            break;
        }
    }
}
