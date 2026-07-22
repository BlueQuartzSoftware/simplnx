#!/usr/bin/env python3
"""Author the shared legacy-format input file for the CAxisSegmentFeatures A/B comparison.

Builds four pure-Phi Bunge fixtures (the same Class 1 analytical fixtures encoded in
CAxisSegmentFeaturesTest.cpp) as separate DataContainers in one legacy v7 .dream3d file that
BOTH the DREAM3D 6.5.171 PipelineRunner and nxrunner read:

  TC1_Chain   8x1x1  Phi = [0, 5, 8, 45, 50, 120, 124, 90], tol 10  -> partition {0,1,2}{3,4}{5,6}{7}
  TC2_PiFold  3x1x1  Phi = [2, 176, 88], tol 10                      -> partition {0,1}{2}
  TC3_Mask    5x1x1  Phi = [0, 20, 22, 0, 90], mask [0,1,1,0,1]      -> {1,2}{4}, cells 0,3 -> id 0
  TC4_Phase0  4x1x1  Phi = [0,0,0,0], phases [0,1,1,1]               -> {1,2,3}, cell 0 -> id 0

A pure rotation about x by Phi tilts the c-axis by Phi in the sample y-z plane, so the c-axis
angular distance between two cells is exactly min(|dPhi|, 180 - |dPhi|). Quaternion storage is
{x, y, z, w} = {sin(Phi/2), 0, 0, cos(Phi/2)} for both SIMPL and NX.

Usage: python3 make_input.py  (writes ../results/caxis_ab_input.dream3d)
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, "/Users/mjackson/Workspace1/Claude_Support/skills/compare-legacy-dream3d")
from legacy_dream3d import D3DLegacyWriter  # noqa: E402

HEX_HIGH = 0
UNKNOWN = 999


def quat_from_phi_deg(phi_deg):
    half = math.radians(phi_deg) * 0.5
    return [math.sin(half), 0.0, 0.0, math.cos(half)]


def add_case(w, dc_name, phis, phases=None, mask=None, num_ensembles=2):
    n = len(phis)
    w.add_image_geom(dc_name, (n, 1, 1))
    w.add_attribute_matrix(dc_name, "CellData", (1, 1, n), "Cell")
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

    print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
