COMMAND LINE FOR WINDOW OR DISPLAY CAPTURE

Edit the file "aa-capture.bat" to enter the window or display to capture

For a window, enter the name shown on the title bar
Run the window application before aa-capture.bat or aa-start.vbs

For a display, enter the index, 0, 1, 2 etc
Thee index is ignored if greater than the number of displays

For example :

For a window :
"Win32CaptureSample" "Spout Demo Sender"

For a display :
"Win32CaptureSample" "0"

"aa-capture.bat" can be run directly and will show a console window.
If you don't want a console window to show, run "aa-start.vbs".
Win32ApplicationSample.exe will start and appear minimized on the taskbar.
You will know it's working by opening a Spout receiver.

Win32CaptureSample can also be activated by command line
from a program using ShellExecute of similar


WIN32CAPTURESAMPLE ICON

It's difficult to assign a suitable icon, so the program
is made so that you can change it to anything you like.

Rename the icon you want to use to "Win32CaptureSample.ico".
Several are included for you to choose from.
The default "pacman" is just for fun.

