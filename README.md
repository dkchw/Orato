# Orato 🎙️

A high-performance Qt6 C++ Audio Recording & Speech Training Studio for Linux, built with **Whisper.cpp**, **Kyutai Pocket TTS**, **Interactive Waveform Timeline**, and **Real-Time Markdown Notes**.

Designed specifically for speech shadowing, language learning, pronunciation training, and multi-paragraph audio session note-taking. Targets NixOS via `flake.nix` with `nix develop`.

---

## ✨ Features

- 🎙️ **Multi-Sentence & Multi-Paragraph Recording**:
  - **Fresh Take**: Record a new take (`Record` button).
  - **Append Mode**: Record additional sentences or paragraphs (`Append` button) to build multi-paragraph audio sessions incrementally.
  - **Retake**: Reset audio and start over cleanly (`Retake` button).
  - **Input Source Selection**: Choose any audio device (microphones, USB headsets, line-in).
  - **Live VU Meter**: Real-time signal level monitoring with RMS and peak needle indicators.

- 📐 **Flexible Workspace Layouts**:
  - **Stacked Split (Transcript Under Note)**: View your Markdown notes on top with the transcribed sentence cards right beneath them, separated by a resizable divider.
  - **Side-by-Side Split**: View notes on the left and transcript cards on the right.
  - **Tabs Mode**: Focus on one workspace at a time (Sentences, Notes, TTS Studio, Models).
  - Switch layouts instantly via the top layout buttons.

- 🗂️ **Session Management & Folder Access**:
  - Resizable left sidebar: Drag the divider to adjust sidebar width freely.
  - Toggle / Close sidebar anytime with the top bar toggle or <kbd>Ctrl+B</kbd>.
  - **Open Folder**: Click the folder button to open the active session folder directly in your system file manager (Dolphin, Nautilus, etc.).
  - Auto-persists audio WAV, JSON metadata, transcribed segments, notes, and cached TTS clips in `~/.local/share/orato/sessions/`.

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
  - Instant **Play** button plays the exact audio snippet `[start_ms, end_ms]` from your recording.
  - **Loop Sentence** mode repeats the sentence indefinitely for speech shadowing.
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
  - Zoom controls: **Zoom In**, **Zoom Out**, and **Fit to Window**.

- 📓 **Session Notes with Real-Time Markdown Rendering**:
  - Split-pane layout: monospace Markdown Editor on the left, formatted preview on the right.
  - Real-time rendering as you type.
  - Formatting toolbar: Headings (H1–H3), Bold, Italic, Bullet Lists, Task Checkboxes, Quotes, Code blocks.
  - **Interactive Timestamps**: Click the **Timestamp** button or **Note** button on any sentence to insert `[mm:ss.t](time://ms)`.
  - Clicking any timestamp link in the preview automatically seeks and plays the audio at that exact second!

- ⏯️ **Full-Featured Playback Control**:
  - Play / Pause (<kbd>Space</kbd>), Stop, Seek Slider.
  - Rewind 5 seconds and Fast-Forward 5 seconds.
  - Variable playback speed: `0.5x`, `0.75x`, `1.0x`, `1.25x`, `1.5x`, `2.0x`.
  - Volume slider and mute toggle.

---

## 🚀 Quick Install on NixOS via Flake

Install directly from GitHub into your user profile:
```bash
nix profile install github:dkchw/Orato
orato
```

Or run without installing:
```bash
nix run github:dkchw/Orato
```

See [INSTALL.md](INSTALL.md) for full system configuration and Home Manager setup instructions.

---

## 🛠️ Development Setup (`nix develop`)

```bash
# 1. Enter development environment
nix develop

# 2. Build the application
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 3. Run
./build/orato
```

### Clean Up Build Artifacts
```bash
./scripts/cleanup.sh
```

---

## 📥 Whisper Models Setup

You can download Whisper models directly from within the app under the **Whisper Models** tab with a single click, or from the terminal:

```bash
# Download fine-tuned German model (primeline / Pomni GGML Q8_0)
./scripts/download_models.sh tiny-german

# Download English tiny model
./scripts/download_models.sh tiny.en

# Download multilingual base model
./scripts/download_models.sh base
```

Models are saved in `~/.local/share/orato/models/`.

---

## ⌨️ Keybindings

| Key | Action |
|:---|:---|
| <kbd>Space</kbd> | Toggle Play / Pause |
| <kbd>R</kbd> | Replay current sentence |
| <kbd>Ctrl + B</kbd> | Toggle left sessions sidebar |
| <kbd>Ctrl + Wheel</kbd> | Zoom In / Out on Waveform Timeline |
