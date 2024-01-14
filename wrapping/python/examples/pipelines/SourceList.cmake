

set(ItkDirPrefix "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/ITKImageProcessing")
set(OrientationDirPrefix "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/OrientationAnalysis")
set(SimplnxDirPrefix "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/Simplnx")

AddPythonTest(NAME "PY::ITKImageProcessing::02_Image_Segmentation" FILE "${ItkDirPrefix}/02_Image_Segmentation.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")
AddPythonTest(NAME "PY::ITKImageProcessing::03_Porosity_Mesh_Export" FILE "${ItkDirPrefix}/03_Porosity_Mesh_Export.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")

#------------------------------------------------------------------------------
# These pipelines do not have any outside file dependencies other than what is
# already decompressed at this point on the CI or local machine. Each test will
# clean up its output file(s)
#------------------------------------------------------------------------------
set(SIMPLNX_PYTHON_TESTS  
  AppendImageGeometryZSlice
  ApplyTransformation_Demo
  ApplyTransformation_Image
  ApplyTransformation_Node
  ArrayCalculatorExample
  CombineSTLFiles
  EnsembleInfoReader
  Import_ASCII
  Import_CSV_Data
  Import_STL_Model
  ReplaceElementAttributesWithNeighbor
  ResamplePorosityImage
  ResampleRectGridToImageGeom
  SurfaceNets_Demo
  Triangle_Face_Data_Demo
)

foreach(test ${SIMPLNX_PYTHON_TESTS})
  AddPythonTest(NAME "PY::Simplnx::${test}" FILE "${SimplnxDirPrefix}/${test}.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")
endforeach(test ${})

set(SIMPLNX_PYTHON_TESTS
  APTR12_Analysis
  AVTR12_Analysis
  CI_Histogram
  Edax_IPF_Colors
  ReadAng
  ReadCTF
  TxCopper_Exposed
  TxCopper_Unexposed
)
foreach(test ${SIMPLNX_PYTHON_TESTS})
  AddPythonTest(NAME "PY::OrientationAnalysis::${test}" FILE "${OrientationDirPrefix}/${test}.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")
endforeach(test ${})


#------------------------------------------------------------------------------
# Add the examples/pipelines/OrientationAnalysis folder
#------------------------------------------------------------------------------
set(PYTHON_TEST_INPUT_DIR "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/OrientationAnalysis")

set(SIMPLNX_PYTHON_TESTS
  01_Small_IN100_Archive

  AlignSectionsMutualInformation      # Depends 01_Small_IN100_Archive
  FindLargestCrossSections            # Depends 01_Small_IN100_Archive
  08_Small_IN100_Full_Reconstruction  # Depends 01_Small_IN100_Archive

  FindBoundaryCells                         # Depends on 08_Small_IN100_Full_Reconstruction
  AvizoWriters                              # Depends on 08_Small_IN100_Full_Reconstruction
  VtkRectilinearGridWriter                  # Depends on 08_Small_IN100_Full_Reconstruction
  01_Small_IN100_Morphological_Statistics   # Depends on 08_Small_IN100_Full_Reconstruction Will delete both input and output file

  05_Small_IN100_Crystallographic_Statistics

  FindBiasedFeatures            # Depends 05_Small_IN100_Crystallographic_Statistics
  01_Small_IN100_Quick_Mesh
  02_Small_IN100_Smooth_Mesh
  03_Small_IN100_Mesh_Statistics

  FindGBCD-GBPDMetricBased   # Depends 03_Small_IN100_Mesh_Statistics
  04_Small_IN100_GBCD        # Depends 03_Small_IN100_Mesh_Statistics 
  05_Small_IN100_GBCD_Metric # Depends 03_Small_IN100_Mesh_Statistics Will delete both input and output file
)

CreatePythonTests(
  PREFIX "PY::OrientationAnalysis"
  INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
  TEST_NAMES ${SIMPLNX_PYTHON_TESTS}
)

