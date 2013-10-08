
set IPHYGUIPATH=..\gui

set PYTHONPATH=%PYTHONPATH%;%IPHYGUIPATH%;%IPHYGUIPATH%\lib\utils;%IPHYGUIPATH%\lib\devices

@echo "Running gui..."
rem start /low /b /wait
C:\Python26\python.exe %IPHYGUIPATH%\ipallreg.py

pause "Press any key to remove this window..."
