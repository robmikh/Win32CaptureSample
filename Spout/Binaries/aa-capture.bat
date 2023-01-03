@echo off
rem
rem Example of command line for window or minotor capture
rem
rem For a window, enter the name shown on the title bar
rem Run the window application before this batch file
rem
rem For a monitor, enter the index of the monitor, 0, 1, 2 etc
rem The index is ignored if greater than the number of monitors
rem
rem For example :
rem
rem For a window :
rem "Win32CaptureSample" "Spout Demo Sender"
rem
rem For a monitor :
"Win32CaptureSample" "0"
rem
rem This batch file can be run without showing a console by "aa-start.vbs"
rem
rem Win32CaptureSample and command line can also be activated
rem from a program using ShellExecute of similar
rem

