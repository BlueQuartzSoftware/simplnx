import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1

# Create the ReadH5EbsdFileParameter and assign values to it.
h5ebsdParameter = cxor.ReadH5EbsdFileParameter.ValueType()
h5ebsdParameter.euler_representation=0
h5ebsdParameter.end_slice=117
h5ebsdParameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
h5ebsdParameter.input_file_path=cxtest.GetDataDirectory() + "Data/Output/Reconstruction/Small_IN100.h5ebsd"
h5ebsdParameter.start_slice=1
h5ebsdParameter.use_recommended_transform=True

# Instantiate Filter
filter = cxor.ReadH5EbsdFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_parameter=h5ebsdParameter
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Set Up Thresholds and Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("DataContainer/CellData/Image Quality")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 120

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath("DataContainer/CellData/Confidence Index")
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 0.1

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = cx.DataType.boolean

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=cx.DataType.boolean
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
    input_orientation_array_path=cx.DataPath("DataContainer/CellData/EulerAngles"),
    input_type=0,
    output_orientation_array_name="Quats",
    output_type=2
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
filter = cxor.AlignSectionsMutualInformationFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    alignment_shift_file_name=cxtest.GetDataDirectory() + "Data/Output/OrientationAnalysis/Alignment_By_Mutual_Information_Shifts.txt",
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
    use_mask=True,
    write_alignment_shifts=True
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
filter = cx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = "Data/Output/OrientationAnalysis/SmallIN100_AlignSectionsMutualInformation.dream3d"
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