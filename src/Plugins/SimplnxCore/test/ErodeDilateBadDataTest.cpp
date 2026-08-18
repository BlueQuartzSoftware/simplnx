#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ErodeDilateBadData.hpp"
#include "SimplnxCore/Filters/ErodeDilateBadDataFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const StringLiteral k_MiscData = "Misc";

constexpr usize k_NumTuples = 32;
// ImageGeom dimensions are ordered X, Y, Z. AttributeMatrix and DataArray tuple shapes are ordered
// slowest-to-fastest, i.e. Z, Y, X. Both describe the same 32 cells, so keep them as separate constants
// rather than reusing one that happens to multiply out to the same tuple count.
const SizeVec3 k_GeometryDimensions{4, 4, 2};
const ShapeType k_TupleShape{k_GeometryDimensions[2], k_GeometryDimensions[1], k_GeometryDimensions[0]};
const DataPath k_DataPath({::k_ImageGeometry, ::k_CellData, k_MiscData});
const DataPath k_ImageFeatureIdsPath({::k_ImageGeometry, ::k_CellData, k_FeatureIds});

using DirectionType = std::array<bool, 3>;
constexpr DirectionType k_XDir{true, false, false};
constexpr DirectionType k_XYDir{true, true, false};
constexpr DirectionType k_XYZDir{true, true, true};
constexpr DirectionType k_XZDir{true, false, true};
constexpr DirectionType k_YDir{false, true, false};
constexpr DirectionType k_YZDir{false, true, true};
constexpr DirectionType k_ZDir{false, false, true};

using ExemplarDataType = std::array<int32, k_NumTuples>;
// Exemplar Dilate data for A/B testing.
//
// PROVENANCE: these constants are genuine DREAM3D 6.5.171 output. They were captured in 2026-08 during the
// PR #1687 V&V pass by running the legacy `ErodeDilateBadData` filter through `PipelineRunner` against a
// hand-verified HDF5 twin of the fixture built by `CreateTestData()`, for all 28 combinations of
// {Dilate, Erode} x {X, XY, XYZ, XZ, Y, YZ, Z} x {1, 2 iterations}, and diffing the results with h5py.
// That makes them a Class 2 (reference implementation) oracle. Full provenance:
// `src/Plugins/SimplnxCore/vv/ErodeDilateBadDataFilter.md`.
//
// WARNING: Never regenerate these from SIMPLNX output — that would make the test circular. If the fixture
// in `CreateTestData()` changes, the constants must be re-captured from the legacy binary, not from a
// SIMPLNX run.
constexpr ExemplarDataType k_ExemplarFeatureIdsDilateX1{0, 0, 1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateX1{0, 0, 2, 3, 4, 5, 6, 7, 8, 10, 10, 10, 13, 13, 14, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateX2{0, 0, 0, 2, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateX2{0, 0, 0, 3, 4, 5, 6, 7, 10, 10, 10, 10, 13, 13, 14, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 31, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateXY1{0, 0, 1, 2, 0, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateXY1{0, 0, 2, 3, 0, 5, 10, 7, 8, 13, 10, 10, 13, 13, 14, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateXY2{0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 0, 0, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateXY2{0, 0, 10, 3, 0, 13, 10, 10, 13, 13, 10, 10, 13, 13, 14, 14, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 31, 31, 28, 31, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateXYZ1{0, 0, 1, 2, 0, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 3, 3, 3, 3, 5, 5, 0, 0, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateXYZ1{0, 0, 2, 3, 0, 5, 10, 7, 8, 13, 10, 10, 13, 13, 14, 31, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 10, 31, 28, 13, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateXYZ2{0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 3, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateXYZ2{0, 0, 10, 3, 0, 13, 10, 10, 13, 13, 10, 10, 13, 13, 14, 31, 0, 0, 18, 19, 0, 21, 10, 31, 24, 13, 10, 31, 13, 13, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateXZ1{0, 0, 1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 3, 3, 3, 3, 5, 5, 0, 5, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateXZ1{0, 0, 2, 3, 4, 5, 6, 7, 8, 10, 10, 10, 13, 13, 14, 31, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 10, 27, 28, 13, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateXZ2{0, 0, 0, 2, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 3, 3, 3, 3, 5, 0, 0, 0, 0, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateXZ2{0, 0, 0, 3, 4, 5, 6, 7, 10, 10, 10, 10, 13, 13, 14, 31, 0, 0, 18, 19, 20, 21, 22, 23, 24, 10, 10, 10, 13, 13, 31, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateY1{0, 1, 1, 2, 0, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};
constexpr ExemplarDataType k_ExemplarDataDilateY1{0, 1, 2, 3, 0, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateY2{0, 1, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 0, 0, 3, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};
constexpr ExemplarDataType k_ExemplarDataDilateY2{0, 1, 10, 3, 0, 13, 10, 7, 0, 13, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateYZ1{0, 1, 1, 2, 0, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 0, 4, 4, 4, 3, 3, 3, 3, 5, 5, 0, 0, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateYZ1{0, 1, 2, 3, 0, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 10, 31, 28, 13, 14, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateYZ2{0, 1, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 4, 4, 4, 0, 3, 0, 0, 5, 0, 0, 0, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateYZ2{0, 1, 10, 3, 0, 13, 10, 7, 0, 13, 10, 31, 12, 13, 14, 31, 0, 17, 18, 19, 0, 21, 10, 31, 24, 13, 10, 31, 28, 13, 14, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateZ1{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 0, 2, 2, 0, 0, 0, 0, 4, 4, 4, 3, 3, 3, 3, 5, 5, 0, 5, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateZ1{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 10, 27, 28, 13, 14, 31};

constexpr ExemplarDataType k_ExemplarFeatureIdsDilateZ2{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 0, 2, 2, 0, 0, 0, 0, 4, 4, 4, 3, 3, 3, 3, 5, 5, 0, 5, 5, 0, 0, 0};
constexpr ExemplarDataType k_ExemplarDataDilateZ2{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 10, 27, 28, 13, 14, 31};

// Exemplar Erode data for A/B testing.
//
// PROVENANCE: same capture as the Dilate block above — genuine DREAM3D 6.5.171 `PipelineRunner` output over
// the 28 direction/operation/iteration combinations, captured 2026-08 during PR #1687's V&V, recorded in
// `src/Plugins/SimplnxCore/vv/ErodeDilateBadDataFilter.md`.
//
// WARNING: Never regenerate these from SIMPLNX output — that would make the test circular.
constexpr ExemplarDataType k_ExemplarFeatureIdsErodeX1{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 6};
constexpr ExemplarDataType k_ExemplarDataErodeX1{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 11, 12, 12, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 30};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeX2{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 6};
constexpr ExemplarDataType k_ExemplarDataErodeX2{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 11, 12, 12, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 30};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeXY1{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 5};
constexpr ExemplarDataType k_ExemplarDataErodeXY1{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 11, 12, 9, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 27};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeXY2{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 5};
constexpr ExemplarDataType k_ExemplarDataErodeXY2{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 11, 12, 9, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 27};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeXYZ1{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeXYZ1{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 11, 12, 9, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeXYZ2{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeXYZ2{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 11, 12, 9, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeXZ1{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeXZ1{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 11, 12, 12, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeXZ2{1, 1, 1, 2, 2, 1, 2, 2, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeXZ2{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 11, 12, 12, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeY1{2, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 5};
constexpr ExemplarDataType k_ExemplarDataErodeY1{4, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 27};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeY2{2, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 5};
constexpr ExemplarDataType k_ExemplarDataErodeY2{4, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 27};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeYZ1{2, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 6, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeYZ1{4, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 30, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeYZ2{2, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 6, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeYZ2{4, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 30, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeZ1{4, 1, 1, 2, 2, 1, 2, 2, 1, 1, 5, 2, 2, 6, 6, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeZ1{16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 26, 11, 12, 29, 30, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

constexpr ExemplarDataType k_ExemplarFeatureIdsErodeZ2{4, 1, 1, 2, 2, 1, 2, 2, 1, 1, 5, 2, 2, 6, 6, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};
constexpr ExemplarDataType k_ExemplarDataErodeZ2{16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 26, 11, 12, 29, 30, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

/**
 * @brief The expected FeatureIds and Misc output for a single (operation, directions, iterations) combination.
 */
struct ExemplarRecord
{
  ChoicesParameter::ValueType operation;
  DirectionType directions;
  int32 iterations;
  const ExemplarDataType& expectedFeatureIds;
  const ExemplarDataType& expectedData;
};

// One row per valid parameter combination: 2 operations x 7 direction combinations x 2 iteration counts.
// The all-directions-off combination is rejected by preflight and therefore has no row.
// clang-format off
const std::array<ExemplarRecord, 28> k_Exemplars{{
    {k_Dilate, k_XDir, 1, k_ExemplarFeatureIdsDilateX1, k_ExemplarDataDilateX1},
    {k_Dilate, k_XDir, 2, k_ExemplarFeatureIdsDilateX2, k_ExemplarDataDilateX2},
    {k_Dilate, k_XYDir, 1, k_ExemplarFeatureIdsDilateXY1, k_ExemplarDataDilateXY1},
    {k_Dilate, k_XYDir, 2, k_ExemplarFeatureIdsDilateXY2, k_ExemplarDataDilateXY2},
    {k_Dilate, k_XYZDir, 1, k_ExemplarFeatureIdsDilateXYZ1, k_ExemplarDataDilateXYZ1},
    {k_Dilate, k_XYZDir, 2, k_ExemplarFeatureIdsDilateXYZ2, k_ExemplarDataDilateXYZ2},
    {k_Dilate, k_XZDir, 1, k_ExemplarFeatureIdsDilateXZ1, k_ExemplarDataDilateXZ1},
    {k_Dilate, k_XZDir, 2, k_ExemplarFeatureIdsDilateXZ2, k_ExemplarDataDilateXZ2},
    {k_Dilate, k_YDir, 1, k_ExemplarFeatureIdsDilateY1, k_ExemplarDataDilateY1},
    {k_Dilate, k_YDir, 2, k_ExemplarFeatureIdsDilateY2, k_ExemplarDataDilateY2},
    {k_Dilate, k_YZDir, 1, k_ExemplarFeatureIdsDilateYZ1, k_ExemplarDataDilateYZ1},
    {k_Dilate, k_YZDir, 2, k_ExemplarFeatureIdsDilateYZ2, k_ExemplarDataDilateYZ2},
    {k_Dilate, k_ZDir, 1, k_ExemplarFeatureIdsDilateZ1, k_ExemplarDataDilateZ1},
    {k_Dilate, k_ZDir, 2, k_ExemplarFeatureIdsDilateZ2, k_ExemplarDataDilateZ2},
    {k_Erode, k_XDir, 1, k_ExemplarFeatureIdsErodeX1, k_ExemplarDataErodeX1},
    {k_Erode, k_XDir, 2, k_ExemplarFeatureIdsErodeX2, k_ExemplarDataErodeX2},
    {k_Erode, k_XYDir, 1, k_ExemplarFeatureIdsErodeXY1, k_ExemplarDataErodeXY1},
    {k_Erode, k_XYDir, 2, k_ExemplarFeatureIdsErodeXY2, k_ExemplarDataErodeXY2},
    {k_Erode, k_XYZDir, 1, k_ExemplarFeatureIdsErodeXYZ1, k_ExemplarDataErodeXYZ1},
    {k_Erode, k_XYZDir, 2, k_ExemplarFeatureIdsErodeXYZ2, k_ExemplarDataErodeXYZ2},
    {k_Erode, k_XZDir, 1, k_ExemplarFeatureIdsErodeXZ1, k_ExemplarDataErodeXZ1},
    {k_Erode, k_XZDir, 2, k_ExemplarFeatureIdsErodeXZ2, k_ExemplarDataErodeXZ2},
    {k_Erode, k_YDir, 1, k_ExemplarFeatureIdsErodeY1, k_ExemplarDataErodeY1},
    {k_Erode, k_YDir, 2, k_ExemplarFeatureIdsErodeY2, k_ExemplarDataErodeY2},
    {k_Erode, k_YZDir, 1, k_ExemplarFeatureIdsErodeYZ1, k_ExemplarDataErodeYZ1},
    {k_Erode, k_YZDir, 2, k_ExemplarFeatureIdsErodeYZ2, k_ExemplarDataErodeYZ2},
    {k_Erode, k_ZDir, 1, k_ExemplarFeatureIdsErodeZ1, k_ExemplarDataErodeZ1},
    {k_Erode, k_ZDir, 2, k_ExemplarFeatureIdsErodeZ2, k_ExemplarDataErodeZ2},
}};
// clang-format on

DataStructure CreateTestData()
{
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, ::k_ImageGeometry);
  geom->setDimensions(k_GeometryDimensions);

  auto* cellData = AttributeMatrix::Create(dataStructure, ::k_CellData, k_TupleShape, geom->getId());

  // Feature IDs
  auto featureIdsPtr = std::make_shared<Int32DataStore>(k_TupleShape, ShapeType{1}, 0);
  auto* featureIdsArray = Int32Array::Create(dataStructure, ::k_FeatureIds, featureIdsPtr, cellData->getId());

  // Index 0, 14, 31
  auto& featureIds = featureIdsArray->getDataStoreRef();
  featureIds[0] = 0;
  featureIds[1] = 1;
  featureIds[2] = 1;
  featureIds[3] = 2;

  featureIds[4] = 2;
  featureIds[5] = 1;
  featureIds[6] = 2;
  featureIds[7] = 2;

  featureIds[8] = 1;
  featureIds[9] = 1;
  featureIds[10] = 0;
  featureIds[11] = 2;

  featureIds[12] = 2;
  featureIds[13] = 0;
  featureIds[14] = 0;
  featureIds[15] = 3;
  // Z
  featureIds[16] = 4;
  featureIds[17] = 4;
  featureIds[18] = 4;
  featureIds[19] = 4;

  featureIds[20] = 3;
  featureIds[21] = 3;
  featureIds[22] = 3;
  featureIds[23] = 3;

  featureIds[24] = 5;
  featureIds[25] = 5;
  featureIds[26] = 5;
  featureIds[27] = 5;

  featureIds[28] = 5;
  featureIds[29] = 6;
  featureIds[30] = 6;
  featureIds[31] = 0;

  // Misc DataArray
  auto dataStorePtr = std::make_shared<Int32DataStore>(k_TupleShape, ShapeType{1}, 0);
  auto* miscArray = Int32Array::Create(dataStructure, k_MiscData, dataStorePtr, cellData->getId());

  auto& dataStore = miscArray->getDataStoreRef();
  for(usize i = 0; i < dataStore.size(); i++)
  {
    dataStore[i] = i;
  }

  return dataStructure;
}

/**
 * @brief Verifies that an array listed in IgnoredDataArrayPaths still holds its original values.
 */
void CheckPathIgnored(const DataStructure& dataStructure)
{
  const DataStructure exemplarStructure = CreateTestData();

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_DataPath));
  const auto& dataStore = dataStructure.getDataRefAs<Int32Array>(k_DataPath).getDataStoreRef();
  REQUIRE_NOTHROW(exemplarStructure.getDataRefAs<Int32Array>(k_DataPath));
  const auto& exemplarStore = exemplarStructure.getDataRefAs<Int32Array>(k_DataPath).getDataStoreRef();

  REQUIRE(dataStore.size() == exemplarStore.size());

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarStore[i]);
  }
}

/**
 * @brief Verifies that the filter actually modified FeatureIds. Without this, a "the ignored array was left
 * alone" assertion would also pass for a filter that never ran at all.
 */
void CheckFeatureIdsModified(const DataStructure& dataStructure)
{
  const DataStructure exemplarStructure = CreateTestData();

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath));
  const auto& dataStore = dataStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath).getDataStoreRef();
  REQUIRE_NOTHROW(exemplarStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath));
  const auto& exemplarStore = exemplarStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath).getDataStoreRef();

  REQUIRE(dataStore.size() == exemplarStore.size());

  bool anyValueChanged = false;
  for(usize i = 0; i < dataStore.size() && !anyValueChanged; i++)
  {
    anyValueChanged = dataStore[i] != exemplarStore[i];
  }
  REQUIRE(anyValueChanged);
}

/**
 * @brief Compares the generated FeatureIds and Misc arrays against the expected values for this combination
 * of operation, enabled directions, and iteration count.
 */
void CheckOutput(const DataStructure& dataStructure, ChoicesParameter::ValueType operation, const DirectionType& directions, int32 iterations)
{
  const auto exemplarIter = std::find_if(k_Exemplars.cbegin(), k_Exemplars.cend(),
                                         [&](const ExemplarRecord& record) { return record.operation == operation && record.directions == directions && record.iterations == iterations; });
  if(exemplarIter == k_Exemplars.cend())
  {
    FAIL(fmt::format("No expected output is tabulated for operation {} with directions X={} Y={} Z={} and {} iteration(s)", operation, directions[0], directions[1], directions[2], iterations));
    return;
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath));
  const Int32AbstractDataStore& featureIds = dataStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_DataPath));
  const Int32AbstractDataStore& dataStore = dataStructure.getDataRefAs<Int32Array>(k_DataPath).getDataStoreRef();

  REQUIRE(featureIds.size() == exemplarIter->expectedFeatureIds.size());
  REQUIRE(dataStore.size() == exemplarIter->expectedData.size());

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(featureIds[i] == exemplarIter->expectedFeatureIds[i]);
    REQUIRE(dataStore[i] == exemplarIter->expectedData[i]);
  }
}

void RunFilter(DataStructure& dataStructure, ChoicesParameter::ValueType operation, int32 numIterations, const DirectionType& directions, const DataPath& geometryPath, const DataPath& featureIdsPath)
{
  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

// =====================================================================================================
// Multi-type transfer fixture -- V&V coverage Paths 12, 13 and 14.
//
// The retired archive-pinned exemplar test was the only test that ran the transfer stage over a cell
// AttributeMatrix holding arrays of more than one element type and more than one component. Retiring it
// forfeited three code paths; this fixture restores them at toy scale (production scale stays retired).
//
//   Path 12 -- mixed-type / multi-component `copyTuple` dispatch. `ErodeDilateBadDataTransferDataImpl`
//              calls `m_DataArrayPtr->copyTuple(neighbor, i)` through the `IDataArray` virtual
//              interface, so each element type and component count reaches a different template
//              instantiation. This fixture supplies int32x1, float32x3, uint8x1, float64x2 and int64x1.
//   Path 13 -- more than one concurrent `ErodeDilateBadDataTransferDataImpl`. The algorithm enqueues one
//              task per non-FeatureIds array on a `ParallelTaskAlgorithm` with parallelization ENABLED
//              (`Algorithms/ErodeDilateBadData.cpp`), so five non-FeatureIds arrays means five tasks
//              sharing the same `featureIds` store and `neighbors` vector.
//   Path 14 -- see `k_FeatureAvgPath` below for the honest scope of what the feature-level assertion
//              does and does not prove.
//
// FeatureIds-last ordering. Every expectation below is a function of the PRE-update FeatureIds: the
// transfer test in `ErodeDilateBadDataTransferDataImpl::operator()` reads `m_FeatureIds[i]` and
// `m_FeatureIds[neighbor]` to decide whether to copy. The algorithm therefore holds the FeatureIds
// update back until every other array has been transferred. With five arrays whose expectations all
// depend on that ordering, this test fails loudly if the FeatureIds update is ever hoisted ahead of the
// parallel batch -- a stronger pin than the single-Misc-array Expanded tests provide.
// =====================================================================================================

const StringLiteral k_Float32x3Data = "Float32x3";
const StringLiteral k_UInt8x1Data = "UInt8x1";
const StringLiteral k_Float64x2Data = "Float64x2";
const StringLiteral k_Int64x1Data = "Int64x1";
const StringLiteral k_FeatureAvgData = "FeatureAvg";

const DataPath k_Float32x3Path({::k_ImageGeometry, ::k_CellData, k_Float32x3Data});
const DataPath k_UInt8x1Path({::k_ImageGeometry, ::k_CellData, k_UInt8x1Data});
const DataPath k_Float64x2Path({::k_ImageGeometry, ::k_CellData, k_Float64x2Data});
const DataPath k_Int64x1Path({::k_ImageGeometry, ::k_CellData, k_Int64x1Data});

// Path 14 -- heterogeneous AttributeMatrix. `GenerateDataArrayList` (src/simplnx/Utilities/
// DataGroupUtilities.cpp:129-174) does NOT filter candidates by tuple count. It takes the parent group
// of the FeatureIds path and calls `parent.findAllChildrenOfType<IDataArray>()` with the DEFAULT
// `recursive == false` argument (BaseGroup.hpp:219), then removes only the paths named in
// IgnoredDataArrayPaths. Selection is therefore purely structural: direct children of the FeatureIds'
// own AttributeMatrix, nothing else.
//
// VERDICT: a feature-level AttributeMatrix that is a SIBLING of the cell AttributeMatrix under the
// geometry is excluded structurally -- it is never enumerated, so no exclusion branch is taken and no
// tuple-count comparison happens. The assertion below is therefore a SANITY / REGRESSION assert, not an
// exercise of an existing exclusion branch. What it genuinely pins is that the selection rule stays
// non-recursive and stays scoped to the FeatureIds' parent: if someone flips `findAllChildrenOfType` to
// `recursive == true`, or re-roots the search at the geometry, this array would be swept into the
// transfer batch and `copyTuple(neighbor, i)` would be called with cell indices up to 31 on a 7-tuple
// array. That is a real out-of-range hazard, and this assertion is what would catch it.
const DataPath k_FeatureAvgPath({::k_ImageGeometry, ::k_CellFeatureData, k_FeatureAvgData});

constexpr usize k_NumFeatureTuples = 7; // FeatureIds 0..6 -- deliberately != k_NumTuples (32).
constexpr usize k_Float32NumComps = 3;
constexpr usize k_Float64NumComps = 2;

// Index-encoded generators. Every element carries its own tuple index, so after the run the SOURCE
// tuple of each copied element is observable per element rather than merely "some value changed".
// The per-component offsets differ within a tuple, so a `copyTuple` that transposed components inside a
// tuple would also be caught.
constexpr float32 Float32x3Value(usize tupleIndex, usize componentIndex)
{
  return static_cast<float32>(tupleIndex) + 0.25f * static_cast<float32>(componentIndex + 1);
}

constexpr uint8 UInt8x1Value(usize tupleIndex)
{
  return static_cast<uint8>(tupleIndex % 251);
}

constexpr float64 Float64x2Value(usize tupleIndex, usize componentIndex)
{
  return static_cast<float64>(tupleIndex) + 0.125 * static_cast<float64>(componentIndex + 1);
}

constexpr int64 Int64x1Value(usize tupleIndex)
{
  return static_cast<int64>(tupleIndex) * 1000 + 7;
}

constexpr float32 FeatureAvgValue(usize tupleIndex)
{
  return static_cast<float32>(tupleIndex) * 100.0f;
}

// -----------------------------------------------------------------------------------------------------
// HAND-DERIVED ORACLE -- Erode, X+Y+Z all enabled, 1 iteration, over the CreateTestData() FeatureIds.
//
// Grid is 4 x 4 x 2 (X,Y,Z); index = 16*z + 4*y + x. Face-neighbor traversal order is
// [-Z, -Y, -X, +X, +Y, +Z] with offsets [-16, -4, -1, +1, +4, +16]
// (`initializeFaceNeighborInternalIdx` / `initializeFaceNeighborOffsets`, NeighborUtilities.hpp:201,244).
// Erode visits only voxels whose FeatureIds == 0; for each in-bounds face neighbor with feature > 0 it
// increments featureCount[feature] and keeps the neighbor when `current > most` -- a STRICT greater-than,
// so on a tie the FIRST neighbor in traversal order wins.
//
// Starting FeatureIds (CreateTestData): the bad voxels are exactly {0, 10, 13, 14, 31}.
//
//   i = 0   (x0,y0,z0): in-bounds neighbors +X=1(f1), +Y=4(f2), +Z=16(f4).
//                       +X -> count[1]=1 > most 0  -> most=1, neighbor=1
//                       +Y -> count[2]=1, not > 1  ; +Z -> count[4]=1, not > 1     => neighbor = 1
//   i = 10  (x2,y2,z0): neighbors -Y=6(f2), -X=9(f1), +X=11(f2), +Y=14(f0), +Z=26(f5).
//                       -Y -> count[2]=1 > 0 -> most=1, neighbor=6
//                       -X -> count[1]=1, not > 1
//                       +X -> count[2]=2 > 1 -> most=2, neighbor=11
//                       +Y skipped (feature 0); +Z -> count[5]=1, not > 2          => neighbor = 11
//   i = 13  (x1,y3,z0): neighbors -Y=9(f1), -X=12(f2), +X=14(f0), +Z=29(f6).
//                       -Y -> count[1]=1 > 0 -> most=1, neighbor=9
//                       -X -> count[2]=1, not > 1; +X skipped; +Z -> count[6]=1, not > 1
//                                                                                   => neighbor = 9
//   i = 14  (x2,y3,z0): neighbors -Y=10(f0), -X=13(f0), +X=15(f3), +Z=30(f6).
//                       -Y and -X skipped (feature 0)
//                       +X -> count[3]=1 > 0 -> most=1, neighbor=15
//                       +Z -> count[6]=1, not > 1                                   => neighbor = 15
//   i = 31  (x3,y3,z1): neighbors -Z=15(f3), -Y=27(f5), -X=30(f6).
//                       -Z -> count[3]=1 > 0 -> most=1, neighbor=15
//                       -Y -> count[5]=1, not > 1; -X -> count[6]=1, not > 1        => neighbor = 15
//
// NEIGHBOR MAP: 0 <- 1, 10 <- 11, 13 <- 9, 14 <- 15, 31 <- 15. All other tuples untouched.
// This map is independently corroborated by the existing legacy-captured constants: subtracting the
// identity Misc fill from `k_ExemplarDataErodeXYZ1` yields exactly these five substitutions, and
// `k_ExemplarFeatureIdsErodeXYZ1` at {0,10,13,14,31} is {1,2,1,3,3} = FeatureIds at {1,11,9,15,15}.
//
// Because each cell array's transfer is `array[i] = array[neighbor]` with the SAME map, the expected
// post-run value of tuple i, component c is generator(k_ErodeXYZ1SourceTuple[i], c). Spelled out for
// every changed element of every array:
//
//   Misc (int32 x1, fill = i):        [0]=1        [10]=11        [13]=9        [14]=15        [31]=15
//   Float32x3 (fill = i + .25(c+1)):
//     [0]  = {1.25, 1.50, 1.75}    [10] = {11.25, 11.50, 11.75}   [13] = {9.25, 9.50, 9.75}
//     [14] = {15.25, 15.50, 15.75} [31] = {15.25, 15.50, 15.75}
//   UInt8x1 (fill = i % 251):         [0]=1        [10]=11        [13]=9        [14]=15        [31]=15
//   Float64x2 (fill = i + .125(c+1)):
//     [0]  = {1.125, 1.25}         [10] = {11.125, 11.25}         [13] = {9.125, 9.25}
//     [14] = {15.125, 15.25}       [31] = {15.125, 15.25}
//   Int64x1 (fill = 1000i + 7):       [0]=1007     [10]=11007     [13]=9007     [14]=15007     [31]=15007
//   FeatureAvg (feature AM):          untouched -- see k_FeatureAvgPath.
//
// Exact floating-point equality is the correct comparison here: 0.25/0.5/0.75 and 0.125/0.25 are exactly
// representable, the filter performs a bitwise tuple copy rather than arithmetic, and the expectation is
// produced by the same generator expression, so the two are bit-identical.
// -----------------------------------------------------------------------------------------------------
constexpr std::array<usize, k_NumTuples> k_ErodeXYZ1SourceTuple{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 11, 12, 9, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};

/**
 * @brief Builds the CreateTestData() fixture and adds four more cell arrays of distinct element types and
 * component counts, plus a feature-level AttributeMatrix whose tuple count differs from the cell count.
 */
DataStructure CreateMultiTypeTestData()
{
  DataStructure dataStructure = CreateTestData();

  const auto& cellDataAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({::k_ImageGeometry, ::k_CellData}));
  const DataObject::IdType cellDataId = cellDataAM.getId();

  auto float32StorePtr = std::make_shared<Float32DataStore>(k_TupleShape, ShapeType{k_Float32NumComps}, 0.0f);
  auto* float32Array = Float32Array::Create(dataStructure, k_Float32x3Data, float32StorePtr, cellDataId);
  auto& float32Ref = float32Array->getDataStoreRef();
  for(usize i = 0; i < k_NumTuples; i++)
  {
    for(usize c = 0; c < k_Float32NumComps; c++)
    {
      float32Ref[i * k_Float32NumComps + c] = Float32x3Value(i, c);
    }
  }

  auto uint8StorePtr = std::make_shared<UInt8DataStore>(k_TupleShape, ShapeType{1}, 0);
  auto* uint8Array = UInt8Array::Create(dataStructure, k_UInt8x1Data, uint8StorePtr, cellDataId);
  auto& uint8Ref = uint8Array->getDataStoreRef();
  for(usize i = 0; i < k_NumTuples; i++)
  {
    uint8Ref[i] = UInt8x1Value(i);
  }

  auto float64StorePtr = std::make_shared<Float64DataStore>(k_TupleShape, ShapeType{k_Float64NumComps}, 0.0);
  auto* float64Array = Float64Array::Create(dataStructure, k_Float64x2Data, float64StorePtr, cellDataId);
  auto& float64Ref = float64Array->getDataStoreRef();
  for(usize i = 0; i < k_NumTuples; i++)
  {
    for(usize c = 0; c < k_Float64NumComps; c++)
    {
      float64Ref[i * k_Float64NumComps + c] = Float64x2Value(i, c);
    }
  }

  auto int64StorePtr = std::make_shared<Int64DataStore>(k_TupleShape, ShapeType{1}, 0);
  auto* int64Array = Int64Array::Create(dataStructure, k_Int64x1Data, int64StorePtr, cellDataId);
  auto& int64Ref = int64Array->getDataStoreRef();
  for(usize i = 0; i < k_NumTuples; i++)
  {
    int64Ref[i] = Int64x1Value(i);
  }

  // Feature-level AttributeMatrix: 7 tuples, sibling of the cell AttributeMatrix under the geometry.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({::k_ImageGeometry}));
  auto* featureDataAM = AttributeMatrix::Create(dataStructure, ::k_CellFeatureData, ShapeType{k_NumFeatureTuples}, imageGeom.getId());

  auto featureAvgStorePtr = std::make_shared<Float32DataStore>(ShapeType{k_NumFeatureTuples}, ShapeType{1}, 0.0f);
  auto* featureAvgArray = Float32Array::Create(dataStructure, k_FeatureAvgData, featureAvgStorePtr, featureDataAM->getId());
  auto& featureAvgRef = featureAvgArray->getDataStoreRef();
  for(usize i = 0; i < k_NumFeatureTuples; i++)
  {
    featureAvgRef[i] = FeatureAvgValue(i);
  }

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  bool dirX = GENERATE(true, false);
  bool dirY = GENERATE(true, false);
  bool dirZ = GENERATE(true, false);
  int32 numIterations = GENERATE(1, 2);

  const DirectionType directions = {dirX, dirY, dirZ};
  const ChoicesParameter::ValueType operation = nx::core::detail::k_ErodeIndex;

  // At least one direction is required; preflight rejects the all-off combination. See the No Direction test.
  if(!dirX && !dirY && !dirZ)
  {
    SUCCEED("at least one direction is required");
    return;
  }

  DataStructure dataStructure = CreateTestData();

  RunFilter(dataStructure, operation, numIterations, directions, DataPath({k_ImageGeometry}), k_ImageFeatureIdsPath);
  CheckOutput(dataStructure, operation, directions, numIterations);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Dilate) Expanded", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  bool dirX = GENERATE(true, false);
  bool dirY = GENERATE(true, false);
  bool dirZ = GENERATE(true, false);
  int32 numIterations = GENERATE(1, 2);

  const DirectionType directions = {dirX, dirY, dirZ};
  const ChoicesParameter::ValueType operation = nx::core::detail::k_DilateIndex;

  // At least one direction is required; preflight rejects the all-off combination. See the No Direction test.
  if(!dirX && !dirY && !dirZ)
  {
    SUCCEED("at least one direction is required");
    return;
  }

  DataStructure dataStructure = CreateTestData();

  RunFilter(dataStructure, operation, numIterations, directions, DataPath({k_ImageGeometry}), k_ImageFeatureIdsPath);
  CheckOutput(dataStructure, operation, directions, numIterations);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter Ignored Path", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestData();
  const DirectionType directions = {true, true, true};
  const ChoicesParameter::ValueType operation = GENERATE(k_Dilate, k_Erode);
  const int32 numIterations = 1;

  const DataPath ignoredPath = k_DataPath;

  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ImageFeatureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{ignoredPath}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The ignored array must come through untouched, and the filter must have actually done work -- without
  // the second check the first one would also pass for a filter that never ran.
  CheckPathIgnored(dataStructure);
  CheckFeatureIdsModified(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter No Direction", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestData();
  const DirectionType directions = {false, false, false};
  const ChoicesParameter::ValueType operation = GENERATE(k_Dilate, k_Erode);
  const int32 numIterations = GENERATE(1, 2);

  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ImageFeatureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  REQUIRE(preflightResult.outputActions.errors()[0].code == -14601);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter No Dimensions", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestData();
  const DirectionType directions = {true, true, true};
  const ChoicesParameter::ValueType operation = k_Dilate;
  const int32 numIterations = 1;
  const DataPath geomPath({k_ImageGeometry});

  auto* imageGeom = dataStructure.getDataAs<ImageGeom>(geomPath);
  imageGeom->setDimensions(SizeVec3());

  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ImageFeatureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  REQUIRE(preflightResult.outputActions.errors()[0].code == -14602);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter Invalid Number of Iterations", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  // DREAM3D 6.5.171 rejected a non-positive iteration count at preflight
  // (`ErodeDilateBadData.cpp:141-146`, error -5555 "The number of iterations (%1) must be positive"). The
  // SIMPLNX port dropped that guard and silently no-opped instead: the iteration loop `for(i = 0; i <
  // NumIterations; i++)` simply never runs, so the filter reported success while doing nothing. Restored
  // 2026-08-19 as a uniform guard across the erode/dilate family.
  DataStructure dataStructure = CreateTestData();
  const DirectionType directions = k_XYZDir;
  const ChoicesParameter::ValueType operation = GENERATE(k_Dilate, k_Erode);
  const DataPath geomPath({k_ImageGeometry});

  const ErodeDilateBadDataFilter filter;
  Arguments args;

  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ImageFeatureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));

  SECTION("Non-positive iteration counts are rejected")
  {
    const int32 numIterations = GENERATE(0, -3);
    args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    REQUIRE(preflightResult.outputActions.errors()[0].code == -14603);
    // The message must name the offending value so the user can find it without re-reading the pipeline.
    REQUIRE(preflightResult.outputActions.errors()[0].message.find(std::to_string(numIterations)) != std::string::npos);
  }

  SECTION("Positive control: the smallest legal iteration count preflights cleanly")
  {
    args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(1));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Multi-Type Transfer)", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateMultiTypeTestData();

  // Erode, all three directions, one iteration -- the combination whose neighbor map is hand-derived in
  // the comment above k_ErodeXYZ1SourceTuple and cross-checked against the file's legacy-captured
  // k_ExemplarFeatureIdsErodeXYZ1 / k_ExemplarDataErodeXYZ1 constants.
  RunFilter(dataStructure, k_Erode, 1, k_XYZDir, DataPath({k_ImageGeometry}), k_ImageFeatureIdsPath);

  // FeatureIds and Misc against the existing tabulated oracle. Misc is one of the five parallel transfer
  // tasks here rather than the only one, so this also confirms the extra arrays did not perturb it.
  CheckOutput(dataStructure, k_Erode, k_XYZDir, 1);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_Float32x3Path));
  const auto& float32Ref = dataStructure.getDataRefAs<Float32Array>(k_Float32x3Path).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_UInt8x1Path));
  const auto& uint8Ref = dataStructure.getDataRefAs<UInt8Array>(k_UInt8x1Path).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(k_Float64x2Path));
  const auto& float64Ref = dataStructure.getDataRefAs<Float64Array>(k_Float64x2Path).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_Int64x1Path));
  const auto& int64Ref = dataStructure.getDataRefAs<Int64Array>(k_Int64x1Path).getDataStoreRef();

  REQUIRE(float32Ref.getNumberOfTuples() == k_NumTuples);
  REQUIRE(uint8Ref.getNumberOfTuples() == k_NumTuples);
  REQUIRE(float64Ref.getNumberOfTuples() == k_NumTuples);
  REQUIRE(int64Ref.getNumberOfTuples() == k_NumTuples);

  for(usize i = 0; i < k_NumTuples; i++)
  {
    CAPTURE(i);
    const usize sourceTuple = k_ErodeXYZ1SourceTuple[i];

    for(usize c = 0; c < k_Float32NumComps; c++)
    {
      CAPTURE(c);
      REQUIRE(float32Ref[i * k_Float32NumComps + c] == Float32x3Value(sourceTuple, c));
    }

    REQUIRE(uint8Ref[i] == UInt8x1Value(sourceTuple));

    for(usize c = 0; c < k_Float64NumComps; c++)
    {
      CAPTURE(c);
      REQUIRE(float64Ref[i * k_Float64NumComps + c] == Float64x2Value(sourceTuple, c));
    }

    REQUIRE(int64Ref[i] == Int64x1Value(sourceTuple));
  }

  // Path 14 -- the feature-level array must be untouched. See the k_FeatureAvgPath comment for what this
  // does and does not prove.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureAvgPath));
  const auto& featureAvgRef = dataStructure.getDataRefAs<Float32Array>(k_FeatureAvgPath).getDataStoreRef();
  REQUIRE(featureAvgRef.getNumberOfTuples() == k_NumFeatureTuples);
  for(usize i = 0; i < k_NumFeatureTuples; i++)
  {
    CAPTURE(i);
    REQUIRE(featureAvgRef[i] == FeatureAvgValue(i));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ErodeDilateBadDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ErodeDilateBadDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ErodeDilateBadDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ErodeDilateBadDataFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ErodeDilateBadDataFilter::k_Operation_Key) == k_Dilate);
      CHECK(args.value<int32>(ErodeDilateBadDataFilter::k_NumIterations_Key) == 5);
      CHECK(args.value<bool>(ErodeDilateBadDataFilter::k_XDirOn_Key) == true);
      CHECK(args.value<bool>(ErodeDilateBadDataFilter::k_YDirOn_Key) == true);
      CHECK(args.value<bool>(ErodeDilateBadDataFilter::k_ZDirOn_Key) == true);
      CHECK(args.value<DataPath>(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
