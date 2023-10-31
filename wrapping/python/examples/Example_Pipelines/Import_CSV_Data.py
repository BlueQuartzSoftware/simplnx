import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cx.CreateImageGeometry.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    dimensions=[60, 80, 100],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[100.0, 100.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)

#Filter 2

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/ConfidenceIndex.csv"),
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("Confidence Index"),
    scalar_type=8
)

#Filter 3

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/FeatureIds.csv"),
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("FeatureIds"),
    scalar_type=4
)

#Filter 4

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/ImageQuality.csv"),
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("Image Quality"),
    scalar_type=8
)

#Filter 5

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file=cx.DataPath("Data/ASCIIData/IPFColor.csv"),
    n_comp=3,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("IPFColors"),
    scalar_type=1
)