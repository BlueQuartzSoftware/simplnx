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
    geometry_data_path=nx.DataPath("[Image Geometry]"),
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
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/ConfidenceIndex.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("[Image Geometry]/Cell Data/Confidence Index"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/FeatureIds.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("[Image Geometry]/Cell Data/FeatureIds"),
    scalar_type=nx.NumericType.int32
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/ImageQuality.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("[Image Geometry]/Cell Data/Image Quality"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.get_data_directory() / "ASCIIData/IPFColor.csv",
    n_comp=3,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("[Image Geometry]/Cell Data/IPFColors"),
    scalar_type=nx.NumericType.uint8
)
nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")
