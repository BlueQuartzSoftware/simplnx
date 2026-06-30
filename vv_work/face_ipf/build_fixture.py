#!/usr/bin/env python3
"""Build a legacy-native triangle-mesh .dream3d fixture for the FaceIPF A/B.

Mirrors the Class 1 analytical unit-test fixture: 4 faces, feature1=cubic / feature2=hex,
identity orientations, corner-aligned face normals. Both legacy PipelineRunner binaries
(6.5.171 buggy, 6.5.172 fixed) can read it.
"""
import sys
import numpy as np

sys.path.insert(0, "/Users/mjackson/Workspace1/Claude_Support/skills/compare-legacy-dream3d")
from legacy_dream3d import D3DLegacyWriter

OUT = "/Users/mjackson/Workspace3/simplnx/vv_work/face_ipf/fixture_legacy.dream3d"

# 4 faces. Trivial connectivity (3 unique verts per triangle); positions are irrelevant to
# IPF coloring (the filter reads the FaceNormals array, not computed normals).
n_faces = 4
vertices = np.zeros((n_faces * 3, 3), dtype=np.float32)
for i in range(n_faces * 3):
    vertices[i] = [float(i), 0.0, 0.0]
triangles = np.arange(n_faces * 3, dtype=np.int64).reshape(n_faces, 3)

face_labels = np.array([[1, 2], [-1, 2], [1, -1], [1, 3]], dtype=np.int32)
face_normals = np.array([[-1.0, 0.0, 0.0],
                         [-1.0, 0.0, 0.0],
                         [-1.0, 0.0, 0.0],
                         [1.0, 1.0, 1.0]], dtype=np.float64)

# 4 features (index 0 unused); identity Euler angles
avg_euler = np.zeros((4, 3), dtype=np.float32)
phases = np.array([[0], [1], [2], [1]], dtype=np.int32)
# phase 0 unknown(999), phase 1 cubic-high(1), phase 2 hex-high(0)
crystal_structures = np.array([[999], [1], [0]], dtype=np.uint32)

with D3DLegacyWriter(OUT) as w:
    # Triangle DC: geometry + face data
    w.add_triangle_geom("TriangleDataContainer", vertices, triangles)
    w.add_attribute_matrix("TriangleDataContainer", "FaceData", (n_faces,), "Face")
    w.add_data_array("TriangleDataContainer", "FaceData", "FaceLabels", face_labels, comp_dims=(2,))
    w.add_data_array("TriangleDataContainer", "FaceData", "FaceNormals", face_normals, comp_dims=(3,))

    # Image DC: feature + ensemble data (GenerateFaceIPFColoring requires these in an Image geom)
    w.add_image_geom("Small IN100", dims=(1, 1, 1))
    w.add_attribute_matrix("Small IN100", "Grain Data", (4,), "CellFeature")
    w.add_data_array("Small IN100", "Grain Data", "AvgEulerAngles", avg_euler, comp_dims=(3,))
    w.add_data_array("Small IN100", "Grain Data", "Phases", phases, comp_dims=(1,))
    w.add_attribute_matrix("Small IN100", "Phase Data", (3,), "CellEnsemble")
    w.add_data_array("Small IN100", "Phase Data", "CrystalStructures", crystal_structures, comp_dims=(1,))

print("wrote", OUT)
