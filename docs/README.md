# docs — dokumentacija

Sadržaj:

- **`report.tex`** — seminarski izvještaj (LaTeX izvor): teorija, metoda,
  implementacija, rezultati, literatura.
- **`figures/`** — slike koje izvještaj koristi (dijagram toka, GUI, grafici,
  jednopolna shema, generisani izvještaj).
- **`diagrams/`** — izvorni vektorski dijagrami u SVG (dijagram toka i jednopolna
  shema), za uređivanje u **OpenDraw / LibreOffice Draw** (vidi `diagrams/README.txt`).

## Kako otvoriti izvještaj u LyX-u

1. Instaliraj **LyX** i uz njega TeX distribuciju (npr. **MiKTeX** na Windowsu).
2. U LyX-u: **File → Import → LaTeX (plain)** i odaberi `report.tex`.
   LyX ga pretvori u `.lyx` dokument.
3. Kompajluj u PDF: **View → View [PDF (pdflatex)]** ili **Document → View**.

> Napomena: izvještaj koristi standardne pakete (`babel` [croatian],
> `siunitx`, `booktabs`, `graphicx`, `amsmath`, `hyperref`), koji su dio pune
> MiKTeX/TeX Live instalacije. Slike se učitavaju iz `figures/`
> (`\graphicspath{{figures/}}`), pa `report.tex` treba kompajlovati iz `docs/`.

## Alternativa (bez LyX-a)

Ako je instalirana samo TeX distribucija, izvještaj se može kompajlovati i
direktno iz `docs/`:

```
pdflatex report.tex
pdflatex report.tex   # drugi prolaz zbog referenci
```
