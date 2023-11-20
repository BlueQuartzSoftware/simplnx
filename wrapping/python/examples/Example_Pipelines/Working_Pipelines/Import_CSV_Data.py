import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.CreateImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    dimensions=[60, 80, 100],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[100.0, 100.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/ASCIIData/ConfidenceIndex.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/Confidence Index"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/ASCIIData/FeatureIds.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/FeatureIds"),
    scalar_type=cx.NumericType.int32
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/ASCIIData/ImageQuality.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/Image Quality"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/ASCIIData/IPFColor.csv",
    n_comp=3,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("[Image Geometry]/Cell Data/IPFColors"),
    scalar_type=cx.NumericType.uint8
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")