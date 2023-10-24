import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    #advanced_options: bool = ...,
    #data_format: str = ...,
    #delimiter_choice: int = ...,
    input_file=cx.DataPath("Data/ASCIIData/FeatureIds.csv"),
    n_comp=1,
    #n_skip_lines: int = ...,
    #n_tuples: List[List[float]] = ...,
    #output_data_array: DataPath = ...,
    scalar_type=4,
)
#Filter 2

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    #advanced_options: bool = ...,
    #data_format: str = ...,
    #delimiter_choice: int = ...,
    input_file=cx.DataPath("Data/ASCIIData/FeatureIds.csv"),
    n_comp=1,
    #n_skip_lines: int = ...,
    #n_tuples: List[List[float]] = ...,
    #output_data_array: DataPath = ...,
    scalar_type=4,
)