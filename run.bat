@echo off
for /f "delims=" %%i in ('wsl wslpath "%~dp0"') do set WSL_PATH=%%i
wsl -e bash -c "cd '%WSL_PATH%' && (cd build && ./main) || (cmake -B build && cmake --build build && (cd build && make release) && cd build && ./main)"
pause
