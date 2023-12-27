import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.CreateImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    dimensions=[60, 80, 100],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[100.0, 100.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "Data/ASCIIData/ConfidenceIndex.csv",
    n_comp=1,
    n_skip_lines=0,
    n_tuples=[[480000.0]],
    output_data_array=cx.DataPath("Confidence Index"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "Data/ASCIIData/FeatureIds.csv",
    n_comp=1,
    n_skip_lines=0,
    n_tuples=[[480000.0]],
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/FeatureIds"),
    scalar_type=cx.NumericType.int32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 4
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "Data/ASCIIData/ImageQuality.csv",
    n_comp=1,
    n_skip_lines=0,
    n_tuples=[[480000.0]],
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/Image Quality"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running ReadTextDataArrayFilter")

# Filter 5
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "Data/ASCIIData/IPFColor.csv",
    n_comp=3,
    n_skip_lines=0,
    n_tuples=[[480000.0]],
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/IPFColors"),
    scalar_type=cx.NumericType.uint8
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running ReadTextDataArrayFilter")

# Filter 6
# Instantiate Filter
filter = cx.CropImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_image_geometry=cx.DataPath("CroppedBottomHalf"),
    max_voxel=[59, 79, 50],
    min_voxel=[0, 0, 0],
    remove_original_geometry=False,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("[Image Geometry]"),
    update_origin=False
    # cell_feature_attribute_matrix: DataPath = ...,  # Not currently part of the code
    # feature_ids: DataPath = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the CropImageGeometry")


# Filter 7
# Instantiate Filter
filter = cx.CropImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_image_geometry=cx.DataPath("CroppedTopHalf"),
    max_voxel=[59, 79, 99],
    min_voxel=[0, 0, 51],
    remove_original_geometry=False,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("[Image Geometry]"),
    update_origin=True
    # cell_feature_attribute_matrix: DataPath = ...,  # Not currently part of the code
    # feature_ids: DataPath = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the CropImageGeometry for the top half")

# Filter 8
# Instantiate Filter
filter = cx.AppendImageGeometryZSliceFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    check_resolution=True,
    destination_geometry=cx.DataPath("CroppedBottomHalf"),
    input_geometry=cx.DataPath("CroppedTopHalf"),
    new_geometry=cx.DataPath("AppendedImageGeom"),
    save_as_new_geometry=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the AppendImageGeometryZSliceFilter")

# Filter 9
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.GetDataDirectory() + "Data/Output/Examples/AppendImageGeometryZSlice.dream3d"
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")