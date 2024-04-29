import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

#Filter 1

generated_file_list_value = nx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = str(nxtest.get_data_directory() / "Porosity_Image")
generated_file_list_value.ordering = nx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2

# Instantiate Filter
nx_filter = cxitk.ITKImportImageStackFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    image_data_array_name="ImageData",
    output_image_geometry_path=nx.DataPath("Porosity_Image"),
    image_transform_index=0,
    input_file_list_object =generated_file_list_value,
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
nxtest.check_filter_result(nx_filter, result)


# Filter 2
# Instantiate Filter
nx_filter = nx.ResampleImageGeomFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    #exact_dimensions: List[int] = ...,
    #feature_ids_path: DataPath = ...,
    new_data_container_path=nx.DataPath("Porosity_Image_Resampled_Spacing"),
    remove_original_geometry=False,
    renumber_features=False,
    resampling_mode_index=0,
    #scaling: List[float] = ...,
    input_image_geometry_path=nx.DataPath("Porosity_Image"),
    spacing=[2.0, 2.0, 2.0]
)
nxtest.check_filter_result(nx_filter, result)


# Filter 3
# Instantiate Filter
nx_filter = nx.ResampleImageGeomFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    #exact_dimensions: List[int] = ...,
    #feature_ids_path: DataPath = ...,
    new_data_container_path=nx.DataPath("Porosity_Image_Resampled_Scaled"),
    remove_original_geometry=False,
    renumber_features=False,
    resampling_mode_index=1,
    scaling=[0.5, 0.5, 0.5],
    input_image_geometry_path=nx.DataPath("Porosity_Image")
    #spacing: List[float] = ...
)
# Error/Result Handling for Filter
nxtest.check_filter_result(nx_filter, result)


# Filter 4
# Instantiate Filter
nx_filter = nx.ResampleImageGeomFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix_path: DataPath = ...,
    exact_dimensions=[262, 195, 82],
    #feature_ids_path: DataPath = ...,
    new_data_container_path=nx.DataPath("Porosity_Image_Resampled_Exact_Dims"),
    remove_original_geometry=False,
    renumber_features=False,
    resampling_mode_index=2,
    #scaling: List[float] = ...,
    input_image_geometry_path=nx.DataPath("Porosity_Image")
    #spacing: List[float] = ...
)
# Error/Result Handling for Filter
nxtest.check_filter_result(nx_filter, result)


# Filter 5
# Define output file path
output_file_path = nxtest.get_data_directory() / "Output/ResamplePorosityImage.dream3d"
# Execute WriteDREAM3DFilter with Parameters
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************



print("===> Pipeline Complete")
