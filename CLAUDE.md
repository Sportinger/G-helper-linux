# Claude Instructions

## App starten

```bash
pkill -9 -f g-helper-linux 2>/dev/null; sleep 0.5; env QT_QPA_PLATFORM=xcb /home/admin/Dokumente/G-helper-linux/build/g-helper-linux 2>&1 &
sleep 2
pgrep -f g-helper-linux && echo "App läuft"
```

Wichtig: Immer alte Prozesse killen, 2 Sekunden warten, und dann prüfen ob App läuft.

## Git Workflow

Nach jeder Änderung am Code immer committen und pushen:

```bash
git add <geänderte-dateien>
git commit -m "Beschreibung der Änderung"
git push
```
