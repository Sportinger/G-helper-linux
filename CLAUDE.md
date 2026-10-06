# Claude Instructions

## App starten

```bash
pkill -x g-helper-linux 2>/dev/null; sleep 0.5; env QT_QPA_PLATFORM=xcb "$(git rev-parse --show-toplevel)"/build/g-helper-linux 2>&1 &
sleep 2
pgrep -x g-helper-linux && echo "App läuft"
```

Wichtig: Immer alte Prozesse killen, 2 Sekunden warten, und dann prüfen ob App läuft.
`-x` (exakter Prozessname) statt `-f`, sonst trifft pkill auch die eigene Shell, deren Kommandozeile den Projektpfad enthält.

## Bauen

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

Benötigt asusd (asusctl ≥ 6.4) auf dem System; Quellcode liegt in `~/src/asusctl`.

## Screenshot (ohne Display)

```bash
GHELPER_SCREENSHOT=/pfad/shot.png QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/g-helper-linux
```

Rendert das Hauptfenster nach 3 s in eine PNG-Datei und beendet sich.

## Git Workflow

Nach jeder Änderung am Code immer committen und pushen:

```bash
git add <geänderte-dateien>
git commit -m "Beschreibung der Änderung"
git push
```
