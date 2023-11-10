import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.EbsdToH5EbsdFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    input_file_list_info=cx.DataPath("Data/Small_IN100"),
    output_file_path=cx.DataPath("Data/Output/Reconstruction/Small_IN100.h5ebsd"),
    reference_frame=0,
    stacking_order=1,
    z_spacing=0.25
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")
