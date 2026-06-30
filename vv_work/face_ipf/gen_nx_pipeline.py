#!/usr/bin/env python3
import json

IN = "/Users/mjackson/Workspace1/DREAM3D_Data/TestFiles/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d"
OUT = "/Users/mjackson/Workspace3/simplnx/vv_work/face_ipf/out_nx_fixed.dream3d"


def arg(v):
    return {"value": v, "version": 1}


pipeline = {
    "name": "FaceIPF NX fixed",
    "pipeline": [
        {
            "args": {"import_data_object": arg({"file_path": IN, "data_paths": None})},
            "comments": "",
            "filter": {"name": "nx::core::ReadDREAM3DFilter", "uuid": "0dbd31c7-19e0-4077-83ef-f4a6459a0e2d"},
            "isDisabled": False,
        },
        {
            "args": {
                "surface_mesh_face_labels_array_path": arg("TriangleDataContainer/FaceData/FaceLabels"),
                "surface_mesh_face_normals_array_path": arg("TriangleDataContainer/FaceData/FaceNormals"),
                "feature_euler_angles_array_path": arg("Small IN100/Grain Data/AvgEulerAngles"),
                "feature_phases_array_path": arg("Small IN100/Grain Data/Phases"),
                "crystal_structures_array_path": arg("Small IN100/Phase Data/CrystalStructures"),
                "first_face_ipf_colors_array_name": arg("NX_FaceIPF_First"),
                "second_face_ipf_colors_array_name": arg("NX_FaceIPF_Second"),
                "color_key_index": arg(0),
            },
            "comments": "",
            "filter": {"name": "nx::core::ComputeFaceIPFColoringFilter", "uuid": "30759600-7c02-4650-b5ca-e7036d6b568e"},
            "isDisabled": False,
        },
        {
            "args": {"export_file_path": arg(OUT), "write_xdmf_file": arg(False)},
            "comments": "",
            "filter": {"name": "nx::core::WriteDREAM3DFilter", "uuid": "b3a95784-2ced-41ec-8d3d-0242ac130003"},
            "isDisabled": False,
        },
    ],
    "version": 1,
}

with open("/Users/mjackson/Workspace3/simplnx/vv_work/face_ipf/nx_FaceIPF.d3dpipeline", "w") as fh:
    json.dump(pipeline, fh, indent=2)
print("wrote nx_FaceIPF.d3dpipeline")
