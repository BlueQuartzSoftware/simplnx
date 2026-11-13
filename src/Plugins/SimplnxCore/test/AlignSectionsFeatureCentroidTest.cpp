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
  const DataPath k_ExemplarShiftsPath = Constants::k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  UnitTest::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects the in-core and OOC scenarios for this test.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Load the exemplar before the selected algorithm scenario.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));

  // Execute the alignment with a fixed reference slice.
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);

// The optional output supports manual inspection of the aligned result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: output test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  const std::string k_CentroidsName = "Centroids";

  UnitTest::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects the in-core and OOC scenarios for this test.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Load the baseline before the selected algorithm scenario.
  auto baselineFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baselineFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));

  // Execute the alignment and retain its diagnostic shift arrays.
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_AlignmentAMName_Key, std::make_any<std::string>(Constants::k_AlignmentAMName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SlicesArrayName_Key, std::make_any<std::string>(Constants::k_SlicesArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(Constants::k_RelativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(Constants::k_CumulativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Load the expected shift arrays from the output exemplar.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/output_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath alignmentAMPath = Constants::k_DataContainerPath.createChildPath(Constants::k_AlignmentAMName);

  const DataPath slicesPath = alignmentAMPath.createChildPath(Constants::k_SlicesArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(slicesPath));
  UnitTest::CompareDataArrays<uint32>(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath), dataStructure.getDataRefAs<IDataArray>(slicesPath));

  const DataPath relativeShiftsPath = alignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  UnitTest::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));

  const DataPath cumulativeShiftsPath = alignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  UnitTest::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));

  const DataPath centroidsPath = alignmentAMPath.createChildPath(k_CentroidsName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(centroidsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(centroidsPath));
  UnitTest::CompareDataArrays<float32>(exemplarDataStructure.getDataRefAs<IDataArray>(centroidsPath), dataStructure.getDataRefAs<IDataArray>(centroidsPath));

// The optional output supports manual inspection of the aligned result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Geometry cell tuple count", "[Reconstruction][AlignSectionsFeatureCentroidFilter][TupleCount]")
{
  using FilterType = AlignSectionsFeatureCentroidFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
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
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -68071);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Nonzero reference top slice", "[SimplnxCore][AlignSectionsFeatureCentroidFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool storeShifts = GENERATE(false, true);
  CAPTURE(scenario, storeShifts);
  UnitTest::AlgorithmTestScope scope(scenario);
  using FilterType = AlignSectionsFeatureCentroidFilter;
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geom != nullptr);
  geom->setDimensions({3, 3, 4});
  geom->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{4, 3, 3}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  auto* mask = UInt8Array::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cells->getId());
  REQUIRE(mask != nullptr);
  mask->fill(0);
  // One marked cell per slice: (1,1), (0,0), (1,1), (2,2) in ascending Z.
  // Reference index 1 counts from the top, so it is physical slice 2.
  // Centroid differences in descending Z are (+1,+1), (0,0), (-1,-1), (0,0).
  // Reading source at destination + shift must place all marks at (1,1).
  (*mask)[4] = 1;
  (*mask)[9] = 1;
  (*mask)[22] = 1;
  (*mask)[35] = 1;

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  for(usize z = 0; z < 4; z++)
  {
    for(usize xy = 0; xy < 9; xy++)
    {
      CAPTURE(z, xy);
      CHECK((*mask)[(z * 9) + xy] == (xy == 4 ? 1 : 0));
    }
  }
  if(storeShifts)
  {
    const auto& slices = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Image", "Shifts", "Slice Indices"}));
    // Row 0 pairs the top slice (physical 3) with the reference (physical 2).
    CHECK(slices[0] == 3);
    CHECK(slices[1] == 2);
    const DataPath shiftsPath({"Image", "Shifts", "Cumulative"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(shiftsPath));
    const auto& shifts = dataStructure.getDataRefAs<Int64Array>(shiftsPath);
    const std::array<int64, 4> expectedShifts = {1, 0, -1, 0};
    for(usize iter = 0; iter < 4; iter++)
    {
      CAPTURE(iter);
      CHECK(shifts[iter * 2] == expectedShifts[iter]);
      CHECK(shifts[(iter * 2) + 1] == expectedShifts[iter]);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Empty slices", "[SimplnxCore][AlignSectionsFeatureCentroidFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool storeShifts = GENERATE(false, true);
  const int emptyCase = GENERATE(0, 1, 2); // Empty top, middle, or fixed reference.
  const bool useReference = emptyCase != 1;
  CAPTURE(scenario, storeShifts, emptyCase);
  UnitTest::AlgorithmTestScope scope(scenario);
  using FilterType = AlignSectionsFeatureCentroidFilter;
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geom != nullptr);
  geom->setDimensions({3, 3, 4});
  geom->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{4, 3, 3}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  auto* mask = UInt8Array::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cells->getId());
  REQUIRE(mask != nullptr);
  mask->fill(0);
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
      (*mask)[z * 9 + markXY[z]] = 1;
    }
  }
  // A separate marker makes the empty slice's applied shift observable without diagnostics.
  auto* values = Int32Array::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cells->getId());
  REQUIRE(values != nullptr);
  values->fill(0);
  for(usize z = 0; z < 4; z++)
  {
    (*values)[z * 9 + 4] = 7;
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
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
      CHECK((*values)[z * 9 + xy] == (sourceX == 1 && sourceY == 1 ? 7 : 0));
    }
  }
  if(storeShifts)
  {
    const DataPath shiftsPath({"Image", "Shifts", "Cumulative"});
    const DataPath relativePath({"Image", "Shifts", "Relative Shifts"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(shiftsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(relativePath));
    const auto& shifts = dataStructure.getDataRefAs<Int64Array>(shiftsPath);
    const auto& relative = dataStructure.getDataRefAs<Int64Array>(relativePath);
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  auto* geom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geom != nullptr);
  geom->setDimensions({3, 3, zDim});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{zDim, 3, 3}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  REQUIRE(UInt8Array::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{zDim, 3, 3}, ShapeType{1}, 0), cells->getId()) != nullptr);

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
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& error = result.outputActions.errors()[0];
    REQUIRE(error.code == -68073);
    CHECK(error.message.find(fmt::format("Reference Slice value ({}) is too large.", reference)) != std::string::npos);
    CHECK(error.message.find(fmt::format("Z dimension {}", zDim)) != std::string::npos);
    CHECK(error.message.find(fmt::format("0 to {}", zDim - 1)) != std::string::npos);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
