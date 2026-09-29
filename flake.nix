{
  description = "Recorder: Qt6 C++ Speech Training Studio with Whisper.cpp, Pocket TTS, Realtime Waveform Timeline, and Markdown Notes";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "sound-recorder";
          version = "1.0.0";
          src = pkgs.lib.cleanSourceWith {
            src = ./.;
            filter = path: type:
              let base = baseNameOf path; in
              !(base == "build" || base == ".venv" || base == ".cache" || base == "result");
          };

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.pkg-config
            pkgs.qt6.wrapQtAppsHook
          ];

          buildInputs = [
            pkgs.qt6.qtbase
            pkgs.qt6.qtmultimedia
            pkgs.qt6.qtsvg
            pkgs.alsa-lib
            pkgs.libpulseaudio
          ];

          cmakeFlags = [
            "-DWHISPER_BUILD_TESTS=OFF"
            "-DWHISPER_BUILD_EXAMPLES=OFF"
          ];

          meta = with pkgs.lib; {
            description = "Qt6 C++ Speech Recording & Training Studio with Whisper.cpp and Pocket TTS";
            license = licenses.mit;
            platforms = platforms.linux;
            mainProgram = "recorder";
          };
        };

        packages.recorder = self.packages.${system}.default;

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/recorder";
        };

        devShells.default = pkgs.mkShell {
          name = "recorder-dev-shell";

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.pkg-config
            pkgs.ninja
            pkgs.qt6.wrapQtAppsHook
          ];

          buildInputs = [
            pkgs.qt6.qtbase
            pkgs.qt6.qtmultimedia
            pkgs.qt6.qtsvg
            pkgs.alsa-lib
            pkgs.libpulseaudio
            pkgs.pipewire
            pkgs.uv
            pkgs.python3
            pkgs.ffmpeg
          ];

          shellHook = ''
            export QT_QPA_PLATFORM="wayland;xcb"
            export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins:${pkgs.qt6.qtmultimedia}/lib/qt-6/plugins"
            export QML2_IMPORT_PATH="${pkgs.qt6.qtbase}/lib/qt-6/qml:${pkgs.qt6.qtmultimedia}/lib/qt-6/qml"

            echo ""
            echo "  ╔═══════════════════════════════════════════════════════════════════════╗"
            echo "  ║        🎙️  Recorder — Qt6 C++ Speech & Pronunciation Studio           ║"
            echo "  ║      Whisper.cpp | Pocket TTS | Visual Timeline | Markdown Notes      ║"
            echo "  ╚═══════════════════════════════════════════════════════════════════════╝"
            echo ""
            echo "  Available tools: cmake, ninja, qt6, whisper.cpp, uv, python3"
            echo ""
            echo "  Build commands:"
            echo "    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release"
            echo "    cmake --build build -j\$(nproc)"
            echo "    ./build/bin/recorder"
            echo ""
          '';
        };
      }
    );
}
