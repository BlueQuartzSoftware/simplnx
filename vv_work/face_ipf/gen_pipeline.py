#!/usr/bin/env python3
"""Generate SIMPL pipelines for the GenerateFaceIPFColoring legacy A/B comparison."""
import json
import sys

IN = "/Users/mjackson/Workspace1/DREAM3D_Data/TestFiles/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d"
OUT_NAME = "NXCompare_IPFColors"  # created 6-component array


def da(name, path, obj_type, comps):
    return {
        "Component Dimensions": [comps],
        "Flag": 2,
        "Name": name,
        "Object Type": obj_type,
        "Path": path,
        "Tuple Dimensions": [1],
        "Version": 2,
    }


def reader(input_file):
    proxy = {
        "Data Containers": [
            {
                "Attribute Matricies": [
                    {
                        "Data Arrays": [
                            da("FaceLabels", "/DataContainers/TriangleDataContainer/FaceData", "DataArray<int32_t>", 2),
                            da("FaceNormals", "/DataContainers/TriangleDataContainer/FaceData", "DataArray<double>", 3),
                        ],
                        "Flag": 2,
                        "Name": "FaceData",
                        "Type": 2,  # Face
                    }
                ],
                "Flag": 2,
                "Geometry": {"Geometry_Type": 4, "Geometry_Type_Name": "TriangleGeometry"},
                "Name": "TriangleDataContainer",
            },
            {
                "Attribute Matricies": [
                    {
                        "Data Arrays": [
                            da("AvgEulerAngles", "/DataContainers/Small IN100/Grain Data", "DataArray<float>", 3),
                            da("Phases", "/DataContainers/Small IN100/Grain Data", "DataArray<int32_t>", 1),
                        ],
                        "Flag": 2,
                        "Name": "Grain Data",
                        "Type": 7,  # CellFeature
                    },
                    {
                        "Data Arrays": [
                            da("CrystalStructures", "/DataContainers/Small IN100/Phase Data", "DataArray<uint32_t>", 1),
                        ],
                        "Flag": 2,
                        "Name": "Phase Data",
                        "Type": 11,  # CellEnsemble
                    },
                ],
                "Flag": 2,
                "Geometry": {"Geometry_Type": 0, "Geometry_Type_Name": "ImageGeometry"},
                "Name": "Small IN100",
            },
        ],
        "Version": 6,
    }
    return {
        "FilterVersion": "6.5.171",
        "Filter_Enabled": True,
        "Filter_Human_Label": "Read DREAM.3D Data File",
        "Filter_Name": "DataContainerReader",
        "Filter_Uuid": "{043cbde5-3878-5718-958f-ae75714df0df}",
        "InputFile": input_file,
        "OverwriteExistingDataContainers": False,
        "InputFileDataContainerArrayProxy": proxy,
    }


def dap(dc, am, name):
    return {"Data Container Name": dc, "Attribute Matrix Name": am, "Data Array Name": name}


def ipf_filter():
    return {
        "FilterVersion": "6.5.171",
        "Filter_Enabled": True,
        "Filter_Human_Label": "Generate IPF Colors (Face)",
        "Filter_Name": "GenerateFaceIPFColoring",
        "Filter_Uuid": "{0a121e03-3922-5c29-962d-40d88653f4b6}",
        "SurfaceMeshFaceLabelsArrayPath": dap("TriangleDataContainer", "FaceData", "FaceLabels"),
        "SurfaceMeshFaceNormalsArrayPath": dap("TriangleDataContainer", "FaceData", "FaceNormals"),
        "FeatureEulerAnglesArrayPath": dap("Small IN100", "Grain Data", "AvgEulerAngles"),
        "FeaturePhasesArrayPath": dap("Small IN100", "Grain Data", "Phases"),
        "CrystalStructuresArrayPath": dap("Small IN100", "Phase Data", "CrystalStructures"),
        "SurfaceMeshFaceIPFColorsArrayName": OUT_NAME,
    }


def writer(output_file):
    return {
        "FilterVersion": "6.5.171",
        "Filter_Enabled": True,
        "Filter_Human_Label": "Write DREAM.3D Data File",
        "Filter_Name": "DataContainerWriter",
        "Filter_Uuid": "{3fcd4c43-9d75-5b86-aad4-4441bc914f37}",
        "OutputFile": output_file,
        "WriteTimeSeries": False,
        "WriteXdmfFile": False,
    }


def build(output_file, name):
    return {
        "0": reader(IN),
        "1": ipf_filter(),
        "2": writer(output_file),
        "PipelineBuilder": {"Name": name, "Number_Filters": 3, "Version": 6},
    }


if __name__ == "__main__":
    out_dir = "/Users/mjackson/Workspace3/simplnx/vv_work/face_ipf"
    for ver in ("6_5_171", "6_5_172"):
        pl = build(f"{out_dir}/out_{ver}.dream3d", f"FaceIPF legacy AB {ver}")
        with open(f"{out_dir}/legacy_FaceIPF_{ver}.json", "w") as fh:
            json.dump(pl, fh, indent=2)
        print(f"wrote legacy_FaceIPF_{ver}.json")
