#include "SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Algorithm Test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  const DataPath kExemplarShiftsPath = constants::k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  unit_test::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects the in-core and OOC scenarios for this test.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Load the exemplar before the selected algorithm scenario.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_MaskArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_MaskArrayPath));

  // Execute the alignment with a fixed reference slice.
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  unit_test::CompareExemplarToGeneratedData(dataStructure, dataStructure, constants::k_CellAttributeMatrix, constants::k_ExemplarDataContainer);

// The optional output supports manual inspection of the aligned result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: output test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  const std::string kCentroidsName = "Centroids";

  unit_test::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects the in-core and OOC scenarios for this test.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Load the baseline before the selected algorithm scenario.
  auto baselineFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baselineFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_MaskArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_MaskArrayPath));

  // Execute the alignment and retain its diagnostic shift arrays.
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));

    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_AlignmentAMName_Key, std::make_any<std::string>(constants::k_AlignmentAMName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SlicesArrayName_Key, std::make_any<std::string>(constants::k_SlicesArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(constants::k_RelativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(constants::k_CumulativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(kCentroidsName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Load the expected shift arrays from the output exemplar.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/output_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  const DataPath alignmentAMPath = constants::k_DataContainerPath.createChildPath(constants::k_AlignmentAMName);

  const DataPath slicesPath = alignmentAMPath.createChildPath(constants::k_SlicesArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(slicesPath));
  unit_test::CompareDataArrays<uint32>(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath), dataStructure.getDataRefAs<IDataArray>(slicesPath));

  const DataPath relativeShiftsPath = alignmentAMPath.createChildPath(constants::k_RelativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  unit_test::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));

  const DataPath cumulativeShiftsPath = alignmentAMPath.createChildPath(constants::k_CumulativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  unit_test::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));

  const DataPath centroidsPath = alignmentAMPath.createChildPath(kCentroidsName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(centroidsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(centroidsPath));
  unit_test::CompareDataArrays<float32>(exemplarDataStructure.getDataRefAs<IDataArray>(centroidsPath), dataStructure.getDataRefAs<IDataArray>(centroidsPath));

// The optional output supports manual inspection of the aligned result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Geometry cell tuple count", "[Reconstruction][AlignSectionsFeatureCentroidFilter][TupleCount]")
{
  using FilterType = AlignSectionsFeatureCentroidFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 3, 2}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Matching arrays remain valid outside the geometry's Cell Attribute Matrix, with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<uint8>::Create(dataStructure, "Values", std::make_shared<DataStore<uint8>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(inputPath));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& errorRef = result.output_actions.errors()[0];
    REQUIRE(errorRef.code == -68071);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Nonzero reference top slice", "[SimplnxCore][AlignSectionsFeatureCentroidFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool storeShifts = GENERATE(false, true);
  CAPTURE(scenario, storeShifts);
  unit_test::AlgorithmTestScope scope(scenario);
  using FilterType = AlignSectionsFeatureCentroidFilter;
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({3, 3, 4});
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{4, 3, 3}, geomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geomPtr->setCellData(*cellsPtr);
  auto* maskPtr = UInt8ArrayType::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cellsPtr->getId());
  REQUIRE(maskPtr != nullptr);
  maskPtr->fill(0);
  // One marked cell per slice: (1,1), (0,0), (1,1), (2,2) in ascending Z.
  // Reference index 1 counts from the top, so it is physical slice 2.
  // Centroid differences in descending Z are (+1,+1), (0,0), (-1,-1), (0,0).
  // Reading source at destination + shift must place all marks at (1,1).
  (*maskPtr)[4] = 1;
  (*maskPtr)[9] = 1;
  (*maskPtr)[22] = 1;
  (*maskPtr)[35] = 1;

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cells", "Mask"})));
  args.insertOrAssign(FilterType::k_UseReferenceSlice_Key, std::make_any<bool>(true));
  args.insertOrAssign(FilterType::k_ReferenceSlice_Key, std::make_any<int32>(1));
  args.insertOrAssign(FilterType::k_StoreAlignmentShifts_Key, std::make_any<bool>(storeShifts));
  args.insertOrAssign(FilterType::k_AlignmentAMName_Key, std::make_any<std::string>("Shifts"));
  args.insertOrAssign(FilterType::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>("Cumulative"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  for(usize z = 0; z < 4; z++)
  {
    for(usize xy = 0; xy < 9; xy++)
    {
      CAPTURE(z, xy);
      CHECK((*maskPtr)[(z * 9) + xy] == (xy == 4 ? 1 : 0));
    }
  }
  if(storeShifts)
  {
    const auto& slices = dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Image", "Shifts", "Slice Indices"}));
    // Row 0 pairs the top slice (physical 3) with the reference (physical 2).
    CHECK(slices[0] == 3);
    CHECK(slices[1] == 2);
    const DataPath shiftsPath({"Image", "Shifts", "Cumulative"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64ArrayType>(shiftsPath));
    const auto& shifts = dataStructure.getDataRefAs<Int64ArrayType>(shiftsPath);
    const std::array<int64, 4> expectedShifts = {1, 0, -1, 0};
    for(usize iter = 0; iter < 4; iter++)
    {
      CAPTURE(iter);
      CHECK(shifts[iter * 2] == expectedShifts[iter]);
      CHECK(shifts[(iter * 2) + 1] == expectedShifts[iter]);
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Empty slices", "[SimplnxCore][AlignSectionsFeatureCentroidFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool storeShifts = GENERATE(false, true);
  const int emptyCase = GENERATE(0, 1, 2); // Empty top, middle, or fixed reference.
  const bool useReference = emptyCase != 1;
  CAPTURE(scenario, storeShifts, emptyCase);
  unit_test::AlgorithmTestScope scope(scenario);
  using FilterType = AlignSectionsFeatureCentroidFilter;
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({3, 3, 4});
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{4, 3, 3}, geomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geomPtr->setCellData(*cellsPtr);
  auto* maskPtr = UInt8ArrayType::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cellsPtr->getId());
  REQUIRE(maskPtr != nullptr);
  maskPtr->fill(0);
  // Descending-Z centroids are (2,2), (1,1), (0,0), (1,1) before removing a mark.
  // Empty top with reference 1: offsets [0,0,-1,0]. Empty reference: all offsets 0.
  // Empty middle (index 2), consecutive: relative [0,-1,0,0], cumulative [0,-1,-1,-1].
  // Both axes have the same values. A zero relative shift preserves the previous cumulative shift.
  std::array<int64, 4> expectedShifts = emptyCase == 0 ? std::array<int64, 4>{0, 0, -1, 0} : emptyCase == 1 ? std::array<int64, 4>{0, -1, -1, -1} : std::array<int64, 4>{0, 0, 0, 0};
  std::array<int64, 4> expectedRelative = emptyCase == 1 ? std::array<int64, 4>{0, -1, 0, 0} : expectedShifts;
  const usize emptyZ = emptyCase == 0 ? 3 : emptyCase == 1 ? 1 : 2;
  const std::array<usize, 4> markXY = {4, 0, 4, 8};
  for(usize z = 0; z < 4; z++)
  {
    if(z != emptyZ)
    {
      (*maskPtr)[z * 9 + markXY[z]] = 1;
    }
  }
  // A separate marker makes the empty slice's applied shift observable without diagnostics.
  auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cellsPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(0);
  for(usize z = 0; z < 4; z++)
  {
    (*valuesPtr)[z * 9 + 4] = 7;
  }

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cells", "Mask"})));
  args.insertOrAssign(FilterType::k_UseReferenceSlice_Key, std::make_any<bool>(useReference));
  args.insertOrAssign(FilterType::k_ReferenceSlice_Key, std::make_any<int32>(1));
  args.insertOrAssign(FilterType::k_StoreAlignmentShifts_Key, std::make_any<bool>(storeShifts));
  args.insertOrAssign(FilterType::k_AlignmentAMName_Key, std::make_any<std::string>("Shifts"));
  args.insertOrAssign(FilterType::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>("Cumulative"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  for(usize z = 0; z < 4; z++)
  {
    const int64 shift = expectedShifts[3 - z];
    for(usize xy = 0; xy < 9; xy++)
    {
      CAPTURE(z, xy);
      const int64 sourceX = static_cast<int64>(xy % 3) + shift;
      const int64 sourceY = static_cast<int64>(xy / 3) + shift;
      CHECK((*valuesPtr)[z * 9 + xy] == (sourceX == 1 && sourceY == 1 ? 7 : 0));
    }
  }
  if(storeShifts)
  {
    const DataPath shiftsPath({"Image", "Shifts", "Cumulative"});
    const DataPath relativePath({"Image", "Shifts", "Relative Shifts"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64ArrayType>(shiftsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64ArrayType>(relativePath));
    const auto& shifts = dataStructure.getDataRefAs<Int64ArrayType>(shiftsPath);
    const auto& relative = dataStructure.getDataRefAs<Int64ArrayType>(relativePath);
    for(usize iter = 0; iter < 4; iter++)
    {
      CAPTURE(iter);
      for(usize axis = 0; axis < 2; axis++)
      {
        CHECK(shifts[iter * 2 + axis] == expectedShifts[iter]);
        CHECK(relative[iter * 2 + axis] == expectedRelative[iter]);
      }
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Reference slice bounds", "[SimplnxCore][AlignSectionsFeatureCentroidFilter]")
{
  using FilterType = AlignSectionsFeatureCentroidFilter;
  const usize zDim = GENERATE(usize{1}, usize{3});
  const bool useReference = GENERATE(true, false);
  const bool atUpperBound = GENERATE(true, false);
  const bool storeShifts = GENERATE(false, true);
  // Z slices have exactly the integer indices 0 through Z-1. Z itself is not an index.
  const int32 reference = static_cast<int32>(atUpperBound ? zDim : zDim - 1);
  CAPTURE(zDim, useReference, reference, storeShifts);
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({3, 3, zDim});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{zDim, 3, 3}, geomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geomPtr->setCellData(*cellsPtr);
  REQUIRE(UInt8ArrayType::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{zDim, 3, 3}, ShapeType{1}, 0), cellsPtr->getId()) != nullptr);

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cells", "Mask"})));
  args.insertOrAssign(FilterType::k_UseReferenceSlice_Key, std::make_any<bool>(useReference));
  args.insertOrAssign(FilterType::k_ReferenceSlice_Key, std::make_any<int32>(reference));
  args.insertOrAssign(FilterType::k_StoreAlignmentShifts_Key, std::make_any<bool>(storeShifts));
  // Stop at preflight: invalid references must never reach the alignment algorithm.
  IFilter::PreflightResult result;
  REQUIRE_NOTHROW(result = filter.preflight(dataStructure, args));
  if(useReference && atUpperBound)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& error = result.output_actions.errors()[0];
    REQUIRE(error.code == -68073);
    CHECK(error.message.find(fmt::format("Reference Slice value ({}) is too large.", reference)) != std::string::npos);
    CHECK(error.message.find(fmt::format("Z dimension {}", zDim)) != std::string::npos);
    CHECK(error.message.find(fmt::format("0 to {}", zDim - 1)) != std::string::npos);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
