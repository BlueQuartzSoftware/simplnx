import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.EbsdToH5EbsdFilter.execute(
    data_structure=data_structure,
    input_file_list_info=cx.DataPath("Data/Small_IN100"),
    output_file_path=cx.DataPath("Data/Output/Reconstruction/Small_IN100.h5ebsd"),
    reference_frame=0,
    stacking_order=1,
    z_spacing=0.25
)