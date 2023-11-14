import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cx.ReadTextDataArrayFilter.execute(
    data_structure=data_structure,
    #advanced_options: bool = ...,
    #data_format: str = ...,
    #delimiter_choice: int = ...,
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/ASCIIData/FeatureIds.csv",
    n_comp=1,
    #n_skip_lines: int = ...,
    #n_tuples: List[List[float]] = ...,
    #output_data_array: DataPath = ...,
    scalar_type=4
)

#Filter 2

result = cx.ReadCSVData.execute(
    data_structure=data_structure,
    #advanced_options: bool = ...,
    #data_format: str = ...,
    #delimiter_choice: int = ...,
    input_file=cx.DataPath("Data/ASCIIData/FeatureIds.csv"),
    n_comp=1,
    #n_skip_lines: int = ...,
    #n_tuples: List[List[float]] = ...,
    #output_data_array: DataPath = ...,
    scalar_type=4
)


#This filter is currently under construction and will be updated at a future time