Set oShell = CreateObject ("Wscript.Shell") 
Dim strArgs
strArgs = "cmd /c aa-sender.bat"
oShell.Run strArgs, 0, false


