#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ErodeDilateMask.hpp"
#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_ImageGeomName("Image");
const std::string k_CellDataName("CellData");
const std::string k_MaskName("Mask");

namespace MaskFixture
{
constexpr usize k_XDim = 5;
constexpr usize k_YDim = 5;
constexpr usize k_ZDim = 1;
constexpr usize k_CellCount = k_XDim * k_YDim * k_ZDim;

constexpr usize k_CubeDim = 3;
constexpr usize k_CubeCellCount = k_CubeDim * k_CubeDim * k_CubeDim;

constexpr usize GetIndex(usize x, usize y)
{
  return y * k_XDim + x;
}

constexpr usize GetCubeIndex(usize x, usize y, usize z)
{
  return (z * k_CubeDim * k_CubeDim) + (y * k_CubeDim) + x;
}

inline DataPath GeomPath()
{
  return DataPath({k_ImageGeomName});
}

inline DataPath MaskPath()
{
  return DataPath({k_ImageGeomName, k_CellDataName, k_MaskName});
}

/**
 * @brief Creates an Image Geometry with a single component boolean mask array attached to its cell data.
 * @param xDim Number of columns
 * @param yDim Number of rows
 * @param zDim Number of planes
 * @param initialValue Value written into every element of the mask
 */
inline DataStructure CreateMaskedImage(usize xDim, usize yDim, usize zDim, bool initialValue)
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeomPtr->setDimensions({xDim, yDim, zDim});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});

  const ShapeType tupleShape = {zDim, yDim, xDim};
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, k_CellDataName, tupleShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto* maskPtr = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_MaskName, tupleShape, ShapeType{1}, cellAmPtr->getId());
  auto& maskRef = maskPtr->getDataStoreRef();
  for(usize i = 0; i < xDim * yDim * zDim; i++)
  {
    maskRef[i] = initialValue;
  }

  return dataStructure;
}

// 5x5x1 grid, single true voxel at (seedX, seedY).
inline DataStructure CreateSeededFixture(usize seedX, usize seedY)
{
  DataStructure dataStructure = CreateMaskedImage(k_XDim, k_YDim, k_ZDim, false);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(MaskPath()));
  auto& maskRef = dataStructure.getDataRefAs<BoolArray>(MaskPath()).getDataStoreRef();
  maskRef[GetIndex(seedX, seedY)] = true;

  return dataStructure;
}

// 5x5x1 grid, single true voxel at (2, 2).
inline DataStructure CreateFixture()
{
  return CreateSeededFixture(2, 2);
}

// 3x3x3 grid, true everywhere except the centre voxel (1, 1, 1).
inline DataStructure CreateHollowCubeFixture()
{
  DataStructure dataStructure = CreateMaskedImage(k_CubeDim, k_CubeDim, k_CubeDim, true);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(MaskPath()));
  auto& maskRef = dataStructure.getDataRefAs<BoolArray>(MaskPath()).getDataStoreRef();
  maskRef[GetCubeIndex(1, 1, 1)] = false;

  return dataStructure;
}

inline Arguments MakeArgs(ChoicesParameter::ValueType operation, int32 numIterations, bool xDirOn, bool yDirOn, bool zDirOn)
{
  Arguments args;
  args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(xDirOn));
  args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(yDirOn));
  args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(zDirOn));
  args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(GeomPath()));
  args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(MaskPath()));

  return args;
}

inline void RunFilter(DataStructure& dataStructure, ChoicesParameter::ValueType operation, int32 numIterations, bool xDirOn, bool yDirOn, bool zDirOn)
{
  const ErodeDilateMaskFilter filter;
  const Arguments args = MakeArgs(operation, numIterations, xDirOn, yDirOn, zDirOn);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

inline void CheckMask(const DataStructure& dataStructure, usize cellCount, const std::vector<usize>& expectedTrueIndices)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(MaskPath()));
  const auto& maskRef = dataStructure.getDataRefAs<BoolArray>(MaskPath()).getDataStoreRef();
  REQUIRE(maskRef.getNumberOfTuples() == cellCount);
  for(usize i = 0; i < cellCount; i++)
  {
    CAPTURE(i);
    const bool expected = std::find(expectedTrueIndices.cbegin(), expectedTrueIndices.cend(), i) != expectedTrueIndices.cend();
    REQUIRE(maskRef[i] == expected);
  }
}

/**
 * @brief Builds the complement of falseIndices over [0, cellCount). Convenient when the expected
 * output is "mostly true", as it is for the hollow-cube erode cases.
 */
inline std::vector<usize> AllIndicesExcept(usize cellCount, const std::vector<usize>& falseIndices)
{
  std::vector<usize> trueIndices;
  for(usize i = 0; i < cellCount; i++)
  {
    if(std::find(falseIndices.cbegin(), falseIndices.cend(), i) == falseIndices.cend())
    {
      trueIndices.push_back(i);
    }
  }
  return trueIndices;
}
} // namespace MaskFixture
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, one iteration", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = MaskFixture::CreateFixture();

  MaskFixture::RunFilter(dataStructure, nx::core::detail::k_DilateIndex, 1, true, true, true);

  // Hand derivation. Dilate walks every voxel where mask[voxelIndex] == false and sets
  // maskCopy[voxelIndex] = true if any in-bounds face neighbour is true. The only true voxel in the
  // input is (2, 2), so exactly the four false voxels that touch it flip to true:
  //   (1, 2) -> index 11, (3, 2) -> index 13, (2, 1) -> index 7, (2, 3) -> index 17.
  // The seed itself is never re-examined (mask[12] is true, so the !mask test skips it) and it is
  // copied forward unchanged by the maskCopy initialization. The grid is one plane thick, so
  // computeValidFaceNeighbors reports -Z and +Z invalid for every voxel and no Z growth is possible.
  // Expected true set: {7, 11, 12, 13, 17} -- a plus shape centred on (2, 2).
  MaskFixture::CheckMask(dataStructure, MaskFixture::k_CellCount,
                         {MaskFixture::GetIndex(2, 2), MaskFixture::GetIndex(1, 2), MaskFixture::GetIndex(3, 2), MaskFixture::GetIndex(2, 1), MaskFixture::GetIndex(2, 3)});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 erode, one iteration", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = MaskFixture::CreateFixture();

  MaskFixture::RunFilter(dataStructure, nx::core::detail::k_ErodeIndex, 1, true, true, true);

  // Hand derivation. Erode also visits only the false voxels, but it clears the *neighbour*:
  // for a false voxel with a true face neighbour it sets maskCopy[neighpoint] = false. The four
  // false voxels (1, 2), (3, 2), (2, 1), (2, 3) each see the true seed (2, 2) and each clear it.
  // No other voxel has a true neighbour, so nothing else is written. Expected true set: {} --
  // an isolated single-voxel region cannot survive one erode iteration.
  MaskFixture::CheckMask(dataStructure, MaskFixture::k_CellCount, {});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate honours direction flags", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = MaskFixture::CreateFixture();

  // X disabled: growth is confined to Y, producing a vertical 3-cell bar.
  MaskFixture::RunFilter(dataStructure, nx::core::detail::k_DilateIndex, 1, false, true, true);

  // Hand derivation. With XDirOn == false the -X and +X face neighbours are masked out, so the
  // false voxels (1, 2) and (3, 2) can no longer reach the seed at (2, 2) and stay false. The
  // false voxels (2, 1) and (2, 3) still reach it through their +Y and -Y faces and flip to true.
  // Z is off the table regardless because the grid is one plane thick.
  // Expected true set: {7, 12, 17} -- the vertical bar (2, 1), (2, 2), (2, 3).
  MaskFixture::CheckMask(dataStructure, MaskFixture::k_CellCount, {MaskFixture::GetIndex(2, 2), MaskFixture::GetIndex(2, 1), MaskFixture::GetIndex(2, 3)});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate with Y direction off", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = MaskFixture::CreateFixture();

  // Y disabled, X and Z enabled: the mirror image of the X-off case, and the axis the fixed
  // regression got wrong most easily. A build that gates only on X and Z -- or that maps the Y flag
  // onto the wrong pair of entries in isValidFaceNeighbor -- produces the full plus shape here while
  // still passing the X-off test, so this case is what pins the Y flag specifically.
  MaskFixture::RunFilter(dataStructure, nx::core::detail::k_DilateIndex, 1, true, false, true);

  // Hand derivation. Grid index = z*25 + y*5 + x on the 5x5x1 grid, so z is always 0 and
  // index = y*5 + x. The single true voxel is the seed (2, 2) -> index 12.
  // adjustValidNeighbors ANDs the 6-entry [-Z,-Y,-X,+X,+Y,+Z] validity array with the axis flags, so
  // with YDirOn == false the -Y and +Y entries are cleared; the -Z and +Z entries are already false
  // for every voxel because the grid is one plane thick (computeValidFaceNeighbors returns zIdx > 0
  // and zIdx < dims[2] - 1, both false when dims[2] == 1). Only the -X and +X faces remain enabled.
  // Dilate visits each false voxel and sets it true if an enabled valid face neighbour is true:
  //   index 11 = (1, 2): +X neighbour is 11 + 1 = 12, true  -> flips to true.
  //   index 13 = (3, 2): -X neighbour is 13 - 1 = 12, true  -> flips to true.
  //   index  7 = (2, 1): reaches 12 only through +Y (7 + 5), which is disabled -> stays false.
  //   index 17 = (2, 3): reaches 12 only through -Y (17 - 5), which is disabled -> stays false.
  //   Every other false voxel has no true neighbour at all.
  // The seed is skipped by the !mask test and carried forward by the maskCopy seeding.
  // Expected true set: {11, 12, 13} -- a horizontal 3-cell bar (1, 2), (2, 2), (3, 2).
  MaskFixture::CheckMask(dataStructure, MaskFixture::k_CellCount, {MaskFixture::GetIndex(1, 2), MaskFixture::GetIndex(2, 2), MaskFixture::GetIndex(3, 2)});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, two iterations", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = MaskFixture::CreateFixture();

  // Two iterations exercise the inter-iteration dataflow: the per-iteration maskCopy seeding must
  // happen inside the iteration loop (so pass 2 starts from pass 1's result, not from the original
  // input) and the write-back must also happen inside it (so pass 2 can see pass 1's result at all).
  // Hoisting either one out of the loop leaves the single-iteration results untouched and is only
  // visible from two iterations onward.
  MaskFixture::RunFilter(dataStructure, nx::core::detail::k_DilateIndex, 2, true, true, true);

  // Hand derivation. Index = y*5 + x; ±Z is invalid everywhere (one plane thick); all flags on, so
  // the enabled neighbours of a voxel are its in-bounds -Y, -X, +X, +Y faces.
  //
  // Iteration 1 (input: seed 12 only). Dilate flips every false voxel with a true enabled neighbour:
  //   11 (-> +X = 12), 13 (-> -X = 12), 7 (-> +Y = 12), 17 (-> -Y = 12).
  //   mask after write-back = {7, 11, 12, 13, 17}.
  //
  // Iteration 2 reads that mask. Walking the 20 false voxels one at a time (neighbours listed as
  // -Y, -X, +X, +Y, omitting the ones the boundary check rejects):
  //    0 (0,0): +X=1, +Y=5                     -> all false, stays false
  //    1 (1,0): -X=0, +X=2, +Y=6               -> all false, stays false
  //    2 (2,0): -X=1, +X=3, +Y=7  (7 is TRUE)  -> TRUE
  //    3 (3,0): -X=2, +X=4, +Y=8               -> all false, stays false
  //    4 (4,0): -X=3, +Y=9                     -> all false, stays false
  //    5 (0,1): -Y=0, +X=6, +Y=10              -> all false, stays false
  //    6 (1,1): -Y=1, -X=5, +X=7  (7 is TRUE)  -> TRUE
  //    8 (3,1): -Y=3, -X=7 (TRUE)              -> TRUE
  //    9 (4,1): -Y=4, -X=8, +Y=14              -> all false, stays false
  //   10 (0,2): -Y=5, +X=11 (TRUE)             -> TRUE
  //   14 (4,2): -Y=9, -X=13 (TRUE)             -> TRUE
  //   15 (0,3): -Y=10, +X=16, +Y=20            -> all false, stays false
  //   16 (1,3): -Y=11 (TRUE)                   -> TRUE
  //   18 (3,3): -Y=13 (TRUE)                   -> TRUE
  //   19 (4,3): -Y=14, -X=18, +Y=24            -> all false, stays false
  //   20 (0,4): -Y=15, +X=21                   -> all false, stays false
  //   21 (1,4): -Y=16, -X=20, +X=22            -> all false, stays false (16 and 22 only turn true
  //                                               in maskCopy this pass; mask is read, not maskCopy)
  //   22 (2,4): -Y=17 (TRUE)                   -> TRUE
  //   23 (3,4): -Y=18, -X=22, +X=24            -> all false, stays false
  //   24 (4,4): -Y=19, -X=23                   -> all false, stays false
  // The five voxels already true (7, 11, 12, 13, 17) are skipped and carried forward.
  // Expected true set: {2, 6, 7, 8, 10, 11, 12, 13, 14, 16, 17, 18, 22} -- the 13-cell diamond of
  // every cell within a Manhattan distance of 2 of (2, 2) that fits on the grid.
  MaskFixture::CheckMask(dataStructure, MaskFixture::k_CellCount, {2, 6, 7, 8, 10, 11, 12, 13, 14, 16, 17, 18, 22});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate from a boundary seed", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();

  // Why this fixture, and why the seed sits on the x == 0 face.
  //
  // The other fixtures are centred, and a centred seed cannot detect a swap of the -X and +X entries
  // of neighborVoxelIndexOffsets. computeValidFaceNeighbors gates by *coordinate*, not by offset:
  // entry k_NegativeXNeighbor is enabled when xIdx > 0 and entry k_PositiveXNeighbor when
  // xIdx < dims[0] - 1. For an interior seed at (2, 2) both gates are open for both of its X
  // neighbours, so a swapped offset pair merely re-labels which entry reaches index 12; the set of
  // voxels that reach the seed -- {11, 13} -- is unchanged and the test still passes. The same is
  // true of an off-centre but interior seed such as (1, 2): a plus shape is mirror-symmetric about
  // its own centre no matter where that centre is.
  //
  // Putting the seed on the x == 0 face breaks the symmetry through the *gate* rather than through
  // the shape. With the seed at (0, 2) = index 10, a swapped ±X offset pair means voxel 9 = (4, 1)
  // -- whose -X gate is open because 4 > 0 -- would read offset +1 and land on index 10, so the
  // mutant grows a cell that wrapped around the row end. Correct code has 9 read index 8, which is
  // false. So index 9 is the discriminator, and no assertion in the committed suite covered it.
  // This is also the only case in the suite where the seed itself sits against a grid face, so it
  // exercises the boundary rejection on the seed's own missing -X neighbour.
  DataStructure dataStructure = MaskFixture::CreateSeededFixture(0, 2);

  MaskFixture::RunFilter(dataStructure, nx::core::detail::k_DilateIndex, 1, true, true, true);

  // Hand derivation. Index = y*5 + x; the seed (0, 2) is index 10; ±Z invalid everywhere.
  // Dilate flips each false voxel that has a true enabled valid neighbour, so exactly the in-bounds
  // face neighbours of index 10 flip:
  //   index  5 = (0, 1): +Y neighbour is 5 + 5 = 10, true -> flips to true.
  //   index 11 = (1, 2): -X neighbour is 11 - 1 = 10, true -> flips to true.
  //   index 15 = (0, 3): -Y neighbour is 15 - 5 = 10, true -> flips to true.
  // The seed has no -X neighbour of its own (xIdx == 0 closes that gate), and index 9 = (4, 1) is
  // *not* adjacent to the seed in the grid even though 9 and 10 are adjacent in flat storage -- its
  // -X neighbour is index 8. Index 9 must stay false.
  // Expected true set: {5, 10, 11, 15}.
  MaskFixture::CheckMask(dataStructure, MaskFixture::k_CellCount, {MaskFixture::GetIndex(0, 1), MaskFixture::GetIndex(0, 2), MaskFixture::GetIndex(1, 2), MaskFixture::GetIndex(0, 3)});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Class 1 erode honours direction flags", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();

  // Hand derivation, 3x3x3 grid seeded true everywhere except the centre (1, 1, 1).
  // Flat index = (z * 9) + (y * 3) + x, so the centre is index 13 and its six face neighbours are
  //   -Z (1, 1, 0) = 4    -Y (1, 0, 1) = 10   -X (0, 1, 1) = 12
  //   +X (2, 1, 1) = 14   +Y (1, 2, 1) = 16   +Z (1, 1, 2) = 22
  // The centre is the only false voxel, so it is the only voxel that enters the neighbour loop, and
  // every enabled face neighbour of it is true and therefore gets cleared in maskCopy. Voxel 13 is
  // already false and stays false.
  const usize k_Centre = MaskFixture::GetCubeIndex(1, 1, 1);
  const usize k_NegZ = MaskFixture::GetCubeIndex(1, 1, 0);
  const usize k_NegY = MaskFixture::GetCubeIndex(1, 0, 1);
  const usize k_NegX = MaskFixture::GetCubeIndex(0, 1, 1);
  const usize k_PosX = MaskFixture::GetCubeIndex(2, 1, 1);
  const usize k_PosY = MaskFixture::GetCubeIndex(1, 2, 1);
  const usize k_PosZ = MaskFixture::GetCubeIndex(1, 1, 2);

  SECTION("All directions on")
  {
    DataStructure dataStructure = MaskFixture::CreateHollowCubeFixture();

    MaskFixture::RunFilter(dataStructure, nx::core::detail::k_ErodeIndex, 1, true, true, true);

    // All six neighbours are cleared: false set = {4, 10, 12, 13, 14, 16, 22}, 20 voxels stay true.
    MaskFixture::CheckMask(dataStructure, MaskFixture::k_CubeCellCount, MaskFixture::AllIndicesExcept(MaskFixture::k_CubeCellCount, {k_Centre, k_NegZ, k_NegY, k_NegX, k_PosX, k_PosY, k_PosZ}));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Z direction off")
  {
    DataStructure dataStructure = MaskFixture::CreateHollowCubeFixture();

    MaskFixture::RunFilter(dataStructure, nx::core::detail::k_ErodeIndex, 1, true, true, false);

    // With ZDirOn == false the -Z and +Z faces are masked out, so voxels 4 and 22 survive:
    // false set = {10, 12, 13, 14, 16}, 22 voxels stay true.
    MaskFixture::CheckMask(dataStructure, MaskFixture::k_CubeCellCount, MaskFixture::AllIndicesExcept(MaskFixture::k_CubeCellCount, {k_Centre, k_NegY, k_NegX, k_PosX, k_PosY}));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();

  // The erode/dilate loop runs `for(i = 0; i < NumIterations; i++)`, so any value below 1 performs no
  // passes at all and the mask is returned untouched. DREAM3D 6.5.171 rejected that configuration in
  // dataCheck with error -5555 rather than silently doing nothing; SIMPLNX now restores the guard so a
  // mistyped iteration count is reported instead of producing a pass-through result that looks like a
  // successful run. Both the boundary value (0) and a negative value are checked.
  const int32 numIterations = GENERATE(0, -2);
  CAPTURE(numIterations);

  DataStructure dataStructure = MaskFixture::CreateFixture();

  const ErodeDilateMaskFilter filter;
  const Arguments args = MaskFixture::MakeArgs(nx::core::detail::k_DilateIndex, numIterations, true, true, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  REQUIRE(preflightResult.outputActions.errors()[0].code == -14701);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ErodeDilateMaskFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ErodeDilateMaskFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ErodeDilateMaskFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ErodeDilateMaskFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ErodeDilateMaskFilter::k_Operation_Key) == 0);
      CHECK(args.value<int32>(ErodeDilateMaskFilter::k_NumIterations_Key) == 5);
      CHECK(args.value<bool>(ErodeDilateMaskFilter::k_XDirOn_Key) == true);
      CHECK(args.value<bool>(ErodeDilateMaskFilter::k_YDirOn_Key) == true);
      CHECK(args.value<bool>(ErodeDilateMaskFilter::k_ZDirOn_Key) == true);
      CHECK(args.value<DataPath>(ErodeDilateMaskFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
