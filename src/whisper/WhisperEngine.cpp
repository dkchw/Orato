#include "WhisperEngine.h"
#include <QFileInfo>
#include <QDebug>
#include <thread>
#include "whisper.h"

WhisperWorker::WhisperWorker(QObject *parent)
    : QObject(parent) {
    m_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
}

WhisperWorker::~WhisperWorker() {
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

void WhisperWorker::setModelPath(const QString &path) {
    m_modelPath = path;
}

void WhisperWorker::setLanguage(const QString &lang) {
    m_language = lang;
}

void WhisperWorker::setTranslate(bool translate) {
    m_translate = translate;
}

void WhisperWorker::setThreads(int threads) {
    if (threads > 0) m_threads = threads;
}

void WhisperWorker::cancel() {
    m_abort.store(true);
}

bool WhisperWorker::loadModelIfNeeded() {
    if (m_modelPath.isEmpty() || !QFileInfo::exists(m_modelPath)) {
        emit error(tr("Model file does not exist: %1").arg(m_modelPath));
        return false;
    }

    if (m_ctx && m_loadedModelPath == m_modelPath) {
        return true;
    }

    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }

    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = false;

    m_ctx = whisper_init_from_file_with_params(m_modelPath.toUtf8().constData(), cparams);
    if (!m_ctx) {
        emit error(tr("Failed to initialize Whisper model from %1").arg(m_modelPath));
        return false;
    }

    m_loadedModelPath = m_modelPath;
    return true;
}

void WhisperWorker::processAudio(const std::vector<float> &pcmSamples) {
    m_abort.store(false);
    emit started();
    emit progress(5);

    if (pcmSamples.empty()) {
        emit error(tr("Audio buffer is empty."));
        return;
    }

    if (!loadModelIfNeeded()) {
        return;
    }

    emit progress(15);

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.translate = m_translate;
    wparams.language = m_language.isEmpty() ? "auto" : m_language.toUtf8().constData();
    wparams.n_threads = m_threads;
    wparams.audio_ctx = 0; // default

    emit progress(30);

    int result = whisper_full(m_ctx, wparams, pcmSamples.data(), static_cast<int>(pcmSamples.size()));

    if (m_abort.load()) {
        emit error(tr("Transcription cancelled."));
        return;
    }

    if (result != 0) {
        emit error(tr("Whisper processing failed with code %1").arg(result));
        return;
    }

    emit progress(85);

    QList<AudioSegment> segments;
    const int n_segments = whisper_full_n_segments(m_ctx);

    for (int i = 0; i < n_segments; ++i) {
        AudioSegment seg;
        seg.id = i + 1;
        // Whisper returns timestamps in 10-millisecond units
        seg.startMs = whisper_full_get_segment_t0(m_ctx, i) * 10;
        seg.endMs = whisper_full_get_segment_t1(m_ctx, i) * 10;

        const char *text = whisper_full_get_segment_text(m_ctx, i);
        seg.text = QString::fromUtf8(text).trimmed();

        if (!seg.text.isEmpty()) {
            segments.append(seg);
            emit segmentReady(seg);
        }
    }

    emit progress(100);
    emit finished(segments);
}

// -----------------------------------------------------------------------------
// WhisperEngine
// -----------------------------------------------------------------------------

WhisperEngine::WhisperEngine(QObject *parent)
    : QObject(parent) {
    m_worker = new WhisperWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(m_worker, &WhisperWorker::started, this, [this]() {
        m_isRunning = true;
        emit transcriptionStarted();
    });

    connect(m_worker, &WhisperWorker::progress, this, &WhisperEngine::progress);
    connect(m_worker, &WhisperWorker::segmentReady, this, &WhisperEngine::segmentDiscovered);

    connect(m_worker, &WhisperWorker::finished, this, [this](const QList<AudioSegment> &segs) {
        m_isRunning = false;
        emit transcriptionCompleted(segs);
    });

    connect(m_worker, &WhisperWorker::error, this, [this](const QString &err) {
        m_isRunning = false;
        emit transcriptionFailed(err);
    });

    m_workerThread.start();
}

WhisperEngine::~WhisperEngine() {
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(3000);
}

void WhisperEngine::setModelPath(const QString &modelPath) {
    m_modelPath = modelPath;
    m_worker->setModelPath(modelPath);
}

void WhisperEngine::setLanguage(const QString &lang) {
    m_language = lang;
    m_worker->setLanguage(lang);
}

void WhisperEngine::setTranslate(bool translate) {
    m_translate = translate;
    m_worker->setTranslate(translate);
}

void WhisperEngine::setThreads(int threads) {
    m_threads = threads;
    m_worker->setThreads(threads);
}

void WhisperEngine::transcribe(const std::vector<float> &pcmSamples) {
    if (m_isRunning) return;

    QMetaObject::invokeMethod(m_worker, "processAudio",
                             Qt::QueuedConnection,
                             Q_ARG(std::vector<float>, pcmSamples));
}

void WhisperEngine::cancel() {
    if (m_worker) {
        m_worker->cancel();
    }
}
