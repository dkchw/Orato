#include "AudioRecorder.h"
#include "AudioUtils.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &AudioRecorder::onTimerTick);
    m_timer.setInterval(50);
}

AudioRecorder::~AudioRecorder() {
    stopRecording();
}

QList<QAudioDevice> AudioRecorder::availableDevices() const {
    return QMediaDevices::audioInputs();
}

QAudioDevice AudioRecorder::defaultDevice() const {
    return QMediaDevices::defaultAudioInput();
}

void AudioRecorder::setPcmSamples(const std::vector<float> &samples) {
    m_recordedPcm16k = samples;
    m_basePcmSamples = samples;
    m_baseDurationMs = static_cast<qint64>((samples.size() * 1000) / 16000);
    m_accumulatedMs = m_baseDurationMs;
}

void AudioRecorder::clearAudio() {
    m_recordedPcm16k.clear();
    m_basePcmSamples.clear();
    m_baseDurationMs = 0;
    m_accumulatedMs = 0;
}

qint64 AudioRecorder::durationMs() const {
    if (m_state == State::Recording) {
        return m_baseDurationMs + m_accumulatedMs + m_elapsedTimer.elapsed();
    }
    return m_baseDurationMs + m_accumulatedMs;
}

bool AudioRecorder::startRecording(const QAudioDevice &device, bool appendMode) {
    if (m_state == State::Recording) {
        return true;
    }

    m_appendMode = appendMode;
    m_currentDevice = device.isNull() ? defaultDevice() : device;
    if (m_currentDevice.isNull()) {
        emit errorOccurred(tr("No audio input device found."));
        return false;
    }

    QAudioFormat desiredFormat;
    desiredFormat.setSampleRate(16000);
    desiredFormat.setChannelCount(1);
    desiredFormat.setSampleFormat(QAudioFormat::Int16);

    if (m_currentDevice.isFormatSupported(desiredFormat)) {
        m_audioFormat = desiredFormat;
    } else {
        m_audioFormat = m_currentDevice.preferredFormat();
    }

    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    m_rawPcmBuffer.clear();
    m_accumulatedMs = 0;

    if (!appendMode) {
        m_recordedPcm16k.clear();
        m_basePcmSamples.clear();
        m_baseDurationMs = 0;
    } else {
        m_basePcmSamples = m_recordedPcm16k;
        m_baseDurationMs = static_cast<qint64>((m_basePcmSamples.size() * 1000) / 16000);
    }

    m_audioSource = new QAudioSource(m_currentDevice, m_audioFormat, this);
    m_ioDevice = m_audioSource->start();

    if (!m_ioDevice) {
        emit errorOccurred(tr("Failed to start audio recording source."));
        delete m_audioSource;
        m_audioSource = nullptr;
        return false;
    }

    connect(m_ioDevice, &QIODevice::readyRead, this, &AudioRecorder::onReadyRead);

    m_state = State::Recording;
    m_elapsedTimer.start();
    m_timer.start();

    emit stateChanged(m_state);
    return true;
}

void AudioRecorder::pauseRecording() {
    if (m_state != State::Recording) return;

    if (m_audioSource) {
        m_audioSource->suspend();
    }
    m_accumulatedMs += m_elapsedTimer.elapsed();
    m_state = State::Paused;
    emit stateChanged(m_state);
}

void AudioRecorder::resumeRecording() {
    if (m_state != State::Paused) return;

    if (m_audioSource) {
        m_audioSource->resume();
    }
    m_elapsedTimer.restart();
    m_state = State::Recording;
    emit stateChanged(m_state);
}

void AudioRecorder::stopRecording(const QString &saveWavPath) {
    if (m_state == State::Stopped) return;

    m_timer.stop();
    if (m_state == State::Recording) {
        m_accumulatedMs += m_elapsedTimer.elapsed();
    }

    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_ioDevice = nullptr;
    }

    m_state = State::Stopped;
    emit stateChanged(m_state);
    emit levelChanged(0.0f, 0.0f);

    std::vector<float> newlyRecorded = processRawBufferTo16k();

    if (m_appendMode && !m_basePcmSamples.empty()) {
        m_recordedPcm16k = m_basePcmSamples;
        m_recordedPcm16k.insert(m_recordedPcm16k.end(), newlyRecorded.begin(), newlyRecorded.end());
    } else {
        m_recordedPcm16k = std::move(newlyRecorded);
    }
    m_basePcmSamples = m_recordedPcm16k;
    m_baseDurationMs = static_cast<qint64>((m_recordedPcm16k.size() * 1000) / 16000);

    if (!saveWavPath.isEmpty() && !m_recordedPcm16k.empty()) {
        m_savePath = saveWavPath;
        AudioUtils::writeWavFile(saveWavPath, m_recordedPcm16k, 16000, 1);
    }

    emit recordingFinished(saveWavPath, m_baseDurationMs);
}

void AudioRecorder::onReadyRead() {
    if (!m_ioDevice) return;

    QByteArray chunk = m_ioDevice->readAll();
    if (chunk.isEmpty()) return;

    m_rawPcmBuffer.append(chunk);

    // Calculate level metrics for VU meter
    if (m_audioFormat.sampleFormat() == QAudioFormat::Int16) {
        const int16_t *samples = reinterpret_cast<const int16_t*>(chunk.constData());
        size_t count = chunk.size() / sizeof(int16_t);
        float peak = 0.0f, rms = 0.0f;
        AudioUtils::calculateLevels(samples, count, peak, rms);
        peak = std::min(1.0f, peak * m_inputGain);
        rms = std::min(1.0f, rms * m_inputGain);
        emit levelChanged(peak, rms);
    } else if (m_audioFormat.sampleFormat() == QAudioFormat::Float) {
        const float *samples = reinterpret_cast<const float*>(chunk.constData());
        size_t count = chunk.size() / sizeof(float);
        float peak = 0.0f;
        double sumSq = 0.0;
        for (size_t i = 0; i < count; ++i) {
            float a = std::abs(samples[i]);
            if (a > peak) peak = a;
            sumSq += a * a;
        }
        float rms = static_cast<float>(std::sqrt(sumSq / std::max<size_t>(count, 1)));
        peak = std::min(1.0f, peak * m_inputGain);
        rms = std::min(1.0f, rms * m_inputGain);
        emit levelChanged(peak, rms);
    }
}

void AudioRecorder::onTimerTick() {
    qint64 d = durationMs();
    emit durationChanged(d);

    if (m_state == State::Recording) {
        std::vector<float> currentChunk = processRawBufferTo16k();
        std::vector<float> liveSamples = m_basePcmSamples;
        liveSamples.insert(liveSamples.end(), currentChunk.begin(), currentChunk.end());
        emit liveAudioUpdated(liveSamples, d);
    }
}

std::vector<float> AudioRecorder::processRawBufferTo16k() {
    std::vector<float> result;
    if (m_rawPcmBuffer.isEmpty()) return result;

    int channels = m_audioFormat.channelCount();
    int sampleRate = m_audioFormat.sampleRate();
    if (channels <= 0 || sampleRate <= 0) return result;

    std::vector<float> monoSamples;

    if (m_audioFormat.sampleFormat() == QAudioFormat::Int16) {
        const int16_t *s16 = reinterpret_cast<const int16_t*>(m_rawPcmBuffer.constData());
        size_t totalSamples = m_rawPcmBuffer.size() / sizeof(int16_t);
        size_t frameCount = totalSamples / channels;
        monoSamples.resize(frameCount);

        for (size_t f = 0; f < frameCount; ++f) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c) {
                sum += static_cast<float>(s16[f * channels + c]) / 32768.0f;
            }
            monoSamples[f] = sum / channels;
        }
    } else if (m_audioFormat.sampleFormat() == QAudioFormat::Float) {
        const float *sf = reinterpret_cast<const float*>(m_rawPcmBuffer.constData());
        size_t totalSamples = m_rawPcmBuffer.size() / sizeof(float);
        size_t frameCount = totalSamples / channels;
        monoSamples.resize(frameCount);

        for (size_t f = 0; f < frameCount; ++f) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c) {
                sum += sf[f * channels + c];
            }
            monoSamples[f] = sum / channels;
        }
    }

    if (sampleRate == 16000) {
        result = std::move(monoSamples);
    } else {
        result = AudioUtils::resample(monoSamples, sampleRate, 16000);
    }

    if (m_inputGain != 1.0f) {
        for (auto &s : result) {
            s *= m_inputGain;
            if (s > 1.0f) s = 1.0f;
            else if (s < -1.0f) s = -1.0f;
        }
    }

    return result;
}
