#include "OrientationAnalysis/Filters/Algorithms/EbsdToH5Ebsd.hpp"
#include "OrientationAnalysis/Filters/EbsdToH5EbsdFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include <catch2/catch.hpp>
#include <hdf5.h>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace H5Support;

namespace
{

std::string s_OutputFilePath;

const herr_t k_GroupDoesNotExist = -5;
const herr_t k_DatasetDoesNotExist = -10;

herr_t ValidateGroupPath(const char* groupPath)
{
  UNSCOPED_INFO(fmt::format("Validating Group: /{}", groupPath));
  hid_t outFileId = H5Utilities::openFile(s_OutputFilePath, true);
  H5ScopedFileSentinel scopedFileSentinel(outFileId, true);

  return (H5Utilities::objectExists(outFileId, groupPath) ? 1 : k_GroupDoesNotExist);
}

herr_t ValidateDataset(hid_t exemplarFileId, const char* datasetPath)
{
  UNSCOPED_INFO(fmt::format("Validating Dataset: /{}", datasetPath));

  hid_t outFileId = H5Utilities::openFile(s_OutputFilePath, true);
  H5ScopedFileSentinel scopedFileSentinel(outFileId, true);

  if(!H5Utilities::objectExists(outFileId, datasetPath))
  {
    return k_DatasetDoesNotExist;
  }

  std::vector<hsize_t> dims;
  H5T_class_t classType;
  size_t sizeType;
  herr_t err = H5Lite::getDatasetInfo(exemplarFileId, datasetPath, dims, classType, sizeType);
  size_t totalElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<size_t>(1), std::multiplies<>());

  switch(classType)
  {
  case H5T_INTEGER:
    if(sizeType == 1)
    {
      std::vector<uint8_t> exemplarData(totalElements);
      err = H5Lite::readPointerDataset(exemplarFileId, datasetPath, exemplarData.data());
      REQUIRE(err >= 0);
      std::vector<uint8_t> generatedData(totalElements);
      err = H5Lite::readPointerDataset(outFileId, datasetPath, generatedData.data());
      REQUIRE(err >= 0);
      REQUIRE(exemplarData == generatedData);
    }
    else if(sizeType == 2)
    {
      std::vector<uint16_t> exemplarData(totalElements);
      err = H5Lite::readPointerDataset(exemplarFileId, datasetPath, exemplarData.data());
      REQUIRE(err >= 0);
      std::vector<uint16_t> generatedData(totalElements);
      err = H5Lite::readPointerDataset(outFileId, datasetPath, generatedData.data());
      REQUIRE(err >= 0);
      REQUIRE(exemplarData == generatedData);
    }
    else if(sizeType == 4)
    {
      std::vector<uint32_t> exemplarData(totalElements);
      err = H5Lite::readPointerDataset(exemplarFileId, datasetPath, exemplarData.data());
      REQUIRE(err >= 0);
      std::vector<uint32_t> generatedData(totalElements);
      err = H5Lite::readPointerDataset(outFileId, datasetPath, generatedData.data());
      REQUIRE(err >= 0);
      REQUIRE(exemplarData == generatedData);
    }
    else if(sizeType == 8)
    {
      std::vector<uint64_t> exemplarData(totalElements);
      err = H5Lite::readPointerDataset(exemplarFileId, datasetPath, exemplarData.data());
      REQUIRE(err >= 0);
      std::vector<uint64_t> generatedData(totalElements);
      err = H5Lite::readPointerDataset(outFileId, datasetPath, generatedData.data());
      REQUIRE(err >= 0);
      REQUIRE(exemplarData == generatedData);
    }
    break;
  case H5T_FLOAT:
    if(sizeType == 4)
    {
      std::vector<float32> exemplarData(totalElements);
      err = H5Lite::readPointerDataset(exemplarFileId, datasetPath, exemplarData.data());
      REQUIRE(err >= 0);
      std::vector<float32> generatedData(totalElements);
      err = H5Lite::readPointerDataset(outFileId, datasetPath, generatedData.data());
      REQUIRE(err >= 0);
      REQUIRE(exemplarData == generatedData);
    }
    else if(sizeType == 8)
    {
      std::vector<float64> exemplarData(totalElements);
      err = H5Lite::readPointerDataset(exemplarFileId, datasetPath, exemplarData.data());
      REQUIRE(err >= 0);
      std::vector<float64> generatedData(totalElements);
      err = H5Lite::readPointerDataset(outFileId, datasetPath, generatedData.data());
      REQUIRE(err >= 0);
      REQUIRE(exemplarData == generatedData);
    }
  default:
    break;
  }

  return err;
}

/************************************************************
  Operator function for H5Ovisit.  This function prints the
  name and type of the object passed to it.
 ************************************************************/
herr_t VisitorFunction(hid_t locId, const char* name, const H5O_info_t* info, void* operatorData)
{
  herr_t status;

  switch(info->type)
  {
  case H5O_TYPE_GROUP:
    status = ValidateGroupPath(name);
    REQUIRE(status >= 1);
    break;
  case H5O_TYPE_DATASET:
    status = ValidateDataset(locId, name);
    REQUIRE(status >= 0);
    break;
  case H5O_TYPE_NAMED_DATATYPE:
  default:
    break;
  }
  return 0;
}

/**
 * @brief
 * @param fileName
 * @return
 */
int TraverseFile(const std::string& fileName)
{
  /*
   * Open file
   */
  hid_t file = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  REQUIRE(file > 0);
  /*
   * Begin iteration using H5Ovisit
   */
  herr_t status = H5Ovisit(file, H5_INDEX_NAME, H5_ITER_NATIVE, VisitorFunction, nullptr);
  REQUIRE(status >= 0);

  /*
   * Close and release resources.
   */
  status = H5Fclose(file);
  REQUIRE(status >= 0);

  return 0;
}

const std::string k_ExemplarFilePath = fmt::format("{}/6_5_h5ebsd_exemplar/6_5_h5ebsd_exemplar.h5ebsd", unit_test::k_TestFilesDir);

const auto k_HighToLow = static_cast<ChoicesParameter::ValueType>(complex::FilePathGenerator::Ordering::HighToLow);
const int32 k_StartIndex = 1;
const int32 k_EndIndex = 2;
const int32 k_IncrementIndex = 1;
const uint32 k_PaddingDigits = 1;
const GeneratedFileListParameter::Ordering k_Ordering = GeneratedFileListParameter::Ordering::LowToHigh;
const std::string k_InputPath = fmt::format("{}/Small_IN100", unit_test::k_TestFilesDir);
const std::string k_FilePrefix = "Slice_";
const std::string k_FileSuffix;
const std::string k_FileExtension = ".ang";
GeneratedFileListParameter::ValueType k_FileListInfo = {k_StartIndex, k_EndIndex, k_IncrementIndex, k_PaddingDigits, k_Ordering, k_InputPath, k_FilePrefix, k_FileSuffix, k_FileExtension};

} // namespace

TEST_CASE("OrientationAnalysis::EbsdToH5Ebsd", "[OrientationAnalysis][EbsdToH5Ebsd]")
{
  const std::string kDataInputArchive = "Small_IN100.tar.gz";
  const std::string kExpectedOutputTopLevel = "Small_IN100";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);
  const std::string kDataInputArchive1 = "6_5_h5ebsd_exemplar.tar.gz";
  const std::string kExpectedOutputTopLevel1 = "6_5_h5ebsd_exemplar";
  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive1, kExpectedOutputTopLevel1,
                                                              complex::unit_test::k_BinaryTestOutputDir);

  fs::path inputPath(k_InputPath);
  REQUIRE(fs::exists(inputPath));

  // Set the global variable, so we have access to it later
  s_OutputFilePath = fmt::format("{}/ebsd_to_h5ebsd_test.h5ebsd", unit_test::k_BinaryTestOutputDir);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  EbsdToH5EbsdFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  // Create default Parameters for the filter.
  args.insertOrAssign(EbsdToH5EbsdFilter::k_ZSpacing_Key, std::make_any<Float32Parameter::ValueType>(0.25F));
  args.insertOrAssign(EbsdToH5EbsdFilter::k_StackingOrder_Key, std::make_any<ChoicesParameter::ValueType>(k_HighToLow));
  args.insertOrAssign(EbsdToH5EbsdFilter::k_ReferenceFrame_Key, std::make_any<ChoicesParameter::ValueType>(complex::EbsdToH5EbsdInputConstants::k_Edax));
  args.insertOrAssign(EbsdToH5EbsdFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(s_OutputFilePath)));
  args.insertOrAssign(EbsdToH5EbsdFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  ::TraverseFile(k_ExemplarFilePath);
}
