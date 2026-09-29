#!/usr/bin/env bash
""":"
# Python wrapper that runs with pocket-tts venv if available
VENV_PY="$(dirname "$0")/../pocket-tts/.venv/bin/python"
if [ -x "$VENV_PY" ]; then
    exec "$VENV_PY" "$0" "$@"
else
    exec python3 "$0" "$@"
fi
"""
import sys
import os
from pathlib import Path

def main():
    if len(sys.argv) < 2:
        print("Usage: convert_huggingface_whisper.py <model-id-or-dir> [output-dir] [whisper-dir]")
        print("Example: convert_huggingface_whisper.py primeline/whisper-tiny-german")
        sys.exit(1)

    model_id = sys.argv[1]
    out_dir = Path(sys.argv[2] if len(sys.argv) > 2 else os.path.expanduser("~/.local/share/recorder/models"))
    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"Loading Hugging Face model: {model_id}...")
    from transformers import WhisperForConditionalGeneration, WhisperTokenizer
    import torch

    try:
        model = WhisperForConditionalGeneration.from_pretrained(model_id)
        tokenizer = WhisperTokenizer.from_pretrained(model_id)
        print("Model loaded successfully!")
    except Exception as e:
        print(f"Error loading model: {e}")
        sys.exit(1)

    # Whisper.cpp conversion
    converter_script = Path(__file__).parent.parent / "third_party" / "whisper.cpp" / "models" / "convert-h5-to-ggml.py"
    if not converter_script.exists():
        print(f"Error: converter script not found at {converter_script}")
        sys.exit(1)

    # Save to temporary directory first
    import tempfile
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_path = Path(tmp_dir)
        print(f"Saving HF model to temporary directory {tmp_path}...")
        model.save_pretrained(tmp_path)
        tokenizer.save_pretrained(tmp_path)

        whisper_dir = Path(sys.argv[3]) if len(sys.argv) > 3 else Path(__file__).parent.parent / "third_party" / "whisper.cpp"
        cmd = f'python3 "{converter_script}" "{tmp_path}" "{whisper_dir}" "{out_dir}"'
        print(f"Running conversion: {cmd}")
        ret = os.system(cmd)
        if ret == 0:
            print(f"Conversion complete! Model saved in: {out_dir}")
        else:
            print(f"Conversion exited with code {ret}")

if __name__ == "__main__":
    main()
