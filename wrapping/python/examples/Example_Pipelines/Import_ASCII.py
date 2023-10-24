import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cx.CreateImageGeometry.execute(
    data_structure=data_structure,
    cell_data_name="CellData",
    dimensions=[189, 201, 1],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[0.0, 0.0, 0.0],
    spacing=[0.25, 0.25, 1.0],
)
#Filter 2

result = cx.ImportCSVDataFilter.execute(
    data_structure=data_structure,
    #created_data_group: DataPath = ...,
    selected_data_group=cx.DataPath("[Image Geometry]/CellData"),
    tuple_dimensions=[1.0, 201.0, 189,0],
    use_existing_group=True,
    #wizard_data=CSVWizardData
)
#Filter 3

result = cx.CombineAttributeArraysFilter.execute(
    data_structure=data_structure,
    move_values=False,
    normalize_data=False,
    selected_data_array_paths=[cx.DataPath(["[Image Geometry]/CellData/phil"]),cx.DataPath(["[Image Geometry]/CellData/Phi"]),cx.DataPath(["[Image Geometry]/CellData/phi2"])],
    stacked_data_array_name="Eulers",
)
#Filter 4

result = cx.DeleteData.execute(
    data_structure=data_structure,
    removed_data_path=[cx.DataPath(["[Image Geometry]/CellData/phi1"]),cx.DataPath(["[Image Geometry]/CellData/Phi"]),cx.DataPath(["[Image Geometry]/CellData/phi2"])],
)
#Filter 5

ensemble_info_parameter = []
ensemble_info_parameter.append(["Hexagonal-High 6/mmm","Primary","Phase 1"]),
ensemble_info_parameter.append(["Cubic-High m-3m","Primary","Phase 2"]),
result = cxor.CreateEnsembleInfoFilter.execute(data_structure=data_structure,
cell_ensemble_attribute_matrix_name=cx.DataPath(["CellEnsembleData"]),
crystal_structures_array_name="CrystalStructures",
phase_names_array_name="PhaseNames",
phase_types_array_name="PhaseTypes",
ensemble=ensemble_info_parameter
)
#Filter 6

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("[Image Geometry]/CellData/Phase"),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=cx.DataPath("[Image Geometry]/CellData/Phase"),
    crystal_structures_array_path=cx.DataPath("[Image Geometry]/CellEnsembleData/CrystalStructures"),
    #good_voxels_array_path: DataPath = ...,
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=False,
)
#Filter 7

result = cxitk.ITKImageWriter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/Import_ASCII_IPF.png"),
    image_array_path=cx.DataPath("[Image Geometry]/CellData/IPFColors"),
    image_geom_path=cx.DataPath("[Image Geometry]"),
    index_offset=0
    plane=0
)
#Filter 8

output_file_path = "Data/Output/Small_IN100/EnsembleData.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure,
                                        export_file_path=output_file_path,
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")