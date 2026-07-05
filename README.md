# SREES_2026_Erović_SCC

**Proračun kratkih spojeva za transmisione mreže (simetrične mreže)**
Short-circuit calculation for symmetrical (balanced three-phase) transmission networks.

Seminar project — SREES 2026, Ilhan Erović (ETF Sarajevo).

## Šta radi

Standalone GUI aplikacija napravljena u **natID** frameworku. Učitava MATPOWER `.m`
opis mreže, gradi matricu admitansi **Y_bus** (rijetka kompleksna matrica iz natID-a),
i proračunava simetrični (tropolni) kratki spoj **Z-bus** metodom: rješavanjem
`Y_bus · z = e_k` dobija se k-ta kolona Z_busa, pa struja kvara `I_f = V_k(0)/Z_kk`
i naponi svih sabirnica poslije kvara. Prednaponi: flat start 1.0∠0° pu.

Sve matrice koriste obavezne natID klase: **sparse** (Y_bus) i **dense** (vektori).

## Struktura repozitorija

- `src/` — CMake konfiguracija i C++ izvorni kod
  - `src/CMakeLists.txt`, `src/shortCircuit.cmake` — build
  - `src/src/` — C++ kod (GUI + logika)
  - `src/res/` — resursi (ikone, prijevodi EN/BA)
- `docs/` — izvještaji i dokumentacija (Lyx)
- `presentation/` — finalna prezentacija + ReadMe.txt

## Build

Preduslov: natID SDK u `~/natID.SDK` i C++ toolchain (Windows: Visual Studio / MSVC;
Linux: gcc). Konfiguracija je u CMake i radi na Windowsu i Linuxu.

```
cmake -S src -B build
cmake --build build --config Debug
```

Izlazni artefakti idu u `~/natID.RAMDisk/Out/SREES_2026_Erovic_SCC/`.
Na Windowsu runtime treba `~/natID.SDK/bin` (i `bin/GTK`) na PATH-u.
