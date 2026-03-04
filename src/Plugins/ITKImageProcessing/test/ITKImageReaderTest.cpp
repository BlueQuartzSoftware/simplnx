#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/ReadImageUtils.hpp"
#include "ITKImageProcessing/Filters/ITKImageReaderFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const std::string k_TestDataDirName = "itk_image_reader_test";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_ExemplarFile = k_TestDataDir / "itk_image_reader_test.dream3d";
const fs::path k_InputImageFile = k_TestDataDir / "200x200_0.tif";
const std::string k_ImageDataName = "ImageData";
} // namespace

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Read_Basic", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Read_Basic"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Read_Basic"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Read_Basic", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Override_Origin", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float64> k_Origin{-32.0, -32.0, 0.0};

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Origin"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Origin"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Override_Origin", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Centering_Origin", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_CenterOrigin_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Centering_Origin"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Centering_Origin"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Centering_Origin", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Cropping", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  // This block generates every combination of croppingOptions, changeOrigin, and changeSpacing and then the entire test executes for each combination
  std::vector<float64> spacing = {2.0, 2.0, 1.0};
  std::vector<float64> origin = {20.0, 30.0, 0.0};
  UnitTest::Cropping::AxisBoundsChoices bounds;
  bounds.voxelX = {IntVec2Type{50, 149}};
  bounds.voxelY = {IntVec2Type{50, 149}};
  bounds.physX = {FloatVec2Type{120.0f, 318.0f}};
  bounds.physY = {FloatVec2Type{130.0f, 328.0f}};
  auto allCropVals = UnitTest::Cropping::GenerateAllCropValues(bounds, true);
  auto croppingOptions = GENERATE_COPY(from_range(allCropVals));

  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  static std::atomic<int> geomCounter{1};
  const int myId = geomCounter.fetch_add(1);

  const std::string sectionName = fmt::format("ImageGeometry {:0>3} (CroppingOptions=[{}, {}, {}])", myId, UnitTest::Cropping::CropTypeToString(croppingOptions.type),
                                              UnitTest::Cropping::BoolToString(croppingOptions.cropX), UnitTest::Cropping::BoolToString(croppingOptions.cropY));
  const std::string computedGeomName = "ImageGeometry";

  DYNAMIC_SECTION(sectionName)
  {
    ITKImageReaderFilter filter;
    Arguments args;

    args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
    args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, DataPath({computedGeomName}));
    args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
    args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
    args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);
    args.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, origin);
    args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, true);
    args.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, spacing);
    args.insertOrAssign(ITKImageReaderFilter::k_CroppingOptions_Key, croppingOptions);
    args.insertOrAssign(ITKImageReaderFilter::k_OriginSpacingProcessing_Key, to_underlying(cxItkImageReaderFilter::OriginSpacingProcessingTiming::Preprocessed));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    if(executeResult.result.invalid())
    {
      INFO([&] {
        std::ostringstream oss;
        oss << "Errors for combo '" << sectionName << "':\n";
        for(const auto& e : executeResult.result.errors())
          oss << "  - " << e.code << " : " << e.message << "\n";
        return oss.str();
      }());
    }
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Compare against exemplar
    DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({computedGeomName})));
    const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({computedGeomName}));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({sectionName})));
    const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({sectionName}));
    UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

    DataPath generatedDataPath = DataPath({computedGeomName, Constants::k_Cell_Data, k_ImageDataName});
    DataPath exemplarDataPath = DataPath({sectionName, Constants::k_Cell_Data, k_ImageDataName});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
    const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
    const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
    UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Override_Spacing", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float64> k_Spacing{2.5, 3.0, 1.0};

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, k_Spacing);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Spacing"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Spacing"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Override_Spacing", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: OriginSpacing_Preprocessed", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float64> k_Origin{10.0, 20.0, 0.0};
  std::vector<float64> k_Spacing{2.0, 2.0, 1.0};

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, k_Spacing);
  args.insertOrAssign(ITKImageReaderFilter::k_OriginSpacingProcessing_Key, to_underlying(cxItkImageReaderFilter::OriginSpacingProcessingTiming::Preprocessed));
  args.insertOrAssign(ITKImageReaderFilter::k_CroppingOptions_Key, cropOptions);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Preprocessed"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Preprocessed"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Preprocessed", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: OriginSpacing_Postprocessed", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float64> k_Origin{10.0, 20.0, 0.0};
  std::vector<float64> k_Spacing{2.0, 2.0, 1.0};
  const uint64_t k_Postprocessed = 1;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, k_Spacing);
  args.insertOrAssign(ITKImageReaderFilter::k_OriginSpacingProcessing_Key, k_Postprocessed);
  args.insertOrAssign(ITKImageReaderFilter::k_CroppingOptions_Key, cropOptions);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Postprocessed"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Postprocessed"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Postprocessed", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: DataType_Conversion", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  const uint64_t k_DataTypeUInt16 = 1;

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeDataType_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataType_Key, k_DataTypeUInt16);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"DataType_Conversion"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"DataType_Conversion"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"DataType_Conversion", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint16>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Interaction_Crop_DataType", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  const uint64_t k_DataTypeUInt32 = 2;

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_CroppingOptions_Key, cropOptions);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeDataType_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataType_Key, k_DataTypeUInt32);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_DataType"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_DataType"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Interaction_Crop_DataType", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint32>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Interaction_All", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;
  std::vector<float64> k_Origin{5.0, 10.0, 0.0};
  std::vector<float64> k_Spacing{2.0, 2.0, 1.0};
  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};
  const uint64_t k_DataTypeUInt16 = 1;
  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, k_Spacing);
  args.insertOrAssign(ITKImageReaderFilter::k_OriginSpacingProcessing_Key, to_underlying(cxItkImageReaderFilter::OriginSpacingProcessingTiming::Preprocessed));
  args.insertOrAssign(ITKImageReaderFilter::k_CroppingOptions_Key, cropOptions);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeDataType_Key, true);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataType_Key, k_DataTypeUInt16);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_OriginSpacing_Preprocessed_DataType"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_OriginSpacing_Preprocessed_DataType"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);
  DataPath generatedDataPath = inputGeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Interaction_Crop_OriginSpacing_Preprocessed_DataType", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<AbstractDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<AbstractDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint16>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
