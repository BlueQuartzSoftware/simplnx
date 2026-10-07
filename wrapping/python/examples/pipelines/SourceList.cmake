#------------------------------------------------------------------------------
# These pipelines do not have any outside file dependencies other than what is
# already decompressed at this point on the CI or local machine. Each test will
# clean up its output file(s)
#
# !!! Group each plugin into its own little section as we re-use variables !!!
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Add the SIMPLNX Python Pipelines
#------------------------------------------------------------------------------
set(SimplnxDirPrefix "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/Simplnx")

set(SIMPLNX_PYTHON_TESTS  
  AppendImageGeometry
  ApplyTransformation_Demo
  ApplyTransformation_Image
  ApplyTransformation_Node
  ArrayCalculatorExample
  CombineSTLFiles
  EnsembleInfoReader
  Import_ASCII
  Import_CSV_File
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

#------------------------------------------------------------------------------
# Add the OrientationAnalysis Python Pipelines
#------------------------------------------------------------------------------
set(OrientationDirPrefix "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/OrientationAnalysis")

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

  AlignSectionsMutualInformation # Depends 01_Small_IN100_Archive
  ComputeLargestCrossSections # Depends 01_Small_IN100_Archive
  08_Small_IN100_Full_Reconstruction # Depends 01_Small_IN100_Archive

  ComputeBoundaryCells # Depends on 08_Small_IN100_Full_Reconstruction
  AvizoWriters # Depends on 08_Small_IN100_Full_Reconstruction
  VtkRectilinearGridWriter # Depends on 08_Small_IN100_Full_Reconstruction
  01_Small_IN100_Morphological_Statistics # Depends on 08_Small_IN100_Full_Reconstruction Will delete both input and output file

  05_Small_IN100_Crystallographic_Statistics

  ComputeBiasedFeatures # Depends 05_Small_IN100_Crystallographic_Statistics
  01_Small_IN100_Quick_Mesh
  02_Small_IN100_Smooth_Mesh
  10_Small_IN100_Triple_Lines # Must run before Mesh Statistics deletes the smoothed mesh
  03_Small_IN100_Mesh_Statistics

  ComputeGBCD-GBPDMetricBased # Depends 03_Small_IN100_Mesh_Statistics
  04_Small_IN100_GBCD # Depends 03_Small_IN100_Mesh_Statistics
  05_Small_IN100_GBCD_Metric # Depends 03_Small_IN100_Mesh_Statistics Will delete both input and output file
)

CreatePythonTests(
  PREFIX "PY::OrientationAnalysis"
  INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
  TEST_NAMES ${SIMPLNX_PYTHON_TESTS}
)


#------------------------------------------------------------------------------
# Add the ImageProcessing Python Pipelines
#------------------------------------------------------------------------------
set(ImageProcessingDirPrefix "${simplnx_SOURCE_DIR}/wrapping/python/examples/pipelines/ImageProcessing")

AddPythonTest(NAME "PY::ImageProcessing::02_Image_Segmentation" FILE "${ImageProcessingDirPrefix}/02_Image_Segmentation.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")
AddPythonTest(NAME "PY::ImageProcessing::03_Porosity_Mesh_Export" FILE "${ImageProcessingDirPrefix}/03_Porosity_Mesh_Export.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")

set(SIMPLNX_PYTHON_TESTS
  "05_AM_XCT_Porosity_Segmentation"
  "06_AM_Powder-Bed_Layer_Inspection"
  "07_AM_Melt-Pool_and_Track_Inspection"
  "08_Powder_Particle_Watershed_Segmentation"
  "09_Microstructure_Watershed_Segmentation"
  "10_Binary_Mask_Repair_and_Skeletonization"
  "11_Grayscale_Surface-Defect_Morphology"
  "12_Pore_Distance_and_Wall-Thickness_Metrology"
  "13_Denoising_and_Edge_Detection"
  "14_Projection-Based_Quality_Summaries"
  "15_Radiography_Intensity_Calibration"
  "16_Phase_and_Angle_Field_Transforms"
  "17_Scientific_Volume_Interoperability"
  "18_Industrial_XCT_Format_Import"
  "19_Serial-Section_Stack_Reconstruction"
  "20_Fiji_Microscopy_Montage_Import"
)
foreach(test ${SIMPLNX_PYTHON_TESTS})
  AddPythonTest(NAME "PY::ImageProcessing::${test}" FILE "${ImageProcessingDirPrefix}/${test}.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")
endforeach(test ${})

