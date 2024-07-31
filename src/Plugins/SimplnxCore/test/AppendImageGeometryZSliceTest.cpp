#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/AppendImageGeometryZSliceFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/RenameDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_CroppedBottomHalfPath({"CroppedBottomHalf"});
const DataPath k_CroppedTopHalfPath({"CroppedTopHalf"});
const DataPath k_AppendedGeometryPath({"AppendedGeometry"});
const DataPath k_InvalidTestGeometryPath1({"Image2dDataContainer"});
const DataPath k_InvalidTestGeometryPath2({"Resampled_2D_ImageGeom"});
const DataPath k_InvalidTestGeometryPath3({"Resampled_3D_ImageGeom"});
const DataPath k_SmallIN100Path({k_SmallIN100});

} // namespace

TEST_CASE("SimplnxCore::AppendImageGeometryZSliceFilter: Valid Filter Execution", "[SimplnxCore][AppendImageGeometryZSliceFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");
  // Read in starting/exemplar image geometry
  const auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // crop input geometry down to bottom half & save as new geometry
  {
    CropImageGeometryFilter filter;
    Arguments args;

    args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 0}));
    args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{188, 200, 50}));
    //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(false));
    args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_CroppedBottomHalfPath));
    args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  // crop input geometry down to top half & save as new geometry
  {
    CropImageGeometryFilter filter;
    Arguments args;

    args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 51}));
    args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{188, 200, 116}));
    //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
    args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
    args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  // Create a neighborlist and string array for the original input geometry and manually divide them up to the two cropped halves since CropImageGeometryFilter only supports IDataArrays
  {
    const auto& cellDataAM = dataStructure.getDataRefAs<AttributeMatrix>(k_DataContainerPath.createChildPath(k_CellData));
    const usize numTuples = cellDataAM.getNumTuples();

    std::vector<std::string> stringArrayValues(numTuples);
    auto* neighborList = NeighborList<float32>::Create(dataStructure, "NeighborList", numTuples, cellDataAM.getId());
    neighborList->resizeTotalElements(numTuples);
    for(usize i = 0; i < numTuples; ++i)
    {
      const float32 factor = static_cast<float32>(i) * 0.001;
      std::vector<float32> list = {factor * 117, factor * 875, factor * 1035, factor * 3905, factor * 4214};
      neighborList->setList(i, std::make_shared<std::vector<float32>>(list));
      stringArrayValues[i] = "String_" + StringUtilities::number(factor);
    }

    const auto* stringArray = StringArray::CreateWithValues(dataStructure, "StringArray", stringArrayValues, cellDataAM.getId());

    const auto& croppedTopHalfAM = dataStructure.getDataRefAs<AttributeMatrix>(k_CroppedTopHalfPath.createChildPath(k_CellData));
    const auto& croppedBottomHalfAM = dataStructure.getDataRefAs<AttributeMatrix>(k_CroppedBottomHalfPath.createChildPath(k_CellData));
    const auto numTuplesTop = croppedTopHalfAM.getNumTuples();
    const auto numTuplesBottom = croppedBottomHalfAM.getNumTuples();
    REQUIRE(numTuplesTop + numTuplesBottom == numTuples);

    auto* neighborListBottom = NeighborList<float32>::Create(dataStructure, "NeighborList", numTuplesBottom, croppedBottomHalfAM.getId());
    neighborListBottom->resizeTotalElements(numTuplesBottom);
    std::vector<std::string> stringValuesBottom(numTuplesBottom);
    for(usize i = 0; i < numTuplesBottom; ++i)
    {
      std::vector<float32> list = neighborList->copyOfList(i);
      neighborListBottom->setList(i, std::make_shared<std::vector<float32>>(list));
      std::string strVal = stringArray->at(i);
      stringValuesBottom[i] = strVal;
    }
    const auto* stringArrayBottom = StringArray::CreateWithValues(dataStructure, "StringArray", stringValuesBottom, croppedBottomHalfAM.getId());
    auto* neighborListTop = NeighborList<float32>::Create(dataStructure, "NeighborList", numTuplesTop, croppedTopHalfAM.getId());
    neighborListTop->resizeTotalElements(numTuplesTop);
    std::vector<std::string> stringValueTop(numTuplesTop);
    for(usize i = 0; i < numTuplesTop; ++i)
    {
      usize adjustedIndex = numTuplesBottom + i;
      std::vector<float32> list = neighborList->copyOfList(adjustedIndex);
      neighborListTop->setList(i, std::make_shared<std::vector<float32>>(list));
      std::string strVal = stringArray->at(adjustedIndex);
      stringValueTop[i] = strVal;
    }
    const auto* stringArrayTop = StringArray::CreateWithValues(dataStructure, "StringArray", stringValueTop, croppedTopHalfAM.getId());
  }
  // Append the 2 cropped geometries together and compare with the original input geometry
  {
    AppendImageGeometryZSliceFilter filter;
    Arguments args;

    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_InputGeometries_Key, std::make_any<std::vector<DataPath>>({k_CroppedTopHalfPath}));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(k_CroppedBottomHalfPath));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_CheckResolution_Key, std::make_any<bool>(true));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_SaveAsNewGeometry_Key, std::make_any<bool>(true));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_NewGeometry_Key, std::make_any<DataPath>(k_AppendedGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // rename the cell data attribute matrix for easier comparison with exemplar data
    RenameDataObjectFilter renameFilter;
    Arguments renameArgs;
    renameArgs.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_CellData));
    renameArgs.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_AppendedGeometryPath.createChildPath(ImageGeom::k_CellDataName)));
    auto renameResult = renameFilter.execute(dataStructure, renameArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(renameResult.result)

    const DataPath appendedCellDataPath = k_AppendedGeometryPath.createChildPath(k_CellData);
    const usize numAppendedArrays = dataStructure.getDataRefAs<AttributeMatrix>(appendedCellDataPath).getSize();
    REQUIRE(numAppendedArrays == 8);

    CompareExemplarToGeneratedData(dataStructure, dataStructure, appendedCellDataPath, k_DataContainer);
  }
}

TEST_CASE("SimplnxCore::AppendImageGeometryZSliceFilter: InValid Filter Execution", "[SimplnxCore][AppendImageGeometryZSliceFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                              "ResampleImageGeom_Exemplar.dream3d");

  // Read in test image geometries
  auto geomFilePath = fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(geomFilePath);

  AppendImageGeometryZSliceFilter filter;
  Arguments args;
  args.insertOrAssign(AppendImageGeometryZSliceFilter::k_CheckResolution_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryZSliceFilter::k_SaveAsNewGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryZSliceFilter::k_NewGeometry_Key, std::make_any<DataPath>(k_AppendedGeometryPath));

  SECTION("Mismatching Dimensions")
  {
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_InputGeometries_Key, std::make_any<std::vector<DataPath>>({k_InvalidTestGeometryPath3}));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(DataPath({k_SmallIN100})));
  }
  SECTION("Mismatching Spacing")
  {
    // crop one of the invalid test geometries to get a second geometry with the same dimensions but different spacing
    {
      CropImageGeometryFilter cropFilter;
      Arguments cropArgs;

      cropArgs.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 0}));
      cropArgs.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{10, 10, 0}));
      //   cropArgs.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
      cropArgs.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InvalidTestGeometryPath2));
      cropArgs.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
      cropArgs.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
      cropArgs.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      cropArgs.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

      auto cropResult = cropFilter.execute(dataStructure, cropArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(cropResult.result)
    }
    {
      args.insertOrAssign(AppendImageGeometryZSliceFilter::k_InputGeometries_Key, std::make_any<std::vector<DataPath>>({k_InvalidTestGeometryPath1}));
      args.insertOrAssign(AppendImageGeometryZSliceFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
    }
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
