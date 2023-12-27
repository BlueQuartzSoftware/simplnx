import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.CreateImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name=("Cell Data"),
    dimensions=[100, 100, 2],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[0.0, 0.0, 0.0],
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
filter = cx.ReadRawBinaryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_attribute_array_path=cx.DataPath("[Image Geometry]/Cell Data/Quats"),
    endian=0,
    input_file=nxtest.GetDataDirectory() + "Data/OrientationAnalysis/quats.raw",
    number_of_components=4,
    scalar_type=cx.NumericType.float32,
    skip_header_bytes=0,
    tuple_dimensions=[[2.0, 100.0, 100.0]]
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
filter = cxor.ConvertOrientations()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("[Image Geometry]/Cell Data/Quats"),
    input_type=2,
    output_orientation_array_name="Eulers",
    output_type=0
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
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="1",
    numeric_type=cx.NumericType.int32,
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/Phases"),
    tuple_dimensions=[[2.0, 100.0, 100.0]]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cxor.ReadEnsembleInfoFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_ensemble_attribute_matrix_name=("Cell Ensemble"),
    crystal_structures_array_name=("CrystalStructures"),
    data_container_name=cx.DataPath("[Image Geometry]"),
    input_file=nxtest.GetDataDirectory() + "Data/OrientationAnalysis/Ensemble.ini",
    phase_types_array_name=("PhaseTypes")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 6
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("[Image Geometry]/Cell Data/Eulers"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=cx.DataPath("[Image Geometry]/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("[Image Geometry]/Cell Ensemble/CrystalStructures"),
    #mask_array_path=cx.DataPath(),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Set Output file path
output_file_path = nxtest.GetDataDirectory() + "Data/Output/Examples/EnsembleInfoReaderExample.dream3d"
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