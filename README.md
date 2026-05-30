## Ongoing Tic-Tac-Toe

A desktop tic-tac-toe game where play continues after scoring, rewarding repeated streaks.

### Features

**Gameplay**
  - 3 game modes: Local Multiplayer, Player vs Bot, Bot vs Bot
  - Configurable board size and in-a-row target
  - Choose to play as X, O, or random side
  - Persistent scores across rounds

**AI**
  - Adjustable difficulty (1–5)
  - Bot speed control in Bot vs Bot mode
  - Non-blocking: UI stays responsive while the bot thinks

**UI**
  - Animated tile highlight on winning streaks
  - Color-coded X (red) and O (green) throughout
  - Separate background music and SFX volume sliders
  - Scales to window size

### Running

Download the latest release and run the executable for your platform:
- **Windows**: `OngoingTicTacToe.exe`
- **Linux**: `OngoingTicTacToe`

### Building From Source (Linux)

**Requirements**

- On Windows, install [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install)
- Install dependencies:
  ```bash
  sudo apt update
  sudo apt install cmake g++ libglfw3-dev libgl1-mesa-dev git
  ```
- If audio is not working, install [PipeWire](https://pipewire.org/):
  ```bash
  sudo apt install pipewire pipewire-pulse wireplumber
  systemctl --user start pipewire pipewire-pulse wireplumber
  ```

**Build**

```bash
cmake -B build
cd build && make
./OngoingTicTacToe
```

For a debug build, use `make debug` instead.

### Troubleshooting

- **(Windows/WSL) Window doesn't open or appears in the taskbar but can't be focused** — WSLg has gotten into a bad state. Run the following in PowerShell, then try again:
  ```powershell
  wsl --shutdown
  ```
