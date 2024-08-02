#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
inline constexpr StringLiteral k_DataContainer("DataContainer2");

struct CompareDataArrayFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right, usize start = 0)
  {
    UnitTest::CompareDataArrays<T>(left, right, start);
  }
};

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  auto imageDimsArray = imageGeomDims.toArray();
  AttributeMatrix::ShapeType cellDataDims{imageDimsArray.crbegin(), imageDimsArray.crend()};
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellDataName, cellDataDims, imageGeom->getId());
  imageGeom->setCellData(*cellDataPtr);

  Int32Array* phases_data = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", cellDataDims, {1}, cellDataPtr->getId());

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Instantiate)", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{0, 0, 0};

  //  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  CropImageGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter Invalid Params", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  std::vector<uint64> k_MinVector{0, 0, 0};
  std::vector<uint64> k_MaxVector{500, 20, 30};

  //  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
  DataStructure dataStructure = CreateDataStructure();

  CropImageGeometryFilter filter;
  Arguments args;

  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // args.insertOrAssign(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insertOrAssign(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5553);

  k_MaxVector = {20, 500, 0};
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5554);

  k_MaxVector = {1, 1, 500};
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5555);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {1, 20, 20};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4011);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {20, 1, 20};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4012);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {20, 20, 1};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4013);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter)", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    REQUIRE(newDimensions[i] == (k_MaxVector[i] - k_MinVector[i] + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - XY", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 2; i++)
  {
    REQUIRE(newDimensions[i] == (k_MaxVector[i] - k_MinVector[i] + 1));
  }
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_XY_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - XZ", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == (k_MaxVector[0] - k_MinVector[0] + 1));
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == (k_MaxVector[2] - k_MinVector[2] + 1));

  DataPath exemplarGeoPath({"6_5_Cropped_XZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - YZ", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == (k_MaxVector[1] - k_MinVector[1] + 1));
  REQUIRE(newDimensions[2] == (k_MaxVector[2] - k_MinVector[2] + 1));

  DataPath exemplarGeoPath({"6_5_Cropped_YZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - X", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == (k_MaxVector[0] - k_MinVector[0] + 1));
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_X_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - Y", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == (k_MaxVector[1] - k_MinVector[1] + 1));
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_Y_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - Z", "[SimplnxCore][CropImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> ioResult = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == (k_MaxVector[2] - k_MinVector[2] + 1));

  DataPath exemplarGeoPath({"6_5_Cropped_Z_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[i] - origin[i]) / spacing[i]));
    REQUIRE(newDimensions[i] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop XY Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 2; i++)
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[i] - origin[i]) / spacing[i]));
    REQUIRE(newDimensions[i] == (max - min + 1));
  }
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_XY_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop XZ Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[0] - origin[0]) / spacing[0]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[0] - origin[0]) / spacing[0]));
    REQUIRE(newDimensions[0] == (max - min + 1));
  }
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[2] - origin[2]) / spacing[2]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[2] - origin[2]) / spacing[2]));
    REQUIRE(newDimensions[2] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_XZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop YZ Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  for(usize i = 1; i < 3; i++)
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[i] - origin[i]) / spacing[i]));
    REQUIRE(newDimensions[i] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_YZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop X Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[0] - origin[0]) / spacing[0]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[0] - origin[0]) / spacing[0]));
    REQUIRE(newDimensions[0] == (max - min + 1));
  }
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_X_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Y Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[1] - origin[1]) / spacing[1]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[1] - origin[1]) / spacing[1]));
    REQUIRE(newDimensions[1] == (max - min + 1));
  }
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_Y_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Z Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[2] - origin[2]) / spacing[2]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[2] - origin[2]) / spacing[2]));
    REQUIRE(newDimensions[2] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_Z_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}
