import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
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

# Instantiate Filter
filter = cxitk.ITKImportImageStack()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    image_data_array_path="ImageData",
    image_geometry_path=cx.DataPath("Porosity_Image"),
    image_transform_choice=0,
    input_file_list_info=generated_file_list_value,
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
filter = cx.ResampleImageGeomFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    #exact_dimensions: List[int] = ...,
    #feature_ids_path: DataPath = ...,
    new_data_container_path=cx.DataPath("Porosity_Image_Resampled_Spacing"),
    remove_original_geometry=False,
    renumber_features=False,
    resampling_mode=0,
    #scaling: List[float] = ...,
    selected_image_geometry=cx.DataPath("Porosity_Image"),
    spacing=[2.0, 2.0, 2.0]
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
filter = cx.ResampleImageGeomFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    #exact_dimensions: List[int] = ...,
    #feature_ids_path: DataPath = ...,
    new_data_container_path=cx.DataPath("Porosity_Image_Resampled_Scaled"),
    remove_original_geometry=False,
    renumber_features=False,
    resampling_mode=1,
    scaling=[0.5, 0.5, 0.5],
    selected_image_geometry=cx.DataPath("Porosity_Image")
    #spacing: List[float] = ...
)
# Error/Result Handling for Filter
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 4
# Instantiate Filter
filter = cx.ResampleImageGeomFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    exact_dimensions=[262, 195, 82],
    #feature_ids_path: DataPath = ...,
    new_data_container_path=cx.DataPath("Porosity_Image_Resampled_Exact_Dims"),
    remove_original_geometry=False,
    renumber_features=False,
    resampling_mode=2,
    #scaling: List[float] = ...,
    selected_image_geometry=cx.DataPath("Porosity_Image")
    #spacing: List[float] = ...
)
# Error/Result Handling for Filter
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 5
# Define output file path
output_file_path = "Data/Output/ResamplePorosityImage.dream3d"
# Execute WriteDREAM3DFilter with Parameters
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")