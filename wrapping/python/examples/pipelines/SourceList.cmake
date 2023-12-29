
#------------------------------------------------------------------------------
# Add the examples/Pipelines/Complex folder
#------------------------------------------------------------------------------
set(PYTHON_TEST_INPUT_DIR "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/Complex")

set(SIMPLNX_PYTHON_TESTS
  AlignSectionsMutualInformation
  AppendImageGeometryZSlice
  ApplyTransformation_Demo
  ApplyTransformation_Image
  ApplyTransformation_Node
  ArrayCalculatorExample
  AvizoWriters
  CombineSTLFiles
  EnsembleInfoReader
  FindBiasedFeatures
  FindBoundaryCells
  FindLargestCrossSections
  Import_ASCII
  Import_CSV_Data
  Import_STL_Model
  ReplaceElementAttributesWithNeighbor
  ResamplePorosityImage
  ResampleRectGridToImageGeom
  SurfaceNets_Demo
  Triangle_Face_Data_Demo
  VtkRectilinearGridWriter  
)

CreatePythonTests(PREFIX "PY::SimplnxCore"
  INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
  TEST_NAMES ${SIMPLNX_PYTHON_TESTS}
)



#------------------------------------------------------------------------------
# Add the examples/pipelines/ITKImageProcessing folder
#------------------------------------------------------------------------------
set(PYTHON_TEST_INPUT_DIR "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/ITKImageProcessing")

set(SIMPLNX_PYTHON_TESTS
  02_Image_Segmentation
  03_Porosity_Mesh_Export 
)

CreatePythonTests(PREFIX "PY::ITKImageProcessing"
  INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
  TEST_NAMES ${SIMPLNX_PYTHON_TESTS}
)



#------------------------------------------------------------------------------
# Add the examples/pipelines/OrientationAnalysis folder
#------------------------------------------------------------------------------
set(PYTHON_TEST_INPUT_DIR "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/OrientationAnalysis")

set(SIMPLNX_PYTHON_TESTS
  01_Small_IN100_Archive
  08_Small_IN100_Full_Reconstruction
  01_Small_IN100_Morphological_Statistics
  05_Small_IN100_Crystallographic_Statistics
  01_Small_IN100_Quick_Mesh
  02_Small_IN100_Smooth_Mesh
  03_Small_IN100_Mesh_Statistics
  04_Small_IN100_GBCD
  05_Small_IN100_GBCD_Metric
  APTR12_Analysis
  AVTR12_Analysis
  CI_Histogram
  Edax_IPF_Colors
  FindGBCD-GBPDMetricBased
  ReadAng
  ReadCTF
  TxCopper_Exposed
  TxCopper_Unexposed
)

CreatePythonTests(PREFIX "PY::OrientationAnalysis"
  INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
  TEST_NAMES ${SIMPLNX_PYTHON_TESTS}
)



