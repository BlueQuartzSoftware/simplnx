# WritePoleFigure — V&V A/B evidence (regenerated 2026-07-08)

Class-5 (expert-visual) A/B comparison for the `nx::core::WritePoleFigureFilter` (#1647). This folder
is a **full fresh regeneration** — the original working folder's legacy-input minting script and legacy
renders were unrecoverable (see `../../../vv_work/write_pole_figure/RECOVERY_STATUS.md`), so the entire
set was rebuilt here from first principles and re-run through all three engines.

> **What is committed here:** only the text artifacts — the three generator scripts, the four pipeline
> files, and this README. The **binary evidence** (input `.dream3d` files and the rendered `.pdf`/`.png`
> pole figures) is **archived to OneDrive**, not committed to the repo (see `.gitignore`). Everything is
> reproducible from the committed scripts via the steps below.

## What is compared

The same 502 orientations are rendered by three engines, in two modes, for two Laue groups:

| Engine | Runner | Output |
|--------|--------|--------|
| **DREAM3D 6.5.171** | `~/Applications/DREAM3D.app` PipelineRunner | PDF |
| **DREAM3D 6.5.172** | `Workspace3/6.5.172` PipelineRunner | PDF |
| **DREAM3D-NX** | `nxrunner` built from `topic/write_pole_figure_fixes` (#1647) | PNG |

- **Modes:** Color (`GenerationAlgorithm 0`) and Discrete (`GenerationAlgorithm 1`).
- **Laue groups:** hex (Ti, `CrystalStructures=[999,0]`) and cubic (`[999,1]`, same Eulers relabeled).

## Layout

```
input/
  pf_input_hex.dream3d          NX-format input, hex   (502 orientations)
  pf_input_cubic.dream3d        NX-format input, cubic
  pf_input_legacy.dream3d       SIMPL v7.0 input, hex   (minted by build_legacy_input.py)
  pf_input_cubic_legacy.dream3d SIMPL v7.0 input, cubic
out_nx/                 nx_color_1.png, nx_discrete_1.png            (NX, hex)
out_cubic_nx/           nx_color_1.png, nx_discrete_1.png            (NX, cubic)
out_legacy_6_5_171/     color_Phase_1.pdf, discrete_Phase_1.pdf      (6.5.171, hex)
out_legacy_6_5_172/     color_Phase_1.pdf, discrete_Phase_1.pdf      (6.5.172, hex)
out_cubic_legacy_6_5_171/  color_Phase_1.pdf, discrete_Phase_1.pdf   (6.5.171, cubic)
out_cubic_legacy_6_5_172/  color_Phase_1.pdf, discrete_Phase_1.pdf   (6.5.172, cubic)
```

## Reproduce

```bash
# 1. NX inputs are committed. Mint the legacy inputs from them:
/usr/bin/python3 build_legacy_input.py hex   input/pf_input_legacy.dream3d
/usr/bin/python3 build_legacy_input.py cubic input/pf_input_cubic_legacy.dream3d

# 2. NX renders (nxrunner built from #1647):
/usr/bin/python3 gen_nx_pf.py hex   && nxrunner --execute pf_nx_hex.d3dpipeline
/usr/bin/python3 gen_nx_pf.py cubic && nxrunner --execute pf_nx_cubic.d3dpipeline

# 3. Legacy renders (repeat for 172 runner and cubic variant):
/usr/bin/python3 gen_legacy_pf.py hex "$PWD/out_legacy_6_5_171" "$PWD/pf_legacy_171_hex.json"
"$DREAM3D_651/PipelineRunner" -p pf_legacy_171_hex.json
```

Scripts (all committed here): `build_legacy_input.py`, `gen_nx_pf.py`, `gen_legacy_pf.py`.

## Results

- **6.5.171 vs 6.5.172 are the same picture.** hex-color, hex-discrete, and cubic-discrete PDFs are
  **byte-identical**; cubic-color differs only in a trailing PDF metadata block (identical rendering).
  This confirms no pole-figure regression across the two legacy versions.
- **NX vs legacy** reproduces the expected pole-figure topology (pole positions, symmetry, color
  mapping) in both color and discrete modes, for both Laue groups. NX renders to PNG rather than PDF;
  compare visually.
- **Discrete mode** exercises the new `discrete_marker_radius` parameter fixed in #1647 (the develop
  `nxrunner` lacked it and warned `-5433`; the #1647 build honors it). NX discrete PNGs were produced by
  the branch-accurate build.

## Caveat (honesty note)

The legacy input was **reconstructed**, not recovered: `build_legacy_input.py` was rewritten from the
array contract in `gen_legacy_pf.py`'s reader proxy plus the SIMPL v7.0 HDF5 layout cloned from a
known-good legacy file. The 502 orientations themselves are the originals (extracted from the committed
`pf_input_*.dream3d`). Both legacy engines read the minted file without error and produced valid pole
figures, which validates the reconstruction. The trailing space in the phase name (`'Nickel '` in the
template; here `'Primary'`) matches the legacy convention noted in deviation D1.
