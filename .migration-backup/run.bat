@echo off
REM Spustenie hry Operacia Kopanice
cd /d "%~dp0"
python -m pip install -r requirements.txt
python play.py
