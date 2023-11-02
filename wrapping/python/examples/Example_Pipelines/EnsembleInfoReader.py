import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cx.CreateImageGeometry.execute(
    data_structure=data_structure,
    cell_data_name=("Cell Data"),
    dimensions=[100, 100, 2],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)

#Filter 2

result = cx.RawBinaryReaderFilter.execute(
    data_structure=data_structure,
    created_attribute_array_path=cx.DataPath("Quats"),
    endian=0,
    input_file=cx.DataPath("Data/OrientationAnalysis/quats.raw"),
    number_of_components=4,
    scalar_type=8,
    skip_header_bytes=0,
    tuple_dimensions=[[2.0, 100.0, 100.0]]
)

#Filter 3

result = cxor.ConvertOrientations.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("[Image Geometry]/Cell Data/Quats"),
    input_type=2,
    output_orientation_array_name=("Eulers"),
    output_type=0
)

#Filter 4

result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format=("Unknown"),
    initialization_value=("1"),
    numeric_type=4,
    output_data_array=cx.DataPath("Phases"),
    tuple_dimensions=[[2.0, 100.0, 100.0]]
)

#Filter 5

result = cxor.EnsembleInfoReaderFilter.execute(
    data_structure=data_structure,
    cell_ensemble_attribute_matrix_name=("Cell Ensemble"),
    crystal_structures_array_name=("CrystalStructures"),
    data_container_name=cx.DataPath("[Image Geometry]"),
    input_file=cx.DataPath("Data/OrientationAnalysis/Ensemble.ini"),
    phase_types_array_name=("PhaseTypes")
)

#Filter 6

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("[Image Geometry]/Cell Data/Eulers"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=cx.DataPath("[Image Geometry]/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("[Image Geometry]/Cell Ensemble/CrystalStructures"),
    #good_voxels_array_path=cx.DataPath(),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=False
)

#Filter 7

output_file_path = "Data/Output/Examples/EnsembleInfoReaderExample.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")