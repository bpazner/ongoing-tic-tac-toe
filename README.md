## Ongoing Tic-Tac-Toe

A desktop tic-tac-toe game with configurable board size and win condition.

### Features
- Player vs Player and Player vs Bot modes
- Configurable board dimension and target in-a-row
- Adjustable bot difficulty
- Background music and sound effects

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
