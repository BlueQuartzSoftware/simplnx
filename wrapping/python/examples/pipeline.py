# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

data_structure = cx.DataStructure()

pipeline = cx.Pipeline().from_file("/Users/mjackson/Workspace1/complex/src/Plugins/OrientationAnalysis/pipelines/EBSD Reconstruction/(01) Small IN100 Archive.d3dpipeline")

pipeline.to_file( "test pipeline", "/tmp/python_pipeline.d3dpipeline")



# pipeline.append(cx.CreateDataArray(), {'numeric_type': cx.NumericType.int32})
# pipeline[0].set_args({'numeric_type': cx.NumericType.int32})

# did_execute = pipeline.execute(data_structure)

# print('Pipeline Execute: {}'.format(did_execute))


