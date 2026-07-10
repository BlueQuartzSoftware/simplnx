#!/usr/bin/env python3
"""Generate legacy SIMPL .json pipelines: DataContainerReader -> RotateSampleRefFrame -> DataContainerWriter."""
import json

WORK = "/private/tmp/claude-501/-Users-mjackson-Workspace3-simplnx/8f4ca080-1346-435d-9929-f3a94423bf95/scratchpad/rotate_vv"
INP = f"{WORK}/input_rotate.dream3d"


def make(case, axis, angle):
    out = f"{WORK}/output_6_5_171_{case}.dream3d"
    pipe = {
        "0": {
            "FilterVersion": "6.5.171", "Filter_Enabled": True, "Filter_Human_Label": "Read DREAM.3D Data File",
            "Filter_Name": "DataContainerReader", "Filter_Uuid": "{043cbde5-3878-5718-958f-ae75714df0df}",
            "InputFile": INP, "OverwriteExistingDataContainers": False,
            "InputFileDataContainerArrayProxy": {
                "Data Containers": [{
                    "Attribute Matricies": [{
                        "Data Arrays": [{
                            "Component Dimensions": [1], "Flag": 2, "Name": "Data", "Object Type": "DataArray<int32_t>",
                            "Path": "/DataContainers/ImageDataContainer/CellData", "Tuple Dimensions": [1], "Version": 2,
                        }],
                        "Flag": 2, "Name": "CellData", "Type": 3,
                    }],
                    "Flag": 2, "Geometry": {"Geometry_Type": 0, "Geometry_Type_Name": "ImageGeometry"}, "Name": "ImageDataContainer",
                }],
                "Version": 6,
            },
        },
        "1": {
            "FilterVersion": "6.5.171", "Filter_Enabled": True, "Filter_Human_Label": "Rotate Sample Reference Frame",
            "Filter_Name": "RotateSampleRefFrame", "Filter_Uuid": "{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}",
            "RotationAngle": float(angle), "RotationAxis": {"x": float(axis[0]), "y": float(axis[1]), "z": float(axis[2])},
            "CellAttributeMatrixPath": {"Data Container Name": "ImageDataContainer", "Attribute Matrix Name": "CellData", "Data Array Name": ""},
        },
        "2": {
            "FilterVersion": "6.5.171", "Filter_Enabled": True, "Filter_Human_Label": "Write DREAM.3D Data File",
            "Filter_Name": "DataContainerWriter", "Filter_Uuid": "{3fcd4c43-9d75-5b86-aad4-4441bc914f37}",
            "OutputFile": out, "WriteTimeSeries": False, "WriteXdmfFile": False,
        },
        "PipelineBuilder": {"Name": f"legacy_rotate_{case}", "Number_Filters": 3, "Version": 6},
    }
    path = f"{WORK}/simpl_{case}.json"
    json.dump(pipe, open(path, "w"), indent=2)
    print("wrote", path)


CASES = {"90Z": ((0, 0, 1), 90), "180Z": ((0, 0, 1), 180), "90X": ((1, 0, 0), 90), "180Y": ((0, 1, 0), 180)}
if __name__ == "__main__":
    for c, (ax, ang) in CASES.items():
        make(c, ax, ang)
