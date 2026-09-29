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

std::vector<float> computePitchTrack(const std::vector<float> &pcm, int sampleRate, int hopSize, int frameSize) {
    std::vector<float> pitchTrack;
    if (pcm.size() < static_cast<size_t>(frameSize)) {
        return pitchTrack;
    }

    const int minLag = std::max(1, sampleRate / 380); // ~42 samples at 16k (~380 Hz)
    const int maxLag = std::min(frameSize - 1, sampleRate / 65); // ~246 samples at 16k (~65 Hz)
    const size_t numFrames = (pcm.size() - frameSize) / hopSize + 1;
    pitchTrack.resize(numFrames, 0.0f);

    for (size_t f = 0; f < numFrames; ++f) {
        size_t start = f * hopSize;

        float energy = 0.0f;
        for (int n = 0; n < frameSize; ++n) {
            float s = pcm[start + n];
            energy += s * s;
        }
        float rms = std::sqrt(energy / frameSize);

        if (rms < 0.015f) {
            pitchTrack[f] = 0.0f;
            continue;
        }

        float bestCorr = -1.0f;
        int bestLag = -1;

        std::vector<float> r(maxLag + 2, 0.0f);
        for (int lag = minLag - 1; lag <= maxLag + 1; ++lag) {
            float sum = 0.0f;
            float sumSq0 = 0.0f;
            float sumSqLag = 0.0f;
            int count = frameSize - lag;
            for (int n = 0; n < count; ++n) {
                float s0 = pcm[start + n];
                float sL = pcm[start + n + lag];
                sum += s0 * sL;
                sumSq0 += s0 * s0;
                sumSqLag += sL * sL;
            }
            float norm = std::sqrt(sumSq0 * sumSqLag) + 1e-9f;
            r[lag] = sum / norm;

            if (lag >= minLag && lag <= maxLag && r[lag] > bestCorr) {
                bestCorr = r[lag];
                bestLag = lag;
            }
        }

        if (bestCorr > 0.40f && bestLag > minLag && bestLag < maxLag) {
            float y0 = r[bestLag - 1];
            float y1 = r[bestLag];
            float y2 = r[bestLag + 1];
            float denom = 2.0f * (2.0f * y1 - y0 - y2);
            float delta = 0.0f;
            if (std::abs(denom) > 1e-6f) {
                delta = (y2 - y0) / denom;
            }
            float refinedLag = static_cast<float>(bestLag) + delta;
            float f0 = static_cast<float>(sampleRate) / refinedLag;
            if (f0 >= 60.0f && f0 <= 400.0f) {
                pitchTrack[f] = f0;
            }
        }
    }

    // 3-point median filter
    if (pitchTrack.size() >= 3) {
        for (size_t i = 1; i < pitchTrack.size() - 1; ++i) {
            if (pitchTrack[i] > 0.0f && pitchTrack[i-1] > 0.0f && pitchTrack[i+1] > 0.0f) {
                float a = pitchTrack[i-1], b = pitchTrack[i], c = pitchTrack[i+1];
                float med = std::max(std::min(a, b), std::min(std::max(a, b), c));
                pitchTrack[i] = med;
            }
        }
    }

    return pitchTrack;
}

std::vector<float> computeEnergyTrack(const std::vector<float> &pcm, int hopSize, int frameSize) {
    std::vector<float> energyTrack;
    if (pcm.size() < static_cast<size_t>(frameSize)) {
        return energyTrack;
    }

    const size_t numFrames = (pcm.size() - frameSize) / hopSize + 1;
    energyTrack.resize(numFrames, 0.0f);
    float maxEnergy = 1e-6f;

    for (size_t f = 0; f < numFrames; ++f) {
        size_t start = f * hopSize;
        float sumSq = 0.0f;
        for (int n = 0; n < frameSize; ++n) {
            float s = pcm[start + n];
            sumSq += s * s;
        }
        float rms = std::sqrt(sumSq / frameSize);
        energyTrack[f] = rms;
        if (rms > maxEnergy) {
            maxEnergy = rms;
        }
    }

    for (float &e : energyTrack) {
        e = std::min(1.0f, e / maxEnergy);
    }

    return energyTrack;
}

SpeechMetrics analyzeSpeech(const std::vector<float> &pcm, qint64 durationMs, int wordCount) {
    SpeechMetrics m;
    if (pcm.empty() || durationMs <= 0) {
        return m;
    }

    auto pitch = computePitchTrack(pcm);
    auto energy = computeEnergyTrack(pcm);

    std::vector<float> voiced;
    voiced.reserve(pitch.size());
    for (float p : pitch) {
        if (p > 50.0f) {
            voiced.push_back(p);
        }
    }

    if (!voiced.empty()) {
        float sum = 0.0f;
        m.minPitchHz = voiced[0];
        m.maxPitchHz = voiced[0];
        for (float p : voiced) {
            sum += p;
            m.minPitchHz = std::min(m.minPitchHz, p);
            m.maxPitchHz = std::max(m.maxPitchHz, p);
        }
        m.meanPitchHz = sum / voiced.size();

        float varSum = 0.0f;
        for (float p : voiced) {
            float diff = p - m.meanPitchHz;
            varSum += diff * diff;
        }
        m.pitchVariation = std::sqrt(varSum / voiced.size());

        size_t third = voiced.size() / 3;
        if (third > 0) {
            float firstSum = 0.0f, lastSum = 0.0f;
            for (size_t i = 0; i < third; ++i) firstSum += voiced[i];
            for (size_t i = voiced.size() - third; i < voiced.size(); ++i) lastSum += voiced[i];
            float firstMean = firstSum / third;
            float lastMean = lastSum / third;

            if (lastMean > firstMean + 12.0f) {
                m.intonationTrend = QString("Rising ↗");
            } else if (lastMean < firstMean - 12.0f) {
                m.intonationTrend = QString("Falling ↘");
            } else if (m.pitchVariation > 24.0f) {
                m.intonationTrend = QString("Dynamic 〰");
            } else {
                m.intonationTrend = QString("Steady →");
            }
        } else {
            m.intonationTrend = QString("Steady →");
        }
    } else {
        m.intonationTrend = QString("Neutral / Unvoiced");
    }

    size_t activeCount = 0;
    float peakVal = 0.0f;
    for (float s : pcm) {
        float a = std::abs(s);
        if (a > peakVal) peakVal = a;
    }
    for (float e : energy) {
        if (e > 0.08f) {
            activeCount++;
        }
    }
    if (!energy.empty()) {
        m.speechRatioPercent = (static_cast<float>(activeCount) / energy.size()) * 100.0f;
        m.pauseRatioPercent = 100.0f - m.speechRatioPercent;
    }

    m.peakDbfs = 20.0f * std::log10(std::max(1e-4f, peakVal));
    m.dynamicRangeDb = std::min(45.0f, std::max(5.0f, (m.peakDbfs + 45.0f)));

    double durMin = durationMs / 60000.0;
    if (wordCount > 0 && durMin > 0.01) {
        m.wordsPerMinute = static_cast<int>(wordCount / durMin);
    } else if (durMin > 0.01) {
        int pulses = 0;
        bool above = false;
        for (float e : energy) {
            if (e > 0.22f && !above) {
                pulses++;
                above = true;
            } else if (e < 0.12f && above) {
                above = false;
            }
        }
        m.wordsPerMinute = static_cast<int>((pulses / 1.35) / durMin);
    }

    if (m.wordsPerMinute < 115) {
        m.tempoRating = QString("Relaxed Pace");
    } else if (m.wordsPerMinute <= 165) {
        m.tempoRating = QString("Natural Pace");
    } else {
        m.tempoRating = QString("Brisk / Fast Pace");
    }

    return m;
}

} // namespace AudioUtils
