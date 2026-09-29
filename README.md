# Recorder 🎙️

A high-performance Qt6 C++ Audio Recording & Speech Training Studio for Linux, built with **Whisper.cpp**, **Kyutai Pocket TTS**, **Interactive Waveform Timeline**, and **Real-Time Markdown Notes**.

Designed specifically for speech shadowing, language learning, pronunciation training, and audio session note-taking. Targets NixOS via `flake.nix` with `nix develop`.

---

## ✨ Features

- 🎙️ **Audio Recording & Input Source Selection**:
  - Dynamically discover and choose any system audio input device (microphones, USB headsets, line-in).
  - High-quality 16 kHz 16-bit mono PCM capture (the native format required by Whisper).
  - Live VU audio level meter with RMS and peak needle indicators.
  - Recording duration timer with pause and resume support.

- 🧠 **Whisper.cpp Speech-to-Text Integration**:
  - Offline local inference running in background threads without blocking the UI.
  - Precise timestamped segmentation: generates start/end milliseconds for every sentence.
  - Model selection including:
    - **`primeline/whisper-tiny-german`**: fine-tuned German model (GGML Q8_0 & F16 quants pre-configured).
    - Official OpenAI Whisper models: `tiny`, `tiny.en`, `base`, `base.en`, `small`, `small.en`, `medium`.
    - Custom model browser (`.bin` or `.gguf` files).
    - HuggingFace model converter helper (`scripts/convert_huggingface_whisper.py`).
  - Built-in 1-click downloader for preset models with progress bar.

- 🔁 **"Hear Again" & Shadowing Sentence Replay**:
  - Each transcribed sentence appears as an interactive card.
  - Instant **▶ Play** button plays the exact audio snippet `[start_ms, end_ms]` from your recording.
  - **🔁 Loop Sentence** mode repeats the sentence indefinitely for speech shadowing.
  - Keyboard shortcut: press <kbd>R</kbd> to replay the current sentence at any time.

- 🔊 **Kyutai Pocket TTS (Text-to-Speech)**:
  - Integrated local CPU-based neural TTS powered by Kyutai's Pocket TTS (`pocket-tts`).
  - Generate and compare native speech pronunciation side-by-side with your recording.
  - Supported voices:
    - 🇩🇪 German: `Jürgen` (`juergen`)
    - 🇬🇧 English: `Alba` (`alba`)
    - 🇫🇷 French: `Estelle` (`estelle`)
    - 🇪🇸 Spanish: `Lola` (`lola`)
    - 🇮🇹 Italian: `Giovanni` (`giovanni`)
    - 🇵🇹 Portuguese: `Rafael` (`rafael`)
  - Dedicated **Pocket TTS Studio** tab to type and synthesize custom text.
  - Caches generated TTS audio clips in the session for instant replay.

- 🌊 **Real-Time Visual Waveform Timeline**:
  - High-resolution audio waveform visualizer (`WaveformWidget`).
  - Upper time ruler with second/minute tick marks.
  - Visual overlay bounding boxes for all transcribed sentences with badge numbers (`#1`, `#2`...).
  - Real-time playhead cursor with timestamp badge tracking playback.
  - Interactive scrubbing: click or drag anywhere on the timeline to seek.
  - Double-click on any sentence region on the timeline to play that sentence immediately.
  - Zoom controls: **Zoom In (+)**, **Zoom Out (-)**, and **Fit to Window**.

- 📓 **Session Notes with Real-Time Markdown Rendering**:
  - Split-pane layout: monospace Markdown Editor on the left, formatted preview on the right.
  - Real-time rendering as you type.
  - Formatting toolbar: Headings (H1/H2/H3), Bold, Italic, Bullet Lists, Task Checkboxes, Quotes, Code blocks.
  - **Interactive Timestamps**: Click the **⏱ Timestamp** button or **📝 Note** button on any sentence to insert `[mm:ss.t](time://ms)`.
  - Clicking any timestamp link in the preview automatically seeks and plays the audio at that exact second!

- ⏯️ **Full-Featured Playback Control**:
  - Play / Pause (<kbd>Space</kbd>), Stop, Seek Slider.
  - Rewind 5 seconds (<kbd>⏪ -5s</kbd>) and Fast-Forward 5 seconds (<kbd>+5s ⏩</kbd>).
  - Variable playback speed: `0.5x`, `0.75x`, `1.0x`, `1.25x`, `1.5x`, `2.0x`.
  - Volume slider and mute toggle.

- 💾 **Training Session Management**:
  - Sidebar listing all saved sessions with title, date, duration, and sentence count.
  - Auto-persists audio WAV, JSON metadata, transcribed segments, Markdown notes, and TTS audio into `~/.local/share/recorder/sessions/<session_id>/`.
  - Create new sessions, rename, load previous training days, or delete sessions.

---

## 🚀 Quick Start with NixOS (`nix develop`)

### 1. Enter the development environment
```bash
nix develop
```

This sets up:
- Qt 6 (Base, Multimedia, Network, Widgets)
- CMake & Ninja
- ALSA & PulseAudio development libraries
- Python 3 & `uv` for Pocket TTS
- FFmpeg tools

### 2. Build the application
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 3. Run
```bash
./build/recorder
```

Or build directly via Nix:
```bash
nix build
./result/bin/recorder
```

---

## 📥 Whisper Models Setup

You can download Whisper models directly from within the app under the **🧠 Whisper Models** tab with a single click, or from the terminal:

```bash
# Download fine-tuned German model (primeline / Pomni GGML Q8_0)
./scripts/download_models.sh tiny-german

# Download English tiny model
./scripts/download_models.sh tiny.en

# Download multilingual base model
./scripts/download_models.sh base
```

Models are saved in `~/.local/share/recorder/models/`.

### Converting Hugging Face Models
To convert any Hugging Face Whisper model (including `primeline/whisper-tiny-german` from raw safetensors/PyTorch weights):
```bash
./scripts/convert_huggingface_whisper.py primeline/whisper-tiny-german
```

---

## 📁 Project Structure

```
Recorder/
├── CMakeLists.txt                 # CMake build configuration (Qt6 + Whisper.cpp)
├── flake.nix                      # NixOS flake for nix develop and nix build
├── flake.lock                     # Flake dependencies lockfile
├── README.md                      # Documentation
├── scripts/
│   ├── download_models.sh         # CLI script to download Whisper GGML models
│   └── convert_huggingface_whisper.py # Hugging Face to GGML conversion helper
├── pocket-tts/                    # Kyutai Pocket TTS Python engine
├── third_party/
│   └── whisper.cpp/               # Vendored Whisper.cpp engine
└── src/
    ├── main.cpp                   # Application entry point
    ├── MainWindow.h / .cpp        # Studio central window coordinating all modules
    ├── models/
    │   └── SessionData.h          # Data structures for Session and Segments
    ├── audio/
    │   ├── AudioRecorder.h / .cpp # Qt6 Audio capture with 16k conversion & VU meter
    │   ├── AudioPlayer.h / .cpp   # Audio playback, speed control, segment replay
    │   └── AudioUtils.h / .cpp    # WAV I/O, resampling, waveform peak computation
    ├── whisper/
    │   ├── WhisperEngine.h / .cpp # Background threaded Whisper.cpp transcription
    │   └── ModelManager.h / .cpp  # Model catalog, download manager, HF converter
    ├── tts/
    │   └── PocketTTSEngine.h / .cpp # Local Kyutai Pocket TTS runner
    ├── session/
    │   └── SessionManager.h / .cpp # Session folder persistence (JSON, WAV, MD, TTS)
    └── widgets/
        ├── AudioLevelMeter.h / .cpp  # Real-time VU meter
        ├── WaveformWidget.h / .cpp   # Interactive waveform timeline & playhead
        ├── SentenceListView.h / .cpp # Interactive cards with Play/Loop/TTS/Note
        └── MarkdownNoteEditor.h / .cpp # Split Markdown editor with real-time preview
```

---

## ⌨️ Keybindings

| Key | Action |
|:---|:---|
| <kbd>Space</kbd> | Toggle Play / Pause |
| <kbd>R</kbd> | Replay current sentence |
| <kbd>Ctrl</kbd> + <kbd>Wheel</kbd> | Zoom In / Out on Waveform Timeline |
