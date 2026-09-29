# Installation & Usage Guide 🛠️

This guide explains how to install, build, and run **Recorder** on your NixOS system using flakes, as well as how to clean up build artifacts.

---

## 1. System Installation on NixOS

### Option A: Declarative via NixOS Configuration (`configuration.nix` with Flakes)
If your NixOS system is managed with a flake, add `Recorder` as an input:

```nix
# /etc/nixos/flake.nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    recorder.url = "github:dkchw/Recorder";
  };

  outputs = { self, nixpkgs, recorder, ... }: {
    nixosConfigurations.myhostname = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        ./configuration.nix
        {
          environment.systemPackages = [
            recorder.packages.x86_64-linux.default
          ];
        }
      ];
    };
  };
}
```
Then rebuild:
```bash
sudo nixos-rebuild switch --flake /etc/nixos#myhostname
```

---

### Option B: Via Home Manager
Add to your `home.nix`:
```nix
{ pkgs, inputs, ... }: {
  home.packages = [
    inputs.recorder.packages.${pkgs.system}.default
  ];
}
```
Then switch:
```bash
home-manager switch --flake ~/.config/home-manager#username
```

---

### Option C: Imperative Installation (`nix profile`)
To install directly into your user environment without modifying system configuration:

```bash
# Install directly from GitHub
nix profile install github:dkchw/Recorder

# Or install from this local directory
nix profile install .
```

To update later:
```bash
nix profile upgrade recorder
```

To uninstall:
```bash
nix profile remove recorder
```

---

### Option D: Run Instantly without Installing (`nix run`)
```bash
# Run directly from GitHub
nix run github:dkchw/Recorder

# Or run from local repo
nix run .
```

---

## 2. Development Setup (`nix develop`)

To develop or modify the source code:

```bash
# 1. Enter development environment with all Qt6, CMake, and audio dependencies
nix develop

# 2. Configure & build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 3. Launch application
./build/recorder
```

---

## 3. Cleaning Up Build Artifacts

To remove all CMake compilation objects, Nix store symlinks, and temporary cache files:

```bash
./scripts/cleanup.sh
```

Or manually:
```bash
rm -rf build result .cache
```

---

## 4. Desktop Integration

To make Recorder appear in your desktop application launcher (Rofi, Wofi, GNOME, KDE, etc.):

```bash
mkdir -p ~/.local/share/applications
cp recorder.desktop ~/.local/share/applications/
```
If using `nix profile install` or NixOS system packages, the desktop entry is installed automatically.
