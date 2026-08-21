# Deviations from DREAM3D 6.5.171: RotateEulerRefFrameFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`RotateEulerRefFrameFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

| | |
|---|---|
| SIMPLNX UUID | `0458edcd-3655-4465-adc8-b036d76138b5` |
| Legacy (SIMPL) UUID | `{ef9420b2-8c46-55f3-8ae4-f53790639de4}` |
| Comparison fixture | 6 axis/angle cases × 12 orientations, shared CSV input (`Code_Review/RotateEulerRefFrame/`) |
| Comparison date | 2026-07-03 |

**Headline: no deviations.** DREAM3D 6.5.171 and SIMPLNX agree to within 7.2e-7 rad (float32 ULP level, wrap-aware) on every comparison case, and both independently match the Class 1 analytical oracle. The entries below are **non-deviations** — differences in representation, precision, or invalid-input handling only — recorded so future engineers do not re-discover them.

---

## Non-deviation N1 — degree→radian conversion precision

| Field | Value |
|---|---|
| **ID** | `RotateEulerRefFrameFilter-N1` (non-deviation) |
| **Filter UUID** | `0458edcd-3655-4465-adc8-b036d76138b5` |
| **Status** | informational |

**Symptom:** None user-visible. Maximum observed output difference between versions is 7.2e-7 rad across all comparison cases.

**Root cause:** Precision. Legacy converts the rotation angle degrees→radians in `float` (`float rotAngle = m_RotationAngle * k_Pi / 180.0`, `RotateEulerRefFrame.cpp` line ~215) before handing it to the double-precision orientation transforms; SIMPLNX performs the conversion in `double` (`Algorithms/RotateEulerRefFrame.cpp`, kernel setup). Both then run identical double-precision math. SIMPLNX lands marginally closer to the double-precision oracle (max 2.3e-7 vs legacy 8.1e-7).

**Affected users:** None at float32 storage precision.

**Recommendation:** Either acceptable within tolerance 1e-5 rad.

---

## Non-deviation N2 — 0 vs 2π canonical representation at the wrap boundary

| Field | Value |
|---|---|
| **ID** | `RotateEulerRefFrameFilter-N2` (non-deviation) |
| **Filter UUID** | `0458edcd-3655-4465-adc8-b036d76138b5` |
| **Status** | informational |

**Symptom:** For an input orientation whose rotated `phi1` lands exactly on the 0/2π boundary — observed for euler (π/2, π/4, ¾π) rotated 90° about Z, where `phi1' = 0` exactly — 6.5.171 outputs `6.2831855` (2π) while SIMPLNX outputs `~6e-17` (0). These represent the same angle.

**Root cause:** Precision (representation). The N1 float conversion places legacy's intermediate `phi1'` at −ε, which om2eu canonicalizes by adding 2π; SIMPLNX's double conversion places it at +6e-17, which needs no wrap. Element-wise comparison without wrap awareness reports a spurious 2π difference for such boundary values.

**Affected users:** Anyone diffing euler arrays element-wise between versions (e.g., regression scripts). Orientation-space comparisons are unaffected.

**Recommendation:** Either acceptable — compare euler angles modulo 2π.

---

## Non-deviation N3 — zero-length rotation axis handling (SIMPLNX guard added 2026-07-03)

| Field | Value |
|---|---|
| **ID** | `RotateEulerRefFrameFilter-N3` (non-deviation; deliberate input-validation difference) |
| **Filter UUID** | `0458edcd-3655-4465-adc8-b036d76138b5` |
| **Status** | active |

**Symptom:** Given rotation axis (0,0,0), 6.5.171 silently fills the entire euler array with NaN (unguarded `axis/|axis|` division). SIMPLNX fails preflight with error `-96200` and a descriptive message.

**Root cause:** Bug (input-validation gap) in 6.5.171, closed on the SIMPLNX side during algorithm review. Not an output deviation for any valid input — outputs are identical whenever the axis is non-zero. Pinned by test `RotateEulerRefFrameFilter: Zero-Length Axis Fails Preflight`.

**Affected users:** Users with malformed pipelines: legacy destroys their data silently; SIMPLNX refuses to run.

**Recommendation:** Trust SIMPLNX. No legacy patch proposed — the legacy codebase is patched only for demonstrably wrong output on *valid* input.
