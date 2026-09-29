# Installation & Usage Guide 🛠️

This guide explains how to install, build, and run **Orato** on your NixOS system using flakes, as well as how to clean up build artifacts.

---

## 1. System Installation on NixOS

### Option A: Declarative via NixOS Configuration (`configuration.nix` with Flakes)
If your NixOS system is managed with a flake, add `orato` as an input:

```nix
# /etc/nixos/flake.nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    orato.url = "github:dkchw/Orato";
  };

  outputs = { self, nixpkgs, orato, ... }: {
    nixosConfigurations.myhostname = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        ./configuration.nix
        {
          environment.systemPackages = [
            orato.packages.x86_64-linux.default
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
    inputs.orato.packages.${pkgs.system}.default
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
nix profile install github:dkchw/Orato

# Or install from this local directory
nix profile install .
```

To update later:
```bash
nix profile upgrade orato
```

To uninstall:
```bash
nix profile remove orato
```

---

### Option D: Run Instantly without Installing (`nix run`)
```bash
# Run directly from GitHub
nix run github:dkchw/Orato

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
./build/orato
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

To make Orato appear in your desktop application launcher (Rofi, Wofi, GNOME, KDE, etc.):

```bash
mkdir -p ~/.local/share/applications
cp orato.desktop ~/.local/share/applications/
```
If using `nix profile install` or NixOS system packages, the desktop entry is installed automatically.
