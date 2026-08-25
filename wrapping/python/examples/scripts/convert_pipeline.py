import simplnx as nx
import itkimageprocessing as nxitk
import orientationanalysis as nxor
from pipeline_to_python import create_default_generator

pipeline = nx.Pipeline.from_file("/Users/mjackson/Workspace1/simplnx/src/Plugins/OrientationAnalysis/pipelines/Small_IN100_Processing/(02) Small IN100 Full Reconstruction.d3dpipeline")
generator = create_default_generator()
print(generator.generate(pipeline))