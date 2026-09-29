#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QSplitter>
#include <QToolBar>
#include <QAction>

class MarkdownNoteEditor : public QWidget {
    Q_OBJECT

public:
    explicit MarkdownNoteEditor(QWidget *parent = nullptr);
    ~MarkdownNoteEditor() override = default;

    QString markdownText() const;
    void setMarkdownText(const QString &text);
    void appendSentence(qint64 timestampMs, const QString &text);

public slots:
    void insertTimestamp(qint64 ms);

signals:
    void textChanged();
    void timestampClicked(qint64 positionMs);

private slots:
    void onEditorTextChanged();
    void onAnchorClicked(const QUrl &url);

    void insertHeader(int level);
    void insertBold();
    void insertItalic();
    void insertBulletList();
    void insertCheckbox();
    void insertCode();
    void insertBlockquote();

private:
    void setupUi();
    void setupToolbar();
    void wrapSelectedText(const QString &prefix, const QString &suffix);

    QToolBar *m_toolBar = nullptr;
    QSplitter *m_splitter = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QTextBrowser *m_preview = nullptr;
};
