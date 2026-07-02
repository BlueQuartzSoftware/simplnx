#!/usr/bin/env python3
"""Generate an NX .d3dpipeline: Read input -> RotateSampleRefFrame (in-place) -> Write output."""
import json
import sys

WORK = "/private/tmp/claude-501/-Users-mjackson-Workspace3-simplnx/8f4ca080-1346-435d-9929-f3a94423bf95/scratchpad/rotate_vv"
IDENTITY_4x4 = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]


def make(case, axis_angle):
    inp = f"{WORK}/input_rotate.dream3d"
    out = f"{WORK}/output_nx_{case}.dream3d"
    pipeline = {
        "name": f"rotate_nx_{case}",
        "pipeline": [
            {
                "args": {"import_data_object": {"value": {"data_paths": [], "file_path": inp, "path_import_policy": 0}, "version": 2}, "parameters_version": 1},
                "comments": "", "filter": {"name": "nx::core::ReadDREAM3DFilter", "uuid": "0dbd31c7-19e0-4077-83ef-f4a6459a0e2d"}, "isDisabled": False,
            },
            {
                "args": {
                    "input_image_geometry_path": {"value": "ImageDataContainer", "version": 1},
                    "output_image_geometry_path": {"value": "ImageDataContainer", "version": 1},
                    "remove_original_geometry": {"value": True, "version": 1},
                    "keep_input_geometry_origin": {"value": False, "version": 1},
                    "rotate_slice_by_slice": {"value": False, "version": 1},
                    "rotation_representation_index": {"value": 0, "version": 1},
                    "rotation_axis_angle": {"value": axis_angle, "version": 1},
                    "rotation_matrix": {"value": IDENTITY_4x4, "version": 1},
                    "parameters_version": 1,
                },
                "comments": "", "filter": {"name": "nx::core::RotateSampleRefFrameFilter", "uuid": "d2451dc1-a5a1-4ac2-a64d-7991669dcffc"}, "isDisabled": False,
            },
            {
                "args": {"export_file_path": {"value": out, "version": 1}, "write_xdmf_file": {"value": False, "version": 1}, "use_compression": {"value": False, "version": 1}, "compression_level": {"value": 1, "version": 1}, "parameters_version": 1},
                "comments": "", "filter": {"name": "nx::core::WriteDREAM3DFilter", "uuid": "b3a95784-2ced-41ec-8d3d-0242ac130003"}, "isDisabled": False,
            },
        ],
        "version": 1,
    }
    path = f"{WORK}/nx_{case}.d3dpipeline"
    json.dump(pipeline, open(path, "w"), indent=2)
    print("wrote", path)


CASES = {"90Z": [0.0, 0.0, 1.0, 90.0], "180Z": [0.0, 0.0, 1.0, 180.0], "90X": [1.0, 0.0, 0.0, 90.0], "180Y": [0.0, 1.0, 0.0, 180.0]}
if __name__ == "__main__":
    for c, aa in CASES.items():
        make(c, aa)
