#pragma once

#include <QString>
#include <QVector>
#include <vector>
#include <cstdint>

namespace AudioUtils {

// WAV Header struct (standard 44-byte PCM)
#pragma pack(push, 1)
struct WavHeader {
    char riffTag[4] = {'R', 'I', 'F', 'F'};
    uint32_t riffSize = 0;
    char waveTag[4] = {'W', 'A', 'V', 'E'};
    char fmtTag[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM = 1
    uint16_t numChannels = 1;
    uint32_t sampleRate = 16000;
    uint32_t byteRate = 32000; // sampleRate * numChannels * bitsPerSample / 8
    uint16_t blockAlign = 2;   // numChannels * bitsPerSample / 8
    uint16_t bitsPerSample = 16;
    char dataTag[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};
#pragma pack(pop)

// Saves 16-bit PCM samples to a WAV file
bool writeWavFile(const QString &filePath, const int16_t *samples, size_t numSamples, uint32_t sampleRate = 16000, uint16_t channels = 1);
bool writeWavFile(const QString &filePath, const std::vector<float> &samples, uint32_t sampleRate = 16000, uint16_t channels = 1);

// Reads a WAV file into 16kHz mono float32 samples (range -1.0 to 1.0) for whisper.cpp
bool loadWavToMono16k(const QString &filePath, std::vector<float> &outPcmF32, uint32_t &outDurationMs);

// Resample floating point audio buffer from inRate to outRate (linear interpolation)
std::vector<float> resample(const std::vector<float> &input, uint32_t inRate, uint32_t outRate);

// Compute min and max peaks for visual waveform rendering (N buckets)
struct WaveformPeak {
    float minVal = 0.0f;
    float maxVal = 0.0f;
};
QVector<WaveformPeak> computeWaveformPeaks(const std::vector<float> &pcm, int numBuckets);

// Calculate RMS and Peak for VU meter from int16 PCM buffer
void calculateLevels(const int16_t *data, size_t count, float &peak, float &rms);

// Speech & Pronunciation Analysis Metrics
struct SpeechMetrics {
    float meanPitchHz = 0.0f;
    float minPitchHz = 0.0f;
    float maxPitchHz = 0.0f;
    float pitchVariation = 0.0f; // std deviation
    QString intonationTrend;     // "Rising ↗", "Falling ↘", "Dynamic 〰", "Steady →"
    float peakDbfs = -90.0f;
    float dynamicRangeDb = 0.0f;
    float speechRatioPercent = 0.0f;
    float pauseRatioPercent = 0.0f;
    int wordsPerMinute = 0;
    QString tempoRating;         // "Relaxed", "Natural Pace", "Brisk / Fast"
};

// Compute pitch track (F0 in Hz) using normalized autocorrelation with parabolic interpolation
std::vector<float> computePitchTrack(const std::vector<float> &pcm, int sampleRate = 16000, int hopSize = 160, int frameSize = 480);

// Compute frame energy track (0.0 to 1.0)
std::vector<float> computeEnergyTrack(const std::vector<float> &pcm, int hopSize = 160, int frameSize = 480);

// Analyze speech acoustic properties (tempo, pitch, intonation, vocal dynamics)
SpeechMetrics analyzeSpeech(const std::vector<float> &pcm, qint64 durationMs, int wordCount = 0);

} // namespace AudioUtils
