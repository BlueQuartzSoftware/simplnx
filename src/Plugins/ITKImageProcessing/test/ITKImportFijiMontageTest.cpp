#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImportFijiMontageFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

using namespace complex;

namespace fs = std::filesystem;

namespace
{
const std::string k_SmallZeissZenDir = fmt::format("{}/fiji_montage_test/small_zeiss_zen", unit_test::k_TestFilesDir);
const fs::path k_SmallInputFile = fs::path(k_SmallZeissZenDir + "/" + "TileConfiguration.registered.txt");
const std::string k_MontageName = "Montage";
const DataPath k_MontagePath = {{k_MontageName}};
} // namespace

TEST_CASE("ITKImageProcessing::ITKImportFijiMontage: Basic 2x2 Grid Montage", "[ITKImageProcessing][ITKImportFijiMontage]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "fiji_montage_test.tar.gz", "fiji_montage_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/fiji_montage_test/small_zeiss_zen_montage.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(ITKImportFijiMontageFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_SmallInputFile));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_MontageName_Key, std::make_any<std::string>(k_MontageName));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_ColumnMontageLimits_Key, std::make_any<VectorInt32Parameter::ValueType>({0,1}));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_RowMontageLimits_Key, std::make_any<VectorInt32Parameter::ValueType>({0,1}));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(7));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_ChangeOrigin_Key, std::make_any<bool>(false));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_ConvertToGrayScale_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0f,0.0f,0.0f}));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_DataContainerPath_Key, std::make_any<std::string>(""));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cell AM"));
  args.insertOrAssign(ITKImportFijiMontageFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("data"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  UnitTest::CompareMontage(dataStructure.getDataRefAs<AbstractMontage>(k_MontagePath), exemplarDataStructure.getDataRefAs<AbstractMontage>(k_MontagePath));
}
