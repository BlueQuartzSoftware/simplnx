#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include <simplnx/Utilities/DataArrayUtilities.hpp>

#include "SimplnxCore/Filters/AppendImageGeometryFilter.hpp"
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
const DataPath k_CroppedBottomZPath({"CroppedBottomZ"});
const DataPath k_CroppedMiddleZPath({"CroppedMiddleZ"});
const DataPath k_CroppedTopZPath({"CroppedTopZ"});
const DataPath k_CroppedBottomXPath({"CroppedBottomX"});
const DataPath k_CroppedMiddleXPath({"CroppedMiddleX"});
const DataPath k_CroppedTopXPath({"CroppedTopX"});
const DataPath k_CroppedBottomYPath({"CroppedBottomY"});
const DataPath k_CroppedMiddleYPath({"CroppedMiddleY"});
const DataPath k_CroppedTopYPath({"CroppedTopY"});
const DataPath k_AppendedGeometryPath({"AppendedGeometry"});
const DataPath k_InvalidTestGeometryPath1({"Image2dDataContainer"});
const DataPath k_InvalidTestGeometryPath2({"Resampled_2D_ImageGeom"});
const DataPath k_InvalidTestGeometryPath3({"Resampled_3D_ImageGeom"});
const DataPath k_SmallIN100Path({k_SmallIN100});

void cropGeometry(DataStructure& dataStructure, const DataPath& selectedPath, const DataPath& createdPath, const std::vector<uint64>& minVoxel, const std::vector<uint64>& maxVoxel)
{
  CropImageGeometryFilter filter;
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(minVoxel));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(maxVoxel));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(selectedPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(createdPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

void createNeighborListsAndStringArrays(DataStructure& dataStructure, const std::vector<uint64>& minBottomVoxel, const std::vector<uint64>& maxBottomVoxel, const std::vector<uint64>& minMiddleVoxel,
                                        const std::vector<uint64>& maxMiddleVoxel, const std::vector<uint64>& minTopVoxel, const std::vector<uint64>& maxTopVoxel, const DataPath& bottomPath,
                                        const DataPath& middlePath, const DataPath& topPath)
{
  // Create a neighbor list and string array for the original input geometry and manually divide them up to the two cropped halves since CropImageGeometryFilter only supports IDataArrays
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

  const auto& croppedTopAM = dataStructure.getDataRefAs<AttributeMatrix>(topPath.createChildPath(k_CellData));
  const auto& croppedMiddleAM = dataStructure.getDataRefAs<AttributeMatrix>(middlePath.createChildPath(k_CellData));
  const auto& croppedBottomAM = dataStructure.getDataRefAs<AttributeMatrix>(bottomPath.createChildPath(k_CellData));
  const auto numTuplesTop = croppedTopAM.getNumTuples();
  const auto numTuplesMiddle = croppedMiddleAM.getNumTuples();
  const auto numTuplesBottom = croppedBottomAM.getNumTuples();
  REQUIRE(numTuplesTop + numTuplesMiddle + numTuplesBottom == numTuples);

  usize xCount = maxTopVoxel[0] - minBottomVoxel[0] + 1;
  usize yCount = maxTopVoxel[1] - minBottomVoxel[1] + 1;

  auto* neighborListBottom = NeighborList<float32>::Create(dataStructure, "NeighborList", numTuplesBottom, croppedBottomAM.getId());
  neighborListBottom->resizeTotalElements(numTuplesBottom);
  std::vector<std::string> stringValuesBottom(numTuplesBottom);
  usize i = 0;
  for(usize z = minBottomVoxel[2]; z <= maxBottomVoxel[2]; ++z)
  {
    for(usize y = minBottomVoxel[1]; y <= maxBottomVoxel[1]; ++y)
    {
      for(usize x = minBottomVoxel[0]; x <= maxBottomVoxel[0]; ++x)
      {
        usize srcIdx = (z * yCount + y) * xCount + x;
        std::vector<float32> list = neighborList->copyOfList(srcIdx);
        neighborListBottom->setList(i, std::make_shared<std::vector<float32>>(list));
        std::string strVal = stringArray->at(srcIdx);
        stringValuesBottom[i] = strVal;
        i++;
      }
    }
  }
  StringArray::CreateWithValues(dataStructure, "StringArray", stringValuesBottom, croppedBottomAM.getId());

  auto* neighborListMiddle = NeighborList<float32>::Create(dataStructure, "NeighborList", numTuplesMiddle, croppedMiddleAM.getId());
  neighborListMiddle->resizeTotalElements(numTuplesMiddle);
  std::vector<std::string> stringValuesMiddle(numTuplesMiddle);
  i = 0;
  for(usize z = minMiddleVoxel[2]; z <= maxMiddleVoxel[2]; ++z)
  {
    for(usize y = minMiddleVoxel[1]; y <= maxMiddleVoxel[1]; ++y)
    {
      for(usize x = minMiddleVoxel[0]; x <= maxMiddleVoxel[0]; ++x)
      {
        usize srcIdx = (z * yCount + y) * xCount + x;
        std::vector<float32> list = neighborList->copyOfList(srcIdx);
        neighborListMiddle->setList(i, std::make_shared<std::vector<float32>>(list));
        std::string strVal = stringArray->at(srcIdx);
        stringValuesMiddle[i] = strVal;
        i++;
      }
    }
  }
  StringArray::CreateWithValues(dataStructure, "StringArray", stringValuesMiddle, croppedMiddleAM.getId());

  auto* neighborListTop = NeighborList<float32>::Create(dataStructure, "NeighborList", numTuplesTop, croppedTopAM.getId());
  neighborListTop->resizeTotalElements(numTuplesTop);
  std::vector<std::string> stringValueTop(numTuplesTop);
  i = 0;
  for(usize z = minTopVoxel[2]; z <= maxTopVoxel[2]; ++z)
  {
    for(usize y = minTopVoxel[1]; y <= maxTopVoxel[1]; ++y)
    {
      for(usize x = minTopVoxel[0]; x <= maxTopVoxel[0]; ++x)
      {
        usize srcIdx = (z * yCount + y) * xCount + x;
        std::vector<float32> list = neighborList->copyOfList(srcIdx);
        neighborListTop->setList(i, std::make_shared<std::vector<float32>>(list));
        std::string strVal = stringArray->at(srcIdx);
        stringValueTop[i] = strVal;
        i++;
      }
    }
  }
  StringArray::CreateWithValues(dataStructure, "StringArray", stringValueTop, croppedTopAM.getId());
}

void appendGeometries(DataStructure& dataStructure, const DataPath& destinationPath, const std::vector<DataPath>& inputPaths, uint64 appendDimension, std::optional<const DataPath> newGeometryPathOpt)
{
  AppendImageGeometryFilter filter;
  Arguments args;

  args.insertOrAssign(AppendImageGeometryFilter::k_InputGeometries_Key, std::make_any<std::vector<DataPath>>(inputPaths));
  args.insertOrAssign(AppendImageGeometryFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(destinationPath));
  args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(appendDimension));
  args.insertOrAssign(AppendImageGeometryFilter::k_CheckResolution_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryFilter::k_SaveAsNewGeometry_Key, std::make_any<bool>(newGeometryPathOpt.has_value()));

  if(newGeometryPathOpt.has_value())
  {
    args.insertOrAssign(AppendImageGeometryFilter::k_NewGeometry_Key, std::make_any<DataPath>(newGeometryPathOpt.value()));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  if(newGeometryPathOpt.has_value())
  {
    auto newGeometryPath = newGeometryPathOpt.value();
    // rename the cell data attribute matrix for easier comparison with exemplar data
    RenameDataObjectFilter renameFilter;
    Arguments renameArgs;
    renameArgs.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_CellData));
    renameArgs.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(newGeometryPath.createChildPath(ImageGeom::k_CellDataName)));
    auto renameResult = renameFilter.execute(dataStructure, renameArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(renameResult.result)
  }
}

} // namespace

TEST_CASE("SimplnxCore::AppendImageGeometryFilter: Valid Filter Execution", "[SimplnxCore][AppendImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");
  // Read in starting/exemplar image geometry
  const auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  auto [dimensionStr, minBottom, maxBottom, minMiddle, maxMiddle, minTop, maxTop, bottomPath, middlePath, topPath, appendDimension] =
      GENERATE(std::make_tuple("X Dimension", std::vector<uint64>{0, 0, 0}, std::vector<uint64>{66, 200, 116}, std::vector<uint64>{67, 0, 0}, std::vector<uint64>{121, 200, 116},
                               std::vector<uint64>{122, 0, 0}, std::vector<uint64>{188, 200, 116}, k_CroppedBottomXPath, k_CroppedMiddleXPath, k_CroppedTopXPath, 0),
               std::make_tuple("Y Dimension", std::vector<uint64>{0, 0, 0}, std::vector<uint64>{188, 62, 116}, std::vector<uint64>{0, 63, 0}, std::vector<uint64>{188, 122, 116},
                               std::vector<uint64>{0, 123, 0}, std::vector<uint64>{188, 200, 116}, k_CroppedBottomYPath, k_CroppedMiddleYPath, k_CroppedTopYPath, 1),
               std::make_tuple("Z Dimension", std::vector<uint64>{0, 0, 0}, std::vector<uint64>{188, 200, 41}, std::vector<uint64>{0, 0, 42}, std::vector<uint64>{188, 200, 87},
                               std::vector<uint64>{0, 0, 88}, std::vector<uint64>{188, 200, 116}, k_CroppedBottomZPath, k_CroppedMiddleZPath, k_CroppedTopZPath, 2));

  SECTION(fmt::format("{}", dimensionStr))
  {
    // Crop bottom half
    cropGeometry(dataStructure, k_DataContainerPath, bottomPath, minBottom, maxBottom);

    // Crop middle half
    cropGeometry(dataStructure, k_DataContainerPath, middlePath, minMiddle, maxMiddle);

    // Crop top half
    cropGeometry(dataStructure, k_DataContainerPath, topPath, minTop, maxTop);

    // Create neighbor lists and string arrays
    createNeighborListsAndStringArrays(dataStructure, minBottom, maxBottom, minMiddle, maxMiddle, minTop, maxTop, bottomPath, middlePath, topPath);

    DataPath appendedCellDataPath;
    SECTION("Append Geometry")
    {
      appendGeometries(dataStructure, bottomPath, std::vector<DataPath>{middlePath, topPath}, appendDimension, {});
      appendedCellDataPath = bottomPath.createChildPath(k_CellData);
    }

    SECTION("New Geometry")
    {
      appendGeometries(dataStructure, bottomPath, std::vector<DataPath>{middlePath, topPath}, appendDimension, k_AppendedGeometryPath);
      appendedCellDataPath = k_AppendedGeometryPath.createChildPath(k_CellData);
    }

    const usize numAppendedArrays = dataStructure.getDataRefAs<AttributeMatrix>(appendedCellDataPath).getSize();
    REQUIRE(numAppendedArrays == 8);

    CompareExemplarToGeneratedData(dataStructure, dataStructure, appendedCellDataPath, k_DataContainer);
  }
}

TEST_CASE("SimplnxCore::AppendImageGeometryFilter: Invalid Filter Execution", "[SimplnxCore][AppendImageGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                              "ResampleImageGeom_Exemplar.dream3d");

  // Read in test image geometries
  auto geomFilePath = fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(geomFilePath);

  AppendImageGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(AppendImageGeometryFilter::k_CheckResolution_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryFilter::k_SaveAsNewGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryFilter::k_NewGeometry_Key, std::make_any<DataPath>(k_AppendedGeometryPath));

  SECTION("Mismatching Dimensions")
  {
    args.insertOrAssign(AppendImageGeometryFilter::k_InputGeometries_Key, std::make_any<std::vector<DataPath>>({k_InvalidTestGeometryPath3}));
    args.insertOrAssign(AppendImageGeometryFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(DataPath({k_SmallIN100})));
    SECTION("X Dimension")
    {
      args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(0));
    }
    SECTION("Y Dimension")
    {
      args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(1));
    }
    SECTION("Z Dimension")
    {
      args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(2));
    }
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
      cropArgs.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_CroppedTopZPath));
      cropArgs.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
      cropArgs.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      cropArgs.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

      auto cropResult = cropFilter.execute(dataStructure, cropArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(cropResult.result)
    }
    {
      args.insertOrAssign(AppendImageGeometryFilter::k_InputGeometries_Key, std::make_any<std::vector<DataPath>>({k_InvalidTestGeometryPath1}));
      args.insertOrAssign(AppendImageGeometryFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(k_CroppedTopZPath));
      SECTION("X Dimension")
      {
        args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(0));
      }
      SECTION("Y Dimension")
      {
        args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(1));
      }
      SECTION("Z Dimension")
      {
        args.insertOrAssign(AppendImageGeometryFilter::k_AppendDimension_Key, std::make_any<uint64>(2));
      }
    }
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
