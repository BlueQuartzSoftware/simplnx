#!/usr/bin/env python3
"""Mint the shared A/B input for RotateSampleRefFrame legacy comparison.

4x3x2 ImageGeom (nx=4, ny=3, nz=2), origin (0,0,0), spacing (1,1,1), with an
Int32 "Data" cell array filled 1..24 in ZYX (slowest-to-fastest) order. Distinct,
nonzero values so the exact voxel permutation produced by each version is visible.
"""
import sys
import os
import numpy as np

sys.path.insert(0, "/Users/mjackson/Workspace1/Claude_Support/skills/compare-legacy-dream3d")
from legacy_dream3d import D3DLegacyWriter

NX, NY, NZ = 4, 3, 2
N = NX * NY * NZ  # 24

data = np.arange(1, N + 1, dtype=np.int32)  # 1..24, ZYX order


def build(out_path):
    with D3DLegacyWriter(out_path) as w:
        w.add_image_geom("ImageDataContainer", dims=(NX, NY, NZ), origin=(0.0, 0.0, 0.0), spacing=(1.0, 1.0, 1.0))
        w.add_attribute_matrix("ImageDataContainer", "CellData", (NZ, NY, NX), "Cell")
        w.add_data_array("ImageDataContainer", "CellData", "Data", data)
    print("wrote", out_path)


if __name__ == "__main__":
    build(sys.argv[1] if len(sys.argv) > 1 else "input_rotate.dream3d")
