import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadH5EbsdFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("CellData"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_filter=cx.DataPath("Data/Output/Reconstruction/Small_IN100.h5ebsd"),
)
#Filter 2

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["DataContainer/CellData/Image Quality"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath(["DataContainer/CellData/ConfidenceIndex"])
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)

#Filter 3

result = cxor.ConvertOrientations.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("DataContainer/CellData/EulerAngles"),
    input_type=0,
    output_orientation_array_name=("Quats"),
    output_type=2,
)
#Filter 4

result = cxor.AlignSectionsMutualInformationFilter.execute(
    data_structure=data_structure,
    alignment_shift_file_name=cx.DataPath("Data/Output/OrientationAnalysis/Alignment_By_Mutual_Information_Shifts.txt"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    good_voxels_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
    use_good_voxels=True,
    write_alignment_shifts=True,
)
#Filter 5

output_file_path = "Data/Output/OrientationAnalysis/SmallIN100_AlignSectionsMutualInformation.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")