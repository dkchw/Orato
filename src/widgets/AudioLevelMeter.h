#pragma once

#include <QWidget>
#include <QTimer>

class AudioLevelMeter : public QWidget {
    Q_OBJECT

public:
    explicit AudioLevelMeter(QWidget *parent = nullptr);
    ~AudioLevelMeter() override = default;

public slots:
    void setLevels(float peak, float rms);
    void reset();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateDecay();

private:
    float m_peak = 0.0f;
    float m_rms = 0.0f;
    float m_displayPeak = 0.0f;
    float m_displayRms = 0.0f;
    QTimer m_decayTimer;
};
