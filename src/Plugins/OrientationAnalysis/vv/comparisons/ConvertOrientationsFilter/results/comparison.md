# Legacy Comparison: ConvertOrientationsFilter

Date: 2026-06-30

## Test Case
Single toy orientation (Euler `(0.7853982, 0.5235988, 1.0471976)` rad = 45°/30°/60°), converted from Euler to each of the 6 other **shared** representations (Orientation Matrix, Quaternion, Axis-Angle, Rodrigues, Homochoric, Cubochoric). Stereographic excluded — no 6.5.171 equivalent (deviation D3).

## Input Data
The legacy 6.5.171 pipeline (`pipelines/legacy_6_5_171.json`) builds the EulerAngles array from three constant scalars + `CombineAttributeArrays` and writes `results/legacy_out.dream3d`. The NX pipeline (`pipelines/nx.d3dpipeline`) **reads that same file** (NX reads legacy `.dream3d`), so the Euler input is byte-identical (verified: `np.array_equal == True`).

## Runners
- Legacy: `/Users/mjackson/Workspace9/6.5.172/DREAM3D-Build/D3D-Rel-Qt515-6_5_171/Bin/PipelineRunner` (PipelineRunner 1.2.832, official 6.5.171)
- NX: `/Users/mjackson/Workspace9/DREAM3D-Build/NX-Com-Qt69-Vtk96-Rel/Bin/nxrunner` (1.7.0), EbsdLib 3.0.0

## Results — max |Δ| (legacy vs NX), per conversion

| eu → | max \|Δ\| |
|---|---|
| Quaternion | 0 (bit-identical) |
| Axis-Angle | 0 (bit-identical) |
| Rodrigues | 0 (bit-identical) |
| Homochoric | 0 (bit-identical) |
| Orientation Matrix | 1.49e-08 |
| Cubochoric | 1.78e-06 |

**Overall max |Δ| = 1.78e-06** (worst single component: cubochoric `cu[1]`, legacy `0.04432392` vs NX `0.04432571`). All conversions agree within 1e-5; four are bit-identical.

## Fixes Applied
None. SIMPLNX is independently verified-correct against the Class 3 / Class 1 / Class 4 oracle; the legacy output is not wrong, so no legacy patch and no NX change. The sub-2e-6 differences are float32 round-off from the differing intermediate-conversion paths (legacy routes everything through a quaternion intermediate; NX uses EbsdLib 2.0 direct `input.toX()`). Largest in cubochoric, which involves a cube-root + series expansion most sensitive to intermediate precision.

## Notes
- Confirms deviation **ConvertOrientationsFilter-D1** (order of operations + library): differences ≤ ~1.8e-6, recommendation "either acceptable within tolerance ~1e-5".
- D2 (float64 scope) not exercised here (input is float32). D3 (Stereographic) has no legacy equivalent. D4 (error codes) is a preflight-only difference.
