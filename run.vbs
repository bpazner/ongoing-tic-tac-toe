Dim shell, fso, dir, wsl_path
Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
dir = fso.GetParentFolderName(WScript.ScriptFullName) & "\"

' Convert Windows path to WSL path
wsl_path = shell.Exec("wsl wslpath """ & dir & """").StdOut.ReadAll()
wsl_path = Trim(wsl_path)

' If build/main doesn't exist, run cmake with visible terminal
If Not fso.FileExists(dir & "build\main") Then
    shell.Run "wsl -e bash -c ""cd '" & wsl_path & "' && cmake -B build && cd build && make release""", 1, True
End If

' Launch game with hidden terminal
shell.Run "wsl -e bash -c ""cd '" & wsl_path & "build' && ./main""", 0, False
