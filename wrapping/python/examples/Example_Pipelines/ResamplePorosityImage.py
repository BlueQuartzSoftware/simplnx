import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
generated_file_list_value = cx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = "Data/Porosity_Image"
generated_file_list_value.ordering = cx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2

result = cxitk.ITKImportImageStack.execute(
    data_structure=data_structure,
    cell_data_name = "Cell Data",
    image_data_array_path = "ImageData",
    image_geometry_path=cx.DataPath("Porosity_Image"),
    image_transform_choice=0,
    input_file_list_info=generated_file_list_value,
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0],
)
#Filter 2
result = cx.ResampleImageGeomFilter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    #exact_dimensions: List[int] = ...,
    #feature_ids_path: DataPath = ...,
    #new_data_container_path: DataPath = ...,
    #remove_original_geometry: bool = ...,
    renumber_features=False,
    resampling_mode=0,
    #scaling: List[float] = ...,
    selected_image_geometry=cx.DataPath("Porosity_Image"),
    spacing=[2.0, 2.0, 2.0],
)
#Filter 3
result = cx.ResampleImageGeomFilter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    #exact_dimensions: List[int] = ...,
    #feature_ids_path: DataPath = ...,
    #new_data_container_path: DataPath = ...,
    #remove_original_geometry: bool = ...,
    renumber_features=False,
    resampling_mode=1,
    scaling=[0.5, 0.5, 0.5],
    selected_image_geometry=cx.DataPath("Porosity_Image"),
    #spacing: List[float] = ...
)
#Filter 4
result = cx.ResampleImageGeomFilter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    exact_dimensions=[262, 195, 82],
    #feature_ids_path: DataPath = ...,
    #new_data_container_path: DataPath = ...,
    #remove_original_geometry: bool = ...,
    renumber_features=False,
    resampling_mode=2,
    #scaling: List[float] = ...,
    selected_image_geometry=cx.DataPath("Porosity_Image"),
    #spacing: List[float] = ...
)
#Filter 5
result =cx.ExportDREAM3DFilter.execute(
    data_structure=data_structure,
    export_file_path=cx.DataPath("Data/Output/ResamplePorosityImage.dream3d"),
    write_xdmf_file=True,
)