## Ongoing Tic-Tac-Toe

A desktop tic-tac-toe game with configurable board size and win condition.

### Features
- Player vs Player and Player vs Bot modes
- Configurable board dimension and target in-a-row
- Adjustable bot difficulty
- Background music and sound effects

### Requirements

- Windows with [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install)
- Inside WSL, install dependencies:
  ```bash
  sudo apt update
  sudo apt install cmake g++ libglfw3-dev libgl1-mesa-dev git
  ```
- For audio, install [PipeWire](https://pipewire.org/) (if not already present):
  ```bash
  sudo apt install pipewire pipewire-pulse wireplumber
  systemctl --user start pipewire pipewire-pulse wireplumber
  ```

### Running

Double-click `run.bat`. On first launch it will build the project automatically, then start the game.
