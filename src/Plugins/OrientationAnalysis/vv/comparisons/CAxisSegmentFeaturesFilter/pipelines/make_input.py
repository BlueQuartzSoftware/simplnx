#!/usr/bin/env python3
"""Author the shared legacy-format input file for the CAxisSegmentFeatures A/B comparison.

Builds four pure-Phi Bunge fixtures (the same Class 1 analytical fixtures encoded in
CAxisSegmentFeaturesTest.cpp) as separate DataContainers in one legacy v7 .dream3d file that
BOTH the DREAM3D 6.5.171 PipelineRunner and nxrunner read:

  TC1_Chain   8x1x1  Phi = [0, 5, 8, 45, 50, 120, 124, 90], tol 10  -> partition {0,1,2}{3,4}{5,6}{7}
  TC2_PiFold  3x1x1  Phi = [2, 176, 88], tol 10                      -> partition {0,1}{2}
  TC3_Mask    5x1x1  Phi = [0, 20, 22, 0, 90], mask [0,1,1,0,1]      -> {1,2}{4}, cells 0,3 -> id 0
  TC4_Phase0  4x1x1  Phi = [0,0,0,0], phases [0,1,1,1]               -> {1,2,3}, cell 0 -> id 0
  TC5_3D      3x2x2  Phi = [0,50,55, 4,90,53, 8,95,130, 6,176,120]   -> see below
              mask [0,1,1,1,1,0, 1,1,1,1,1,1]

TC5_3D exercises the y- and z-stride branches of the 3-D flood fill against legacy (TC1-TC4 are
all 1-D lines). With x-fastest linearization (idx = x + 3y + 6z) and tol 10 the expected partition
is: {1,2} via x; {3,6,9,10} spanning a z hop (3->9: |4-6|=2), a y hop (9->6: |6-8|=2), and a
pi-fold x hop (9->10: fold(6,176)=10); singletons {4} and {7}; {8,11} via a y hop at z=1
(|130-120|=10). Masked cells 0 and 5 keep id 0 (cell 0 also pins the validated-first-seed path).
Expected FeatureIds (canonical NX labeling): [0,1,1,2,3,0,2,4,5,2,2,5] -> 5 features.

A pure rotation about x by Phi tilts the c-axis by Phi in the sample y-z plane, so the c-axis
angular distance between two cells is exactly min(|dPhi|, 180 - |dPhi|). Quaternion storage is
{x, y, z, w} = {sin(Phi/2), 0, 0, cos(Phi/2)} for both SIMPL and NX.

Usage: python3 make_input.py  (writes ../results/caxis_ab_input.dream3d)
"""
import math
import os
import sys

import numpy as np

# The legacy_dream3d writer module ships with the compare-legacy-dream3d skill; point
# LEGACY_DREAM3D_SKILL_DIR at a checkout of it if yours lives elsewhere.
sys.path.insert(0, os.environ.get("LEGACY_DREAM3D_SKILL_DIR", "/Users/mjackson/Workspace1/Claude_Support/skills/compare-legacy-dream3d"))
from legacy_dream3d import D3DLegacyWriter  # noqa: E402

HEX_HIGH = 0  # EbsdLib CrystalStructure enum value for Hexagonal_High
UNKNOWN = 999  # EbsdLib CrystalStructure "unknown" sentinel (ensemble 0)


def quat_from_phi_deg(phi_deg):
    half = math.radians(phi_deg) * 0.5
    return [math.sin(half), 0.0, 0.0, math.cos(half)]


def add_case(w, dc_name, phis, phases=None, mask=None, num_ensembles=2, dims=None):
    n = len(phis)
    if dims is None:
        dims = (n, 1, 1)  # (x, y, z)
    assert dims[0] * dims[1] * dims[2] == n
    w.add_image_geom(dc_name, dims)
    w.add_attribute_matrix(dc_name, "CellData", (dims[2], dims[1], dims[0]), "Cell")
    quats = np.array([quat_from_phi_deg(p) for p in phis], dtype=np.float32)
    w.add_data_array(dc_name, "CellData", "Quats", quats, comp_dims=(4,))
    if phases is None:
        phases = [1] * n
    w.add_data_array(dc_name, "CellData", "Phases", np.array(phases, dtype=np.int32))
    if mask is not None:
        w.add_data_array(dc_name, "CellData", "Mask", np.array(mask, dtype=bool), object_type="DataArray<bool>")
    w.add_attribute_matrix(dc_name, "CellEnsembleData", (num_ensembles,), "CellEnsemble")
    structures = np.full(num_ensembles, HEX_HIGH, dtype=np.uint32)
    structures[0] = UNKNOWN
    w.add_data_array(dc_name, "CellEnsembleData", "CrystalStructures", structures)


def main():
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.abspath(os.path.join(out_dir, "caxis_ab_input.dream3d"))

    with D3DLegacyWriter(out_path) as w:
        add_case(w, "TC1_Chain", [0.0, 5.0, 8.0, 45.0, 50.0, 120.0, 124.0, 90.0])
        add_case(w, "TC2_PiFold", [2.0, 176.0, 88.0])
        add_case(w, "TC3_Mask", [0.0, 20.0, 22.0, 0.0, 90.0], mask=[0, 1, 1, 0, 1])
        add_case(w, "TC4_Phase0", [0.0, 0.0, 0.0, 0.0], phases=[0, 1, 1, 1])
        add_case(w, "TC5_3D", [0.0, 50.0, 55.0, 4.0, 90.0, 53.0, 8.0, 95.0, 130.0, 6.0, 176.0, 120.0], mask=[0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1], dims=(3, 2, 2))

    print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
