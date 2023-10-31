import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cx.CreateImageGeometry.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    dimensions=[60, 80, 100],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[100.0, 100.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)

#Filter 2

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/ConfidenceIndex.csv"),
    n_comp=1,
    n_skip_lines=0,
    n_tuples=[0, 480000],
    output_data_array=cx.DataPath("Confidence Index"),
    scalar_type=8
)

#Filter 3

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/FeatureIds.csv"),
    n_comp=1,
    n_skip_lines=0,
    n_tuples=[0, 480000],
    output_data_array=cx.DataPath("FeatureIds"),
    scalar_type=4
)

#Filter 4

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/ImageQuality.csv"),
    n_comp=1,
    n_skip_lines=0,
    n_tuples=[0, 480000],
    output_data_array=cx.DataPath("Image Quality"),
    scalar_type=8
)

#Filter 5

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/IPFColor.csv"),
    n_comp=3,
    n_skip_lines=0,
    n_tuples=[0, 480000],
    output_data_array=cx.DataPath("IPFColors"),
    scalar_type=0
)

#Filter 6

result = cx.CropImageGeometry.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix: DataPath = ...,
    created_image_geometry=cx.DataPath("CroppedBottomHalf"),
    #feature_ids: DataPath = ...,
    max_voxel=[59, 79, 50],
    min_voxel=[0, 0, 0],
    remove_original_geometry=False,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("[Image Geometry]"),
    update_origin=False
)

#Filter 7

result = cx.CropImageGeometry.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix: DataPath = ...,
    created_image_geometry=cx.DataPath("CroppedTopHalf"),
    #feature_ids: DataPath = ...,
    max_voxel=[59, 79, 99],
    min_voxel=[0, 0, 51],
    remove_original_geometry=False,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("[Image Geometry]"),
    update_origin=True
)

#Filter 8

result = cx.AppendImageGeometryZSliceFilter.execute(
    data_structure=data_structure,
    check_resolution=True,
    destination_geometry=cx.DataPath("CroppedBottomHalf"),
    input_geometry=cx.DataPath("CroppedTopHalf"),
    new_geometry=cx.DataPath("AppendedImageGeom"),
    save_as_new_geometry=True
)

#Filter 9

output_file_path = "Data/Output/Examples/AppendImageGeometryZSlice.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")