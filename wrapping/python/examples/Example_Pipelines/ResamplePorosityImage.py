import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# ------------------------------------------------------------------------------
# Create a DataArray that is as long as my CSV file (99 Rows in this case)
# ------------------------------------------------------------------------------
# Create a Data Structure

#Filter 1
data_structure = cx.DataStructure()

result = cxitk.ITKImportImageStack.execute(
    data_structure=data_structure,
    cell_data_name: str = ...,
    image_data_array_path: str = ...,
    image_geometry_path: DataPath = ...,
    image_transform_choice: int = ...,
    input_file_list_info: Any = ...,
    origin: List[float] = ...,
    spacing: List[float] = ...
)
#??