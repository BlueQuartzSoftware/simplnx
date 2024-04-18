import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = nx.CreateImageGeometry()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    dimensions=[60, 80, 100],
    output_image_geometry_path=nx.DataPath("[Image Geometry]"),
    origin=[100.0, 100.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    set_tuple_dimensions=False,
    data_format="",
    delimiter_index=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/ConfidenceIndex.csv",
    number_comp=1,
    skip_line_count=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array_path=nx.DataPath("[Image Geometry]/Cell Data/Confidence Index"),
    scalar_type_index=nx.NumericType.float32
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    set_tuple_dimensions=False,
    data_format="",
    delimiter_index=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/FeatureIds.csv",
    number_comp=1,
    skip_line_count=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array_path=nx.DataPath("[Image Geometry]/Cell Data/FeatureIds"),
    scalar_type_index=nx.NumericType.int32
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    set_tuple_dimensions=False,
    data_format="",
    delimiter_index=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/ImageQuality.csv",
    number_comp=1,
    skip_line_count=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array_path=nx.DataPath("[Image Geometry]/Cell Data/Image Quality"),
    scalar_type_index=nx.NumericType.float32
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    set_tuple_dimensions=False,
    data_format="",
    delimiter_index=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/IPFColor.csv",
    number_comp=3,
    skip_line_count=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array_path=nx.DataPath("[Image Geometry]/Cell Data/IPFColors"),
    scalar_type_index=nx.NumericType.uint8
)
nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")
