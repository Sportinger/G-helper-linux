# G-Helper Linux

A lightweight Qt/QML control tool for ASUS ROG laptops on Linux - inspired by [G-Helper](https://github.com/seerge/g-helper) for Windows.

![Status](https://img.shields.io/badge/Status-In%20Development-yellow)
![License](https://img.shields.io/badge/License-GPL--3.0-blue)

## Features (Planned)

- **Performance Modes**: Silent, Balanced, Turbo, Manual
- **GPU Control**: Integrated, Hybrid, Dedicated switching via supergfxctl
- **Fan Curves**: Custom fan curve editor with visual graph
- **Keyboard RGB**: Aura control for backlight colors and effects
- **Battery Management**: Charge limit settings for battery longevity
- **System Tray**: Quick access to all settings

## Tech Stack

- **Frontend**: Qt 6 / QML
- **Backend**: C++ with D-Bus integration
- **Dependencies**:
  - `asusctl` - ASUS laptop control daemon
  - `supergfxctl` - GPU switching

## Requirements

- Arch Linux (or other distros with asusctl)
- Qt 6.x
- asusctl & supergfxctl installed

## Building

```bash
# Install dependencies (Arch)
sudo pacman -S qt6-base qt6-declarative cmake

# Build
mkdir build && cd build
cmake ..
make

# Run
./g-helper-linux
```

## Screenshots

Reference screenshots from the original G-Helper (Windows):

See `docs/reference-screenshots/` for UI inspiration.

## Contributing

Contributions welcome! This project aims to bring the G-Helper experience to Linux users.

## Credits

- Original [G-Helper](https://github.com/seerge/g-helper) by seerge
- [asusctl](https://gitlab.com/asus-linux/asusctl) by asus-linux team

## License

GPL-3.0
