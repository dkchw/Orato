#include "AudioUtils.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace AudioUtils {

bool writeWavFile(const QString &filePath, const int16_t *samples, size_t numSamples, uint32_t sampleRate, uint16_t channels) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing WAV:" << filePath;
        return false;
    }

    uint32_t dataBytes = static_cast<uint32_t>(numSamples * sizeof(int16_t));
    WavHeader header;
    header.numChannels = channels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = 16;
    header.blockAlign = channels * (header.bitsPerSample / 8);
    header.byteRate = sampleRate * header.blockAlign;
    header.dataSize = dataBytes;
    header.riffSize = 36 + dataBytes;

    if (file.write(reinterpret_cast<const char*>(&header), sizeof(header)) != sizeof(header)) {
        return false;
    }

    if (file.write(reinterpret_cast<const char*>(samples), dataBytes) != dataBytes) {
        return false;
    }

    file.close();
    return true;
}

bool writeWavFile(const QString &filePath, const std::vector<float> &samples, uint32_t sampleRate, uint16_t channels) {
    std::vector<int16_t> intSamples(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float s = std::max(-1.0f, std::min(1.0f, samples[i]));
        intSamples[i] = static_cast<int16_t>(s * 32767.0f);
    }
    return writeWavFile(filePath, intSamples.data(), intSamples.size(), sampleRate, channels);
}

bool loadWavToMono16k(const QString &filePath, std::vector<float> &outPcmF32, uint32_t &outDurationMs) {
    outPcmF32.clear();
    outDurationMs = 0;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open WAV file:" << filePath;
        return false;
    }

    QByteArray raw = file.readAll();
    file.close();

    if (raw.size() < 44) {
        qWarning() << "File too small to be a valid WAV:" << filePath;
        return false;
    }

    const char *data = raw.constData();
    if (std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0) {
        qWarning() << "Invalid WAV signature:" << filePath;
        return false;
    }

    // Parse RIFF chunks
    size_t offset = 12;
    uint16_t audioFormat = 1;
    uint16_t channels = 1;
    uint32_t sampleRate = 16000;
    uint16_t bitsPerSample = 16;
    const char *pcmData = nullptr;
    uint32_t pcmBytes = 0;

    while (offset + 8 <= static_cast<size_t>(raw.size())) {
        char chunkId[5] = {0};
        std::memcpy(chunkId, data + offset, 4);
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(data + offset + 4);
        offset += 8;

        if (std::strcmp(chunkId, "fmt ") == 0 && chunkSize >= 16) {
            audioFormat = *reinterpret_cast<const uint16_t*>(data + offset);
            channels = *reinterpret_cast<const uint16_t*>(data + offset + 2);
            sampleRate = *reinterpret_cast<const uint32_t*>(data + offset + 4);
            bitsPerSample = *reinterpret_cast<const uint16_t*>(data + offset + 14);
        } else if (std::strcmp(chunkId, "data") == 0) {
            pcmData = data + offset;
            pcmBytes = std::min(chunkSize, static_cast<uint32_t>(raw.size() - offset));
        }

        offset += chunkSize;
    }

    if (!pcmData || pcmBytes == 0) {
        qWarning() << "No data chunk found in WAV:" << filePath;
        return false;
    }

    std::vector<float> inputMono;

    if (audioFormat == 1) { // PCM integer
        if (bitsPerSample == 16) {
            size_t totalSamples = pcmBytes / sizeof(int16_t);
            size_t frameCount = totalSamples / channels;
            inputMono.resize(frameCount);
            const int16_t *s16 = reinterpret_cast<const int16_t*>(pcmData);

            for (size_t f = 0; f < frameCount; ++f) {
                float sum = 0.0f;
                for (int c = 0; c < channels; ++c) {
                    sum += static_cast<float>(s16[f * channels + c]) / 32768.0f;
                }
                inputMono[f] = sum / channels;
            }
        } else if (bitsPerSample == 8) {
            size_t frameCount = pcmBytes / channels;
            inputMono.resize(frameCount);
            const uint8_t *s8 = reinterpret_cast<const uint8_t*>(pcmData);

            for (size_t f = 0; f < frameCount; ++f) {
                float sum = 0.0f;
                for (int c = 0; c < channels; ++c) {
                    sum += (static_cast<float>(s8[f * channels + c]) - 128.0f) / 128.0f;
                }
                inputMono[f] = sum / channels;
            }
        } else {
            qWarning() << "Unsupported bit depth:" << bitsPerSample;
            return false;
        }
    } else if (audioFormat == 3) { // IEEE Float
        if (bitsPerSample == 32) {
            size_t totalSamples = pcmBytes / sizeof(float);
            size_t frameCount = totalSamples / channels;
            inputMono.resize(frameCount);
            const float *sf = reinterpret_cast<const float*>(pcmData);

            for (size_t f = 0; f < frameCount; ++f) {
                float sum = 0.0f;
                for (int c = 0; c < channels; ++c) {
                    sum += sf[f * channels + c];
                }
                inputMono[f] = sum / channels;
            }
        }
    } else {
        qWarning() << "Unsupported audio format code:" << audioFormat;
        return false;
    }

    if (sampleRate == 16000) {
        outPcmF32 = std::move(inputMono);
    } else {
        outPcmF32 = resample(inputMono, sampleRate, 16000);
    }

    outDurationMs = static_cast<uint32_t>((outPcmF32.size() * 1000) / 16000);
    return !outPcmF32.empty();
}

std::vector<float> resample(const std::vector<float> &input, uint32_t inRate, uint32_t outRate) {
    if (input.empty() || inRate == 0 || outRate == 0 || inRate == outRate) {
        return input;
    }

    double ratio = static_cast<double>(outRate) / static_cast<double>(inRate);
    size_t outSize = static_cast<size_t>(std::floor(input.size() * ratio));
    std::vector<float> output(outSize);

    for (size_t i = 0; i < outSize; ++i) {
        double srcIdx = i / ratio;
        size_t idx0 = static_cast<size_t>(std::floor(srcIdx));
        size_t idx1 = std::min(idx0 + 1, input.size() - 1);
        double frac = srcIdx - idx0;

        output[i] = static_cast<float>((1.0 - frac) * input[idx0] + frac * input[idx1]);
    }

    return output;
}

QVector<WaveformPeak> computeWaveformPeaks(const std::vector<float> &pcm, int numBuckets) {
    QVector<WaveformPeak> peaks;
    if (pcm.empty() || numBuckets <= 0) {
        return peaks;
    }

    peaks.resize(numBuckets);
    double samplesPerBucket = static_cast<double>(pcm.size()) / numBuckets;

    for (int i = 0; i < numBuckets; ++i) {
        size_t startIdx = static_cast<size_t>(i * samplesPerBucket);
        size_t endIdx = static_cast<size_t>((i + 1) * samplesPerBucket);
        endIdx = std::min(endIdx, pcm.size());

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (size_t s = startIdx; s < endIdx; ++s) {
            float val = pcm[s];
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }

        peaks[i].minVal = minVal;
        peaks[i].maxVal = maxVal;
    }

    return peaks;
}

void calculateLevels(const int16_t *data, size_t count, float &peak, float &rms) {
    if (!data || count == 0) {
        peak = 0.0f;
        rms = 0.0f;
        return;
    }

    float maxSample = 0.0f;
    double sumSquares = 0.0;

    for (size_t i = 0; i < count; ++i) {
        float absVal = std::abs(static_cast<float>(data[i])) / 32768.0f;
        if (absVal > maxSample) {
            maxSample = absVal;
        }
        sumSquares += absVal * absVal;
    }

    peak = std::min(1.0f, maxSample);
    rms = std::min(1.0f, static_cast<float>(std::sqrt(sumSquares / count)));
}

} // namespace AudioUtils
