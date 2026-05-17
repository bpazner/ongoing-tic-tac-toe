## Ongoing Tic-Tac-Toe

A desktop tic-tac-toe game with configurable board size and win condition.

### Features
- Player vs Player and Player vs Bot modes
- Configurable board dimension and target in-a-row
- Adjustable bot difficulty
- Background music and sound effects

### Running

Download the latest release and run the executable.

### Building from Source

**Requirements**

- [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install)
- Inside WSL, install dependencies:
  ```bash
  sudo apt update
  sudo apt install cmake g++ libglfw3-dev libgl1-mesa-dev git
  ```
- If audio is not working, install [PipeWire](https://pipewire.org/):
  ```bash
  sudo apt install pipewire pipewire-pulse wireplumber
  systemctl --user start pipewire pipewire-pulse wireplumber
  ```

**Build (Linux/WSLg)**

```bash
cmake -B build
cd build && make release
./main
```

**Build (Windows .exe via MinGW)**

Install the cross-compiler and set it to use POSIX threads:
```bash
sudo apt install mingw-w64
sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
```

Then build:
```bash
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw.cmake
cd build-win && make ott
```

The output is `build-win/ott.exe`. To run it, copy `ott.exe` and the `assets/` folder to the same directory on Windows.

### Troubleshooting

- **(Linux/WSL) Window doesn't open or appears in the taskbar but can't be focused** — WSLg has gotten into a bad state. Run the following in PowerShell, then try again:
  ```powershell
  wsl --shutdown
  ```
