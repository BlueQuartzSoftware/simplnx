#include "SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
// Fixture scaffolding shared by the hand-derived (Class 1) oracle test cases.
//
// Geometry dimensions are ordered X, Y, Z. AttributeMatrix / DataArray tuple shapes are ordered
// slowest to fastest, i.e. Z, Y, X.
//
// Every fixture carries two cell arrays:
//  * "Mask"    - the in-mask flag the filter reduces to a per-slice centroid
//  * "Payload" - Payload(z, y, x) = 100*z + 10*y + x, so the exact source cell of every aligned
//                value is readable straight off the number.
// -----------------------------------------------------------------------------
const std::string k_GeomName = "Image Geometry";
const std::string k_CellAmName = "Cell Data";
const std::string k_MaskName = "Mask";
const std::string k_PayloadName = "Payload";
const std::string k_AlignmentAmName = "Alignment Shifts Data";
const std::string k_SlicesName = "Slice Indices";
const std::string k_RelativeShiftsName = "Relative Shifts";
const std::string k_CumulativeShiftsName = "Cumulative Shifts";
const std::string k_CentroidsName = "Centroids";

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellAmPath = k_GeomPath.createChildPath(k_CellAmName);
const DataPath k_MaskPath = k_CellAmPath.createChildPath(k_MaskName);
const DataPath k_PayloadPath = k_CellAmPath.createChildPath(k_PayloadName);
const DataPath k_AlignmentAmPath = k_GeomPath.createChildPath(k_AlignmentAmName);

//! @brief An (x, y) cell of a single slice's mask.
struct MaskCell
{
  usize x = 0;
  usize y = 0;
};

//! @brief Mask cells keyed by *physical* slice index (0 == the slice at the Z origin).
using MaskLayout = std::map<usize, std::vector<MaskCell>>;

enum class MaskStorage
{
  Boolean,
  UInt8
};

DataStructure CreateFixture(const SizeVec3& geomDims, const FloatVec3& spacing, const MaskLayout& maskLayout, MaskStorage maskStorage = MaskStorage::Boolean)
{
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions(geomDims);
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(FloatVec3{0.0F, 0.0F, 0.0F});

  const ShapeType tupleShape{geomDims[2], geomDims[1], geomDims[0]};
  auto* cellAm = AttributeMatrix::Create(dataStructure, k_CellAmName, tupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAm);

  // Payload
  auto payloadStore = std::make_shared<Int32DataStore>(tupleShape, ShapeType{1}, 0);
  auto* payloadArray = Int32Array::Create(dataStructure, k_PayloadName, payloadStore, cellAm->getId());
  auto& payloadRef = payloadArray->getDataStoreRef();
  for(usize z = 0; z < geomDims[2]; z++)
  {
    for(usize y = 0; y < geomDims[1]; y++)
    {
      for(usize x = 0; x < geomDims[0]; x++)
      {
        payloadRef[(z * geomDims[1] * geomDims[0]) + (y * geomDims[0]) + x] = static_cast<int32>((100 * z) + (10 * y) + x);
      }
    }
  }

  // Mask
  if(maskStorage == MaskStorage::Boolean)
  {
    auto maskStore = std::make_shared<BoolDataStore>(tupleShape, ShapeType{1}, false);
    auto* maskArray = BoolArray::Create(dataStructure, k_MaskName, maskStore, cellAm->getId());
    auto& maskRef = maskArray->getDataStoreRef();
    for(const auto& [slice, cells] : maskLayout)
    {
      for(const auto& cell : cells)
      {
        maskRef[(slice * geomDims[1] * geomDims[0]) + (cell.y * geomDims[0]) + cell.x] = true;
      }
    }
  }
  else
  {
    auto maskStore = std::make_shared<UInt8DataStore>(tupleShape, ShapeType{1}, 0);
    auto* maskArray = UInt8Array::Create(dataStructure, k_MaskName, maskStore, cellAm->getId());
    auto& maskRef = maskArray->getDataStoreRef();
    for(const auto& [slice, cells] : maskLayout)
    {
      for(const auto& cell : cells)
      {
        // MaskCompare treats any non-zero uint8 as in-mask (MaskCompareUtilities.hpp:121)
        maskRef[(slice * geomDims[1] * geomDims[0]) + (cell.y * geomDims[0]) + cell.x] = 255;
      }
    }
  }

  return dataStructure;
}

Arguments CreateArgs(bool useReferenceSlice, int32 referenceSlice, bool storeAlignmentShifts)
{
  Arguments args;
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(useReferenceSlice));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(referenceSlice));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(storeAlignmentShifts));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_AlignmentAMName_Key, std::make_any<std::string>(k_AlignmentAmName));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SlicesArrayName_Key, std::make_any<std::string>(k_SlicesName));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(k_RelativeShiftsName));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(k_CumulativeShiftsName));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));
  return args;
}

//! @brief Compares an int32 cell array against a fully enumerated expectation (flat z, y, x order).
void CheckPayload(DataStructure& dataStructure, const std::vector<int32>& expected)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_PayloadPath));
  const auto& payloadRef = dataStructure.getDataRefAs<Int32Array>(k_PayloadPath).getDataStoreRef();
  REQUIRE(payloadRef.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(payloadRef[i] == expected[i]);
  }
}

//! @brief Compares the boolean mask against the set of cells expected to be in-mask after alignment.
void CheckMask(DataStructure& dataStructure, const SizeVec3& geomDims, const MaskLayout& expectedLayout)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(k_MaskPath));
  const auto& maskRef = dataStructure.getDataRefAs<BoolArray>(k_MaskPath).getDataStoreRef();

  std::vector<bool> expected(geomDims[0] * geomDims[1] * geomDims[2], false);
  for(const auto& [slice, cells] : expectedLayout)
  {
    for(const auto& cell : cells)
    {
      expected[(slice * geomDims[1] * geomDims[0]) + (cell.y * geomDims[0]) + cell.x] = true;
    }
  }

  for(usize i = 0; i < expected.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(maskRef[i] == expected[i]);
  }
}

//! @brief Compares all four optional alignment-shift arrays against hand-derived values.
//! Each expectation is flat: two components per tuple.
void CheckShiftArrays(DataStructure& dataStructure, const std::vector<uint32>& slices, const std::vector<int64>& relativeShifts, const std::vector<int64>& cumulativeShifts,
                      const std::vector<float32>& centroids)
{
  const DataPath slicesPath = k_AlignmentAmPath.createChildPath(k_SlicesName);
  const DataPath relativeShiftsPath = k_AlignmentAmPath.createChildPath(k_RelativeShiftsName);
  const DataPath cumulativeShiftsPath = k_AlignmentAmPath.createChildPath(k_CumulativeShiftsName);
  const DataPath centroidsPath = k_AlignmentAmPath.createChildPath(k_CentroidsName);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(slicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(relativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(cumulativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(centroidsPath));

  const auto& slicesRef = dataStructure.getDataRefAs<UInt32Array>(slicesPath).getDataStoreRef();
  const auto& relativeShiftsRef = dataStructure.getDataRefAs<Int64Array>(relativeShiftsPath).getDataStoreRef();
  const auto& cumulativeShiftsRef = dataStructure.getDataRefAs<Int64Array>(cumulativeShiftsPath).getDataStoreRef();
  const auto& centroidsRef = dataStructure.getDataRefAs<Float32Array>(centroidsPath).getDataStoreRef();

  REQUIRE(slicesRef.getSize() == slices.size());
  REQUIRE(relativeShiftsRef.getSize() == relativeShifts.size());
  REQUIRE(cumulativeShiftsRef.getSize() == cumulativeShifts.size());
  REQUIRE(centroidsRef.getSize() == centroids.size());

  for(usize i = 0; i < slices.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(slicesRef[i] == slices[i]);
    REQUIRE(relativeShiftsRef[i] == relativeShifts[i]);
    REQUIRE(cumulativeShiftsRef[i] == cumulativeShifts[i]);
    REQUIRE(centroidsRef[i] == Approx(centroids[i]));
  }
}

//! @brief Returns the number of warnings carrying the given code.
usize CountWarnings(const Result<>& result, int32 code)
{
  return static_cast<usize>(std::count_if(result.warnings().cbegin(), result.warnings().cend(), [code](const Warning& warning) { return warning.code == code; }));
}

//! @brief Returns true when the result carries an error with the given code.
template <class T>
bool HasError(const Result<T>& result, int32 code)
{
  return std::any_of(result.errors().cbegin(), result.errors().cend(), [code](const Error& error) { return error.code == code; });
}

// -----------------------------------------------------------------------------
// F1 / F2 / F2b / F5 / F9 / F10 share this 5x5x3 layout. Mask = one 2x2 block per slice.
//   physical slice 2 (top): block corner (0,0) -> mean_x 0.5, mean_y 0.5
//   physical slice 1      : block corner (1,0) -> mean_x 1.5, mean_y 0.5
//   physical slice 0      : block corner (3,2) -> mean_x 3.5, mean_y 2.5
// -----------------------------------------------------------------------------
const SizeVec3 k_StackDims{5, 5, 3};
const FloatVec3 k_UnitSpacing{1.0F, 1.0F, 1.0F};

MaskLayout StackedBlockLayout()
{
  return MaskLayout{
      {2, {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
      {1, {{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {0, {{3, 2}, {4, 2}, {3, 3}, {4, 3}}},
  };
}

// clang-format off
// Consecutive-mode result: xShifts {0, 1, 3}, yShifts {0, 0, 2}
// slice 0 <- out(x,y) = in(x+3, y+2); slice 1 <- out(x,y) = in(x+1, y); slice 2 unchanged.
const std::vector<int32> k_ConsecutivePayload{
    // z = 0
     23,  24,   0,   0,   0,
     33,  34,   0,   0,   0,
     43,  44,   0,   0,   0,
      0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,
    // z = 1
    101, 102, 103, 104,   0,
    111, 112, 113, 114,   0,
    121, 122, 123, 124,   0,
    131, 132, 133, 134,   0,
    141, 142, 143, 144,   0,
    // z = 2
    200, 201, 202, 203, 204,
    210, 211, 212, 213, 214,
    220, 221, 222, 223, 224,
    230, 231, 232, 233, 234,
    240, 241, 242, 243, 244,
};

// Reference-mode-on-the-Z-origin-slice result: xShifts {-3, -2, 0}, yShifts {-2, -2, 0}
// slice 0 unchanged; slice 1 <- out(x,y) = in(x-2, y-2); slice 2 <- out(x,y) = in(x-3, y-2).
const std::vector<int32> k_ReferenceOriginPayload{
    // z = 0
      0,   1,   2,   3,   4,
     10,  11,  12,  13,  14,
     20,  21,  22,  23,  24,
     30,  31,  32,  33,  34,
     40,  41,  42,  43,  44,
    // z = 1
      0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,
      0,   0, 100, 101, 102,
      0,   0, 110, 111, 112,
      0,   0, 120, 121, 122,
    // z = 2
      0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,
      0,   0,   0, 200, 201,
      0,   0,   0, 210, 211,
      0,   0,   0, 220, 221,
};
// clang-format on

// Every block ends up stacked on the top slice's block at x in {0,1}, y in {0,1}.
MaskLayout ConsecutiveMaskResult()
{
  const std::vector<MaskCell> block{{0, 0}, {1, 0}, {0, 1}, {1, 1}};
  return MaskLayout{{0, block}, {1, block}, {2, block}};
}

// Every block ends up stacked on the Z-origin slice's block at x in {3,4}, y in {2,3}.
MaskLayout ReferenceOriginMaskResult()
{
  const std::vector<MaskCell> block{{3, 2}, {4, 2}, {3, 3}, {4, 3}};
  return MaskLayout{{0, block}, {1, block}, {2, block}};
}
} // namespace

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Algorithm Test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(Constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Constants::k_DataContainerPath);
  const usize zDim = imageGeom.getDimensions()[2];

  // Align Sections Feature Centroid Filter
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    // The exemplar in this archive was produced when "Reference Slice" indexed the filter's internal
    // top-down iteration order, so "0" meant the slice farthest from the Z origin. "Reference Slice"
    // is now a physical slice index, so the equivalent request is the last slice: old 0 == new zDim-1.
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(static_cast<int32>(zDim - 1)));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // No slice of this data set is fully masked out and no shift leaves the volume, so none of the
    // algorithm's diagnostic warnings may fire on the legacy-parity path.
    REQUIRE(CountWarnings(executeResult.result, -53902) == 0);
    REQUIRE(CountWarnings(executeResult.result, -53903) == 0);
    REQUIRE(CountWarnings(executeResult.result, -53904) == 0);
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);

// Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Consecutive Mode Integer Offsets", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // Mask block corners walk (0,0) -> (1,0) -> (3,2) from the top slice down, so the per-slice
  // centroids are (0.5, 0.5), (1.5, 0.5) and (3.5, 2.5). Relative shifts truncate to (1,0) then
  // (2,2), which accumulate to xShifts {0, 1, 3} and yShifts {0, 0, 2}.
  DataStructure dataStructure = CreateFixture(k_StackDims, k_UnitSpacing, StackedBlockLayout());

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  REQUIRE(executeResult.result.warnings().empty());

  CheckPayload(dataStructure, k_ConsecutivePayload);
  CheckMask(dataStructure, k_StackDims, ConsecutiveMaskResult());

  // Consecutive mode leaves tuple 0 as the deterministic zero-filled anchor row: the top slice is
  // the anchor and is never moved.
  CheckShiftArrays(dataStructure, {0, 0, 1, 2, 0, 1}, {0, 0, 1, 0, 2, 2}, {0, 0, 1, 0, 3, 2}, {0.0F, 0.0F, 1.5F, 0.5F, 3.5F, 2.5F});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Reference Slice Is A Physical Slice Index", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Reference Slice 2 anchors the slice farthest from the Z origin")
  {
    // Reference centroid = physical slice 2 = (0.5, 0.5). Shifts are trunc(mean - 0.5):
    // xShifts {0, 1, 3}, yShifts {0, 0, 2} - the same alignment F1 reaches by chaining.
    DataStructure dataStructure = CreateFixture(k_StackDims, k_UnitSpacing, StackedBlockLayout());

    AlignSectionsFeatureCentroidFilter filter;
    Arguments args = CreateArgs(true, 2, true);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    REQUIRE(executeResult.result.warnings().empty());

    CheckPayload(dataStructure, k_ConsecutivePayload);
    CheckMask(dataStructure, k_StackDims, ConsecutiveMaskResult());

    // Reference mode aligns every slice including the topmost, so tuple 0 carries a real row.
    // Slices keeps the {slice, slice+1} pairing convention, hence {2, 3} for the top slice.
    CheckShiftArrays(dataStructure, {2, 3, 1, 2, 0, 1}, {0, 0, 1, 0, 3, 2}, {0, 0, 1, 0, 3, 2}, {0.5F, 0.5F, 1.5F, 0.5F, 3.5F, 2.5F});

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Reference Slice 0 anchors the slice at the Z origin")
  {
    // Reference centroid = physical slice 0 = (3.5, 2.5). Shifts are trunc(mean - reference):
    // xShifts {-3, -2, 0}, yShifts {-2, -2, 0}. The nonzero shift at tuple 0 is what forces the
    // shared transfer loop to start at i = 0.
    DataStructure dataStructure = CreateFixture(k_StackDims, k_UnitSpacing, StackedBlockLayout());

    AlignSectionsFeatureCentroidFilter filter;
    Arguments args = CreateArgs(true, 0, true);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    REQUIRE(executeResult.result.warnings().empty());

    CheckPayload(dataStructure, k_ReferenceOriginPayload);
    CheckMask(dataStructure, k_StackDims, ReferenceOriginMaskResult());

    CheckShiftArrays(dataStructure, {2, 3, 1, 2, 0, 1}, {-3, -2, -2, -2, 0, 0}, {-3, -2, -2, -2, 0, 0}, {0.5F, 0.5F, 1.5F, 0.5F, 3.5F, 2.5F});

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Shifts Truncate Toward Zero", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // 6x3x2 volume, consecutive mode. Each case pins one centroid delta against truncation:
  // rounding to nearest would give the value in the last column, truncation gives the expected one.
  struct TruncationCase
  {
    std::string name;
    std::vector<MaskCell> topSliceMask;    // physical slice 1, iteration index 0
    std::vector<MaskCell> bottomSliceMask; // physical slice 0, iteration index 1
    int64 expectedXShift = 0;
    int64 expectedYShift = 0;
  };

  // mean_x 2.0 vs 13/5 = 2.6 -> delta +/- 0.6 (rounding would give +/-1)
  const std::vector<MaskCell> k_Mean20{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
  const std::vector<MaskCell> k_Mean26{{2, 0}, {3, 0}, {4, 0}, {2, 1}, {2, 2}};
  // mean_x 0.5 vs 2.0 -> delta +/- 1.5 (rounding would give +/-2)
  const std::vector<MaskCell> k_Mean05{{0, 0}, {1, 0}};
  const std::vector<MaskCell> k_Mean20Pair{{1, 0}, {3, 0}};

  const std::vector<TruncationCase> cases{
      {"delta +0.6 truncates to 0", k_Mean20, k_Mean26, 0, 0},
      {"delta -0.6 truncates to 0", k_Mean26, k_Mean20, 0, 0},
      {"delta +1.5 truncates to +1", k_Mean05, k_Mean20Pair, 1, 0},
      {"delta -1.5 truncates to -1", k_Mean20Pair, k_Mean05, -1, 0},
  };

  const SizeVec3 geomDims{6, 3, 2};

  for(const auto& testCase : cases)
  {
    DYNAMIC_SECTION(testCase.name)
    {
      DataStructure dataStructure = CreateFixture(geomDims, k_UnitSpacing, MaskLayout{{1, testCase.topSliceMask}, {0, testCase.bottomSliceMask}});

      AlignSectionsFeatureCentroidFilter filter;
      Arguments args = CreateArgs(false, 0, true);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      const DataPath cumulativeShiftsPath = k_AlignmentAmPath.createChildPath(k_CumulativeShiftsName);
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(cumulativeShiftsPath));
      const auto& cumulativeShiftsRef = dataStructure.getDataRefAs<Int64Array>(cumulativeShiftsPath).getDataStoreRef();
      REQUIRE(cumulativeShiftsRef[2] == testCase.expectedXShift);
      REQUIRE(cumulativeShiftsRef[3] == testCase.expectedYShift);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Off Edge Push Zero Fills", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // Single in-mask voxel at (0,0) on the top slice and (4,4) on the Z-origin slice, so the
  // relative shift is trunc(4 - 0) = 4 in both directions. Only (0,0) has an in-bounds source.
  const SizeVec3 geomDims{5, 5, 2};
  DataStructure dataStructure = CreateFixture(geomDims, k_UnitSpacing, MaskLayout{{1, {{0, 0}}}, {0, {{4, 4}}}});

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  // |4| <= X and |4| <= Y, so the out-of-range diagnostics must stay silent.
  REQUIRE(executeResult.result.warnings().empty());

  // clang-format off
  const std::vector<int32> expectedPayload{
      // z = 0: only (0,0) has an in-bounds source, everything else is zero filled
      44, 0, 0, 0, 0,
       0, 0, 0, 0, 0,
       0, 0, 0, 0, 0,
       0, 0, 0, 0, 0,
       0, 0, 0, 0, 0,
      // z = 1: unchanged
      100, 101, 102, 103, 104,
      110, 111, 112, 113, 114,
      120, 121, 122, 123, 124,
      130, 131, 132, 133, 134,
      140, 141, 142, 143, 144,
  };
  // clang-format on
  CheckPayload(dataStructure, expectedPayload);
  // The mask is a cell array too, so it is shifted along with everything else.
  CheckMask(dataStructure, geomDims, MaskLayout{{0, {{0, 0}}}, {1, {{0, 0}}}});

  CheckShiftArrays(dataStructure, {0, 0, 0, 1}, {0, 0, 4, 4}, {0, 0, 4, 4}, {0.0F, 0.0F, 4.0F, 4.0F});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Fully Masked Out Slice Warns And Does Not Shift", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // A 5x5x4 stack. Physical slice 1 has no in-mask cells, so it must contribute a relative shift of
  // 0 and one Warning, and the last valid centroid (1.5, 1.5) must carry forward so the Z-origin
  // slice still chains off it: trunc(2.5 - 1.5) = 1.
  //   physical slice 3 (i=0): block (0,0)-(1,1) -> mean (0.5, 0.5), shift (0, 0)
  //   physical slice 2 (i=1): block (1,1)-(2,2) -> mean (1.5, 1.5), shift (1, 1)
  //   physical slice 1 (i=2): empty                                 shift (1, 1) (inherited)
  //   physical slice 0 (i=3): block (2,2)-(3,3) -> mean (2.5, 2.5), shift (2, 2)
  // The empty slice sits at physical index 1 but iteration index 2, so the Warning's slice number
  // also pins that the message reports the physical slice.
  const SizeVec3 geomDims{5, 5, 4};
  DataStructure dataStructure = CreateFixture(geomDims, k_UnitSpacing,
                                              MaskLayout{
                                                  {3, {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
                                                  {2, {{1, 1}, {2, 1}, {1, 2}, {2, 2}}},
                                                  {0, {{2, 2}, {3, 2}, {2, 3}, {3, 3}}},
                                              });

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(CountWarnings(executeResult.result, -53904) == 1);
  REQUIRE(executeResult.result.warnings()[0].message.find("Slice=1") != std::string::npos);
  REQUIRE(executeResult.result.warnings()[0].message.find("Slice=2") == std::string::npos);

  // clang-format off
  const std::vector<int32> expectedPayload{
      // z = 0, shift (2,2): out(x,y) = in(x+2, y+2) where the source is in bounds
       22,  23,  24,   0,   0,
       32,  33,  34,   0,   0,
       42,  43,  44,   0,   0,
        0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,
      // z = 1, the empty slice, shift (1,1) inherited from the slice above it
      111, 112, 113, 114,   0,
      121, 122, 123, 124,   0,
      131, 132, 133, 134,   0,
      141, 142, 143, 144,   0,
        0,   0,   0,   0,   0,
      // z = 2, shift (1,1)
      211, 212, 213, 214,   0,
      221, 222, 223, 224,   0,
      231, 232, 233, 234,   0,
      241, 242, 243, 244,   0,
        0,   0,   0,   0,   0,
      // z = 3, the anchor slice, untouched
      300, 301, 302, 303, 304,
      310, 311, 312, 313, 314,
      320, 321, 322, 323, 324,
      330, 331, 332, 333, 334,
      340, 341, 342, 343, 344,
  };
  // clang-format on
  CheckPayload(dataStructure, expectedPayload);

  // The three populated blocks all end up stacked at x in {0,1}, y in {0,1}; the empty slice stays
  // empty.
  const std::vector<MaskCell> block{{0, 0}, {1, 0}, {0, 1}, {1, 1}};
  CheckMask(dataStructure, geomDims, MaskLayout{{0, block}, {2, block}, {3, block}});

  // An empty slice reports a centroid of 0 alongside its Warning.
  CheckShiftArrays(dataStructure, {0, 0, 2, 3, 1, 2, 0, 1}, {0, 0, 1, 1, 0, 0, 1, 1}, {0, 0, 1, 1, 1, 1, 2, 2}, {0.0F, 0.0F, 1.5F, 1.5F, 0.0F, 0.0F, 2.5F, 2.5F});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Fully Masked Out Reference Slice Is An Error", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // Physical slice 1 is empty and is also the requested alignment target, so there is nothing to
  // align to and the filter must fail at execute time rather than divide by a zero cell count.
  const SizeVec3 geomDims{5, 5, 4};
  DataStructure dataStructure = CreateFixture(geomDims, k_UnitSpacing,
                                              MaskLayout{
                                                  {3, {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
                                                  {2, {{1, 1}, {2, 1}, {1, 2}, {2, 2}}},
                                                  {0, {{2, 2}, {3, 2}, {2, 3}, {3, 3}}},
                                              });

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(true, 1, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(HasError(executeResult.result, -53901));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Non 3D Geometry Is Rejected", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<std::pair<std::string, SizeVec3>> cases{
      {"single slice (Z = 1)", SizeVec3{5, 5, 1}},
      {"single column (X = 1)", SizeVec3{1, 5, 5}},
      {"single row (Y = 1)", SizeVec3{5, 1, 5}},
  };

  for(const auto& [label, geomDims] : cases)
  {
    DYNAMIC_SECTION(label)
    {
      DataStructure dataStructure = CreateFixture(geomDims, k_UnitSpacing, MaskLayout{{0, {{0, 0}}}});

      AlignSectionsFeatureCentroidFilter filter;
      Arguments args = CreateArgs(false, 0, false);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
      REQUIRE(HasError(preflightResult.outputActions, -68072));

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Reference Slice Bounds", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  struct BoundsCase
  {
    std::string name;
    bool useReferenceSlice = true;
    int32 referenceSlice = 0;
    int32 expectedErrorCode = 0; // 0 == expect a valid preflight
  };

  // The fixture has Z = 3, so the valid physical slice indices are 0, 1 and 2.
  const std::vector<BoundsCase> cases{
      {"slice 0 is valid", true, 0, 0},
      {"slice Z-1 is valid", true, 2, 0},
      {"slice Z is out of range", true, 3, -68071},
      {"slice beyond Z is out of range", true, 6, -68071},
      {"negative slice is rejected", true, -1, -68064},
      {"negative slice is ignored when unused", false, -1, 0},
      {"out of range slice is ignored when unused", false, 6, 0},
  };

  for(const auto& testCase : cases)
  {
    DYNAMIC_SECTION(testCase.name)
    {
      DataStructure dataStructure = CreateFixture(k_StackDims, k_UnitSpacing, StackedBlockLayout());

      AlignSectionsFeatureCentroidFilter filter;
      Arguments args = CreateArgs(testCase.useReferenceSlice, testCase.referenceSlice, false);

      auto preflightResult = filter.preflight(dataStructure, args);
      if(testCase.expectedErrorCode == 0)
      {
        SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
      }
      else
      {
        SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
        REQUIRE(HasError(preflightResult.outputActions, testCase.expectedErrorCode));
      }

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: UInt8 Mask Parity", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // Identical layout to the consecutive-mode case, with the mask stored as uint8 instead of bool.
  DataStructure dataStructure = CreateFixture(k_StackDims, k_UnitSpacing, StackedBlockLayout(), MaskStorage::UInt8);

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CheckPayload(dataStructure, k_ConsecutivePayload);
  CheckShiftArrays(dataStructure, {0, 0, 1, 2, 0, 1}, {0, 0, 1, 0, 2, 2}, {0, 0, 1, 0, 3, 2}, {0.0F, 0.0F, 1.5F, 0.5F, 3.5F, 2.5F});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Non Unit Spacing Invariance", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // Centroids are accumulated in real units and the shift divides by the same spacing, so the voxel
  // shifts must match the unit-spacing case exactly while the stored centroids scale.
  const FloatVec3 spacing{0.5F, 2.0F, 1.0F};
  DataStructure dataStructure = CreateFixture(k_StackDims, spacing, StackedBlockLayout());

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CheckPayload(dataStructure, k_ConsecutivePayload);
  // 0.5 * 1.5 = 0.75, 2.0 * 0.5 = 1.0, 0.5 * 3.5 = 1.75, 2.0 * 2.5 = 5.0
  CheckShiftArrays(dataStructure, {0, 0, 1, 2, 0, 1}, {0, 0, 1, 0, 2, 2}, {0, 0, 1, 0, 3, 2}, {0.0F, 0.0F, 0.75F, 1.0F, 1.75F, 5.0F});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Accumulated Shift Beyond The X Dimension Warns", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // A 2x2x13 stack whose per-slice mean_x cycles 1.0 -> 2/3 -> 1/3 -> 0 from the top down. Each
  // -1/3 step truncates to 0 while every return to 1.0 truncates to +1, so the cumulative X shift
  // banks one voxel per four slices and reaches 3 - one more than the X dimension - at the last
  // slice. mean_y cycles 0 -> 1/3 -> 1/3 -> 0, so every Y step truncates to 0.
  const SizeVec3 geomDims{2, 2, 13};
  const std::vector<MaskCell> k_MeanX10{{1, 0}};                        // mean_x 1.0,   mean_y 0
  const std::vector<MaskCell> k_MeanXTwoThirds{{0, 0}, {1, 0}, {1, 1}}; // mean_x 2/3, mean_y 1/3
  const std::vector<MaskCell> k_MeanXOneThird{{0, 0}, {0, 1}, {1, 0}};  // mean_x 1/3, mean_y 1/3
  const std::vector<MaskCell> k_MeanX00{{0, 0}};                        // mean_x 0,     mean_y 0

  MaskLayout maskLayout;
  for(usize iter = 0; iter < geomDims[2]; iter++)
  {
    const usize slice = geomDims[2] - 1 - iter;
    switch(iter % 4)
    {
    case 0:
      maskLayout[slice] = k_MeanX10;
      break;
    case 1:
      maskLayout[slice] = k_MeanXTwoThirds;
      break;
    case 2:
      maskLayout[slice] = k_MeanXOneThird;
      break;
    default:
      maskLayout[slice] = k_MeanX00;
      break;
    }
  }

  DataStructure dataStructure = CreateFixture(geomDims, k_UnitSpacing, maskLayout);

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(CountWarnings(executeResult.result, -53902) == 1);

  const std::vector<int64> expectedCumulativeX{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3};
  const DataPath cumulativeShiftsPath = k_AlignmentAmPath.createChildPath(k_CumulativeShiftsName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(cumulativeShiftsPath));
  const auto& cumulativeShiftsRef = dataStructure.getDataRefAs<Int64Array>(cumulativeShiftsPath).getDataStoreRef();
  for(usize iter = 0; iter < expectedCumulativeX.size(); iter++)
  {
    CAPTURE(iter);
    REQUIRE(cumulativeShiftsRef[iter * 2] == expectedCumulativeX[iter]);
    REQUIRE(cumulativeShiftsRef[(iter * 2) + 1] == 0);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Non Data Array Cell Child Is Rejected", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  UnitTest::LoadPlugins();

  // StringArray and NeighborList derive from IArray but not IDataArray, and the shared transfer
  // step casts every cell child to IDataArray. Preflight has to catch this before the cast throws.
  DataStructure dataStructure = CreateFixture(k_StackDims, k_UnitSpacing, StackedBlockLayout());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_CellAmPath));
  const auto& cellAm = dataStructure.getDataRefAs<AttributeMatrix>(k_CellAmPath);
  const std::vector<std::string> stringValues(k_StackDims[0] * k_StackDims[1] * k_StackDims[2], "value");
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Notes", ShapeType{k_StackDims[2], k_StackDims[1], k_StackDims[0]}, stringValues, cellAm.getId()) != nullptr);

  AlignSectionsFeatureCentroidFilter filter;
  Arguments args = CreateArgs(false, 0, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(HasError(preflightResult.outputActions, -68073));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: SIMPL Backwards Compatibility", "[SimplnxCore][AlignSectionsFeatureCentroidFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "AlignSectionsFeatureCentroidFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "AlignSectionsFeatureCentroidFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<AlignSectionsFeatureCentroidFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(AlignSectionsFeatureCentroidFilter::k_StoreAlignmentShifts_Key) == true);
      CHECK(args.value<bool>(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key) == true);
      CHECK(args.value<int32>(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key) == 5);
      CHECK(args.value<DataPath>(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
