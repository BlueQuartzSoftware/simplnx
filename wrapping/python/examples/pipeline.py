# Import the DREAM3D Base library and Plugins
import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

data_structure = nx.DataStructure()

pipeline = cx.Pipeline().from_file(cxtest.GetComplexSourceDir() + '/src/Plugins/OrientationAnalysis/pipelines/EBSD Reconstruction/(01) Small IN100 Archive.d3dpipeline')

pipeline.to_file( "test pipeline", cxtest.GetTestTempDirectory() + "/python_pipeline.d3dpipeline")



# pipeline.append(nx.CreateDataArray(), {'numeric_type': nx.NumericType.int32})
# pipeline[0].set_args({'numeric_type': nx.NumericType.int32})

# did_execute = pipeline.execute(data_structure)

# print('Pipeline Execute: {}'.format(did_execute))


