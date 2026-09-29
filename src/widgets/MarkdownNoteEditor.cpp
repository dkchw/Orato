#include "MarkdownNoteEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextCursor>
#include <QFontDatabase>
#include <QUrl>

MarkdownNoteEditor::MarkdownNoteEditor(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void MarkdownNoteEditor::setupUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    setupToolbar();
    layout->addWidget(m_toolBar);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    // Left Editor
    m_editor = new QPlainTextEdit(m_splitter);
    m_editor->setPlaceholderText(tr("Type markdown notes here...\nUse formatting toolbar or write standard Markdown."));
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(11);
    m_editor->setFont(monoFont);
    m_editor->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1e2027;"
        "  color: #f1f2f6;"
        "  border: 1px solid #363945;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  line-height: 1.5;"
        "}"
    );

    // Right Preview
    m_preview = new QTextBrowser(m_splitter);
    m_preview->setOpenExternalLinks(false);
    m_preview->setOpenLinks(false);
    m_preview->setStyleSheet(
        "QTextBrowser {"
        "  background-color: #18191e;"
        "  color: #e4e7eb;"
        "  border: 1px solid #363945;"
        "  border-radius: 4px;"
        "  padding: 12px;"
        "}"
        "a { color: #70a1ff; text-decoration: none; font-weight: bold; }"
        "a:hover { text-decoration: underline; color: #1e90ff; }"
        "h1 { color: #70a1ff; border-bottom: 1px solid #363945; padding-bottom: 4px; }"
        "h2 { color: #2ed573; border-bottom: 1px solid #363945; padding-bottom: 3px; }"
        "h3 { color: #ffa502; }"
        "code { background-color: #2f3542; color: #ff6b81; border-radius: 3px; padding: 2px 4px; }"
        "blockquote { border-left: 3px solid #70a1ff; margin-left: 0; padding-left: 8px; color: #a4b0be; }"
    );

    connect(m_editor, &QPlainTextEdit::textChanged, this, &MarkdownNoteEditor::onEditorTextChanged);
    connect(m_preview, &QTextBrowser::anchorClicked, this, &MarkdownNoteEditor::onAnchorClicked);

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_preview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    layout->addWidget(m_splitter, 1);
}

void MarkdownNoteEditor::setupToolbar() {
    m_toolBar = new QToolBar(tr("Markdown Tools"), this);
    m_toolBar->setIconSize(QSize(16, 16));
    m_toolBar->setStyleSheet(
        "QToolBar {"
        "  background-color: #23252d;"
        "  border: 1px solid #363945;"
        "  border-radius: 4px;"
        "  padding: 2px;"
        "  spacing: 4px;"
        "}"
        "QToolButton {"
        "  color: #f1f2f6;"
        "  background: #313543;"
        "  border-radius: 3px;"
        "  padding: 3px 6px;"
        "  font-weight: bold;"
        "}"
        "QToolButton:hover { background: #404456; }"
    );

    auto *h1Action = m_toolBar->addAction("H1");
    connect(h1Action, &QAction::triggered, this, [this]() { insertHeader(1); });

    auto *h2Action = m_toolBar->addAction("H2");
    connect(h2Action, &QAction::triggered, this, [this]() { insertHeader(2); });

    auto *h3Action = m_toolBar->addAction("H3");
    connect(h3Action, &QAction::triggered, this, [this]() { insertHeader(3); });

    m_toolBar->addSeparator();

    auto *boldAction = m_toolBar->addAction("B");
    boldAction->setToolTip(tr("Bold (**text**)"));
    connect(boldAction, &QAction::triggered, this, &MarkdownNoteEditor::insertBold);

    auto *italicAction = m_toolBar->addAction("I");
    italicAction->setToolTip(tr("Italic (*text*)"));
    connect(italicAction, &QAction::triggered, this, &MarkdownNoteEditor::insertItalic);

    m_toolBar->addSeparator();

    auto *bulletAction = m_toolBar->addAction("• List");
    bulletAction->setToolTip(tr("Bullet List"));
    connect(bulletAction, &QAction::triggered, this, &MarkdownNoteEditor::insertBulletList);

    auto *checkAction = m_toolBar->addAction("☑ Task");
    checkAction->setToolTip(tr("Task Item"));
    connect(checkAction, &QAction::triggered, this, &MarkdownNoteEditor::insertCheckbox);

    auto *quoteAction = m_toolBar->addAction("” Quote");
    quoteAction->setToolTip(tr("Blockquote"));
    connect(quoteAction, &QAction::triggered, this, &MarkdownNoteEditor::insertBlockquote);

    auto *codeAction = m_toolBar->addAction("<> Code");
    codeAction->setToolTip(tr("Inline or block code"));
    connect(codeAction, &QAction::triggered, this, &MarkdownNoteEditor::insertCode);
}

QString MarkdownNoteEditor::markdownText() const {
    return m_editor->toPlainText();
}

void MarkdownNoteEditor::setMarkdownText(const QString &text) {
    if (m_editor->toPlainText() == text) return;
    m_editor->setPlainText(text);
    m_preview->setMarkdown(text);
}

void MarkdownNoteEditor::onEditorTextChanged() {
    QString md = m_editor->toPlainText();
    m_preview->setMarkdown(md);
    emit textChanged();
}

void MarkdownNoteEditor::onAnchorClicked(const QUrl &url) {
    if (url.scheme() == "time") {
        qint64 ms = url.host().toLongLong();
        if (ms == 0 && !url.path().isEmpty()) {
            ms = url.path().mid(1).toLongLong();
        }
        emit timestampClicked(ms);
    }
}

void MarkdownNoteEditor::wrapSelectedText(const QString &prefix, const QString &suffix) {
    QTextCursor cursor = m_editor->textCursor();
    QString selected = cursor.selectedText();
    if (selected.isEmpty()) {
        cursor.insertText(prefix + tr("text") + suffix);
    } else {
        cursor.insertText(prefix + selected + suffix);
    }
    m_editor->setFocus();
}

void MarkdownNoteEditor::insertHeader(int level) {
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    QString hashes(level, '#');
    cursor.insertText(hashes + " ");
    m_editor->setFocus();
}

void MarkdownNoteEditor::insertBold() {
    wrapSelectedText("**", "**");
}

void MarkdownNoteEditor::insertItalic() {
    wrapSelectedText("*", "*");
}

void MarkdownNoteEditor::insertBulletList() {
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText("- ");
    m_editor->setFocus();
}

void MarkdownNoteEditor::insertCheckbox() {
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText("- [ ] ");
    m_editor->setFocus();
}

void MarkdownNoteEditor::insertBlockquote() {
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText("> ");
    m_editor->setFocus();
}

void MarkdownNoteEditor::insertCode() {
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText().contains('\n')) {
        wrapSelectedText("```\n", "\n```\n");
    } else {
        wrapSelectedText("`", "`");
    }
}

void MarkdownNoteEditor::insertTimestamp(qint64 ms) {
    int totalSec = static_cast<int>(ms / 1000);
    int minutes = totalSec / 60;
    int seconds = totalSec % 60;
    int tenths = static_cast<int>((ms % 1000) / 100);
    QString timeStr = QString("%1:%2.%3")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(tenths);

    QString link = QString("[%1](time://%2)").arg(timeStr).arg(ms);
    m_editor->textCursor().insertText(link + " ");
    m_editor->setFocus();
}

void MarkdownNoteEditor::appendSentence(qint64 timestampMs, const QString &text) {
    int totalSec = static_cast<int>(timestampMs / 1000);
    int minutes = totalSec / 60;
    int seconds = totalSec % 60;
    int tenths = static_cast<int>((timestampMs % 1000) / 100);
    QString timeStr = QString("%1:%2.%3")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(tenths);

    QString line = QString("\n- [%1](time://%2) %3\n").arg(timeStr).arg(timestampMs).arg(text);

    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line);
    m_editor->setTextCursor(cursor);
}
