#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/ResampleRectGridToImageGeomFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <array>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_RectGridGeomPath({"RectGridGeom"});
const DataPath k_RectGridCellDataPath = k_RectGridGeomPath.createChildPath(k_CellData);
const DataPath k_RectGridCellFeatureArrayPath = k_RectGridGeomPath.createChildPath(k_CellFeatureData).createChildPath(k_AvgQuats);

const DataPath k_ImageGeomComputed({"ImageGeom [Computed]"});

const std::string k_NeighborListArrayName = "NeighborList";
const std::string k_StringArrayName = "StringArray";
} // namespace

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: Noncubic cell arrays preserve values and inherit tuple shape", "[SimplnxCore][ResampleRectGridToImageGeomFilter]")
{
  DataStructure dataStructure;
  const SizeVec3 dimensions{2, 3, 4};
  const ShapeType cellTupleShape{4, 3, 2};
  constexpr usize totalVoxels = 24;
  const DataPath numericPath = k_RectGridCellDataPath.createChildPath("CoordinateLabels");
  const DataPath neighborListPath = k_RectGridCellDataPath.createChildPath(k_NeighborListArrayName);
  const DataPath stringPath = k_RectGridCellDataPath.createChildPath(k_StringArrayName);

  auto* rectGridGeomPtr = RectGridGeom::Create(dataStructure, k_RectGridGeomPath.getTargetName());
  REQUIRE(rectGridGeomPtr != nullptr);
  rectGridGeomPtr->setDimensions(dimensions);
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellData, cellTupleShape, rectGridGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  rectGridGeomPtr->setCellData(*cellDataPtr);

  const std::array<std::string, 3> boundsNames{"XBounds", "YBounds", "ZBounds"};
  std::array<Float32Array*, 3> bounds{};
  for(usize axisIdx = 0; axisIdx < bounds.size(); ++axisIdx)
  {
    const DataPath boundsPath = k_RectGridGeomPath.createChildPath(boundsNames[axisIdx]);
    bounds[axisIdx] =
        Float32Array::Create(dataStructure, boundsNames[axisIdx], DataStoreUtilities::CreateDataStore<float32>(dataStructure, boundsPath, {dimensions[axisIdx] + 1}, {1}), rectGridGeomPtr->getId());
    REQUIRE(bounds[axisIdx] != nullptr);
    for(usize boundIdx = 0; boundIdx <= dimensions[axisIdx]; ++boundIdx)
    {
      bounds[axisIdx]->setValue(boundIdx, static_cast<float32>(boundIdx));
    }
  }
  auto boundsResult = rectGridGeomPtr->setBounds(bounds[0], bounds[1], bounds[2]);
  SIMPLNX_RESULT_REQUIRE_VALID(boundsResult);

  auto* numericPtr = Int32Array::Create(dataStructure, numericPath.getTargetName(), DataStoreUtilities::CreateDataStore<int32>(dataStructure, numericPath, cellTupleShape, {1}), cellDataPtr->getId());
  REQUIRE(numericPtr != nullptr);
  auto* neighborListPtr = NeighborList<int32>::Create(dataStructure, k_NeighborListArrayName, cellTupleShape, cellDataPtr->getId());
  REQUIRE(neighborListPtr != nullptr);
  std::vector<std::string> strings(totalVoxels);
  usize voxelIdx = 0;
  for(usize z = 0; z < dimensions[2]; ++z)
  {
    for(usize y = 0; y < dimensions[1]; ++y)
    {
      for(usize x = 0; x < dimensions[0]; ++x)
      {
        const int32 label = static_cast<int32>(x + 10 * y + 100 * z);
        numericPtr->setValue(voxelIdx, label);
        neighborListPtr->setList(static_cast<int32>(voxelIdx), std::vector<int32>{label});
        strings[voxelIdx] = fmt::format("x={},y={},z={}", x, y, z);
        ++voxelIdx;
      }
    }
  }
  REQUIRE(StringArray::CreateWithValues(dataStructure, k_StringArrayName, cellTupleShape, std::move(strings), cellDataPtr->getId()) != nullptr);

  ResampleRectGridToImageGeomFilter filter;
  Arguments args;
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{2, 3, 4}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{numericPath, stringPath, neighborListPath}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputCellDataPath = k_ImageGeomComputed.createChildPath(k_CellData);
  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomComputed);
  const auto* outputCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(outputCellDataPath);
  const auto* outputNumericPtr = dataStructure.getDataAs<Int32Array>(outputCellDataPath.createChildPath(numericPath.getTargetName()));
  const auto* outputStringsPtr = dataStructure.getDataAs<StringArray>(outputCellDataPath.createChildPath(k_StringArrayName));
  const auto* outputNeighborsPtr = dataStructure.getDataAs<NeighborList<int32>>(outputCellDataPath.createChildPath(k_NeighborListArrayName));
  REQUIRE(imageGeomPtr != nullptr);
  REQUIRE(outputCellDataPtr != nullptr);
  REQUIRE(outputNumericPtr != nullptr);
  REQUIRE(outputStringsPtr != nullptr);
  REQUIRE(outputNeighborsPtr != nullptr);
  REQUIRE(outputNumericPtr->getNumberOfTuples() == totalVoxels);
  REQUIRE(outputStringsPtr->getNumberOfTuples() == totalVoxels);
  REQUIRE(outputNeighborsPtr->getNumberOfTuples() == totalVoxels);

  // Unit bounds make this an identity map. The flat oracle uses X-fastest coordinates.
  for(usize outputIdx = 0; outputIdx < totalVoxels; ++outputIdx)
  {
    CAPTURE(outputIdx);
    const usize x = outputIdx % 2;
    const usize y = (outputIdx / 2) % 3;
    const usize z = outputIdx / 6;
    const int32 expectedLabel = static_cast<int32>(x + 10 * y + 100 * z);
    CHECK((*outputNumericPtr)[outputIdx] == expectedLabel);
    CHECK((*outputStringsPtr)[outputIdx] == fmt::format("x={},y={},z={}", x, y, z));
    CHECK(outputNeighborsPtr->getList(static_cast<int32>(outputIdx)) == std::vector<int32>{expectedLabel});
  }

  CHECK(imageGeomPtr->getDimensions() == dimensions);
  CHECK(outputCellDataPtr->getShape() == cellTupleShape);
  CHECK(outputNumericPtr->getTupleShape() == cellTupleShape);
  CHECK(outputStringsPtr->getTupleShape() == cellTupleShape);
  CHECK(outputNeighborsPtr->getTupleShape() == cellTupleShape);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: Valid Filter Execution", "[SimplnxCore][ResampleRectGridToImageGeomFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_resample_rect_grid_to_image_geom.tar.gz", "6_6_resample_rect_grid_to_image_geom.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create a neighbor list and string array for testing selected arrays with all IArray types
  {
    const auto& cellDataAM = dataStructure.getDataRefAs<AttributeMatrix>(k_RectGridCellDataPath);
    const usize numTuples = cellDataAM.getNumberOfTuples();

    std::vector<std::string> stringArrayValues(numTuples);
    auto* neighborList = NeighborList<float32>::Create(dataStructure, k_NeighborListArrayName, cellDataAM.getShape(), cellDataAM.getId());
    for(usize i = 0; i < numTuples; ++i)
    {
      const float32 factor = static_cast<float32>(i) * 0.001;
      std::vector<float32> list = {factor * 117, factor * 875, factor * 1035, factor * 3905, factor * 4214};
      neighborList->setList(i, std::make_shared<std::vector<float32>>(list));
      stringArrayValues[i] = "String_" + StringUtilities::number(factor);
    }

    const auto* stringArray = StringArray::CreateWithValues(dataStructure, k_StringArrayName, cellDataAM.getShape(), stringArrayValues, cellDataAM.getId());
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                          k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds),
                          k_RectGridCellDataPath.createChildPath(k_NeighborListArrayName), k_RectGridCellDataPath.createChildPath(k_StringArrayName)}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));

  std::string exemplarImageGeomName;

  SECTION("Resample Smaller")
  {
    exemplarImageGeomName = "ImageGeom [Smaller]";
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Resample Bigger")
  {
    exemplarImageGeomName = "ImageGeom [Bigger]";
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 20)));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareExemplarToGeneratedData(dataStructure, dataStructure, k_ImageGeomComputed.createChildPath(k_CellData), exemplarImageGeomName);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: InValid Filter Execution")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_resample_rect_grid_to_image_geom.tar.gz", "6_6_resample_rect_grid_to_image_geom.dream3d");
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));

  SECTION("Missing Selected Arrays")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Mismatching Selected Array Tuples")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(
                            MultiArraySelectionParameter::ValueType{k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles),
                                                                    k_RectGridCellDataPath.createChildPath(k_FeatureIds), k_RectGridCellFeatureArrayPath}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Invalid Cell Level Selected Array")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_RectGridCellFeatureArrayPath}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Invalid Dimension Value")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                            k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds)}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{5, 5, -1}));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ResampleRectGridToImageGeomFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ResampleRectGridToImageGeomFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ResampleRectGridToImageGeomFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key) == DataPath({"DataContainer"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key) == DataPath({"ImageGeometry"}));
      CHECK(args.value<std::string>(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key) == "CellData");
    }
  }
}
