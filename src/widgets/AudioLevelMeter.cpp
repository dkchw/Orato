#include "AudioLevelMeter.h"
#include <QPainter>
#include <QLinearGradient>
#include <algorithm>

AudioLevelMeter::AudioLevelMeter(QWidget *parent)
    : QWidget(parent) {
    setFixedHeight(16);
    setMinimumWidth(80);

    connect(&m_decayTimer, &QTimer::timeout, this, &AudioLevelMeter::updateDecay);
    m_decayTimer.setInterval(30);
    m_decayTimer.start();
}

void AudioLevelMeter::setLevels(float peak, float rms) {
    m_peak = std::max(0.0f, std::min(1.0f, peak));
    m_rms = std::max(0.0f, std::min(1.0f, rms));

    if (m_peak > m_displayPeak) {
        m_displayPeak = m_peak;
    }
    if (m_rms > m_displayRms) {
        m_displayRms = m_rms;
    }
    update();
}

void AudioLevelMeter::reset() {
    m_peak = 0.0f;
    m_rms = 0.0f;
    m_displayPeak = 0.0f;
    m_displayRms = 0.0f;
    update();
}

void AudioLevelMeter::updateDecay() {
    bool needUpdate = false;
    if (m_displayRms > 0.001f) {
        m_displayRms *= 0.85f;
        needUpdate = true;
    } else {
        m_displayRms = 0.0f;
    }

    if (m_displayPeak > 0.001f) {
        m_displayPeak -= 0.02f;
        if (m_displayPeak < 0.0f) m_displayPeak = 0.0f;
        needUpdate = true;
    }

    if (needUpdate) {
        update();
    }
}

void AudioLevelMeter::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();

    // Background track
    p.fillRect(0, 0, w, h, QColor(30, 30, 35));

    // Gradient bar for RMS
    int rmsWidth = static_cast<int>(m_displayRms * w);
    if (rmsWidth > 0) {
        QLinearGradient grad(0, 0, w, 0);
        grad.setColorAt(0.0, QColor(46, 204, 113));  // green
        grad.setColorAt(0.7, QColor(241, 196, 15));  // yellow
        grad.setColorAt(0.9, QColor(231, 76, 60));   // red

        p.fillRect(0, 1, rmsWidth, h - 2, grad);
    }

    // Peak needle
    int peakX = static_cast<int>(m_displayPeak * (w - 2));
    if (peakX > 0) {
        p.setPen(QPen(QColor(255, 255, 255, 220), 2));
        p.drawLine(peakX, 0, peakX, h);
    }

    // Border
    p.setPen(QColor(60, 60, 70));
    p.drawRect(0, 0, w - 1, h - 1);
}
