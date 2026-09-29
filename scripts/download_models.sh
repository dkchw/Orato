#!/usr/bin/env bash
# ==============================================================================
# Model Downloader for Recorder
# Supports Whisper GGML models and primeline/whisper-tiny-german
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/recorder/models"
mkdir -p "$MODELS_DIR"

echo "Models destination: $MODELS_DIR"

MODEL="${1:-tiny-german}"

case "$MODEL" in
    "tiny-german"|"whisper-tiny-german")
        echo "Downloading primeline/whisper-tiny-german (GGML Q8_0 quant by Pomni)..."
        OUT_FILE="$MODELS_DIR/ggml-tiny-german-q8_0.bin"
        if [ -f "$OUT_FILE" ]; then
            echo "Model already exists: $OUT_FILE"
        else
            curl -L -C - --progress-bar -o "$OUT_FILE" \
                "https://huggingface.co/Pomni/whisper-tiny-german-ggml-allquants/resolve/main/ggml-tiny-german-q8_0.bin"
            echo "Successfully downloaded to $OUT_FILE"
        fi
        ;;
    "tiny-german-f16")
        echo "Downloading primeline/whisper-tiny-german (GGML F16 quant)..."
        OUT_FILE="$MODELS_DIR/ggml-tiny-german-f16.bin"
        if [ -f "$OUT_FILE" ]; then
            echo "Model already exists: $OUT_FILE"
        else
            curl -L -C - --progress-bar -o "$OUT_FILE" \
                "https://huggingface.co/Pomni/whisper-tiny-german-ggml-allquants/resolve/main/ggml-tiny-german-f16.bin"
            echo "Successfully downloaded to $OUT_FILE"
        fi
        ;;
    "tiny.en"|"tiny"|"base"|"base.en"|"small"|"small.en"|"medium"|"medium.en")
        echo "Downloading Whisper official GGML model: $MODEL..."
        OUT_FILE="$MODELS_DIR/ggml-$MODEL.bin"
        if [ -f "$OUT_FILE" ]; then
            echo "Model already exists: $OUT_FILE"
        else
            curl -L -C - --progress-bar -o "$OUT_FILE" \
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-$MODEL.bin"
            echo "Successfully downloaded to $OUT_FILE"
        fi
        ;;
    *)
        echo "Unknown model: $MODEL"
        echo "Available options: tiny-german, tiny-german-f16, tiny, tiny.en, base, base.en, small, small.en, medium"
        exit 1
        ;;
esac
