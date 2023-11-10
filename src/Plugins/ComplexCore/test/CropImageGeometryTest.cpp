#include "ComplexCore/Filters/CropImageGeometry.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <catch2/catch.hpp>

using namespace complex;

namespace
{

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

TEST_CASE("ComplexCore::CropImageGeometry(Instantiate)", "[ComplexCore][CropImageGeometry]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{0, 0, 0};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  CropImageGeometry filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CropImageGeometry Invalid Params", "[ComplexCore][CropImageGeometry]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  std::vector<uint64> k_MinVector{0, 0, 0};
  std::vector<uint64> k_MaxVector{500, 20, 30};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
  DataStructure dataStructure = CreateDataStructure();

  CropImageGeometry filter;
  Arguments args;

  args.insertOrAssign(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insertOrAssign(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insertOrAssign(CropImageGeometry::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(CropImageGeometry::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insertOrAssign(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insertOrAssign(CropImageGeometry::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5553);

  k_MaxVector = {20, 500, 0};
  args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5554);

  k_MaxVector = {1, 1, 500};
  args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5555);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {1, 20, 20};
  args.insertOrAssign(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5550);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {20, 1, 20};
  args.insertOrAssign(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5551);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {20, 20, 1};
  args.insertOrAssign(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5552);
}

TEST_CASE("ComplexCore::CropImageGeometry(Execute_Filter)", "[ComplexCore][CropImageGeometry]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({Constants::k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_5_test_data_1.tar.gz", "6_5_test_data_1");

  CropImageGeometry filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1/6_5_test_data_1.dream3d", complex::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometry::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

  const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<complex::HDF5::FileWriter> ioResult = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    complex::HDF5::FileWriter fileWriter = std::move(ioResult.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    COMPLEX_RESULT_REQUIRE_VALID(resultH5);
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
