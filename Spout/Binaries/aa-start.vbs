Set oShell = CreateObject ("Wscript.Shell") 
Dim strArgs
strArgs = "cmd /c aa-capture.bat"
oShell.Run strArgs, 0, false


