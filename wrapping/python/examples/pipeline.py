# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

data_structure = cx.DataStructure()

pipeline = cx.Pipeline()
pipeline.append(cx.CreateDataArray(), {'numeric_type': cx.NumericType.int32})
pipeline[0].set_args({'numeric_type': cx.NumericType.int32})

did_execute = pipeline.execute(data_structure)

print('Pipeline Execute: {}'.format(did_execute))

