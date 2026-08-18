#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ErodeDilateBadData.hpp"
#include "SimplnxCore/Filters/ErodeDilateBadDataFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_EbsdScanDataName("EBSD Scan Data");

const DataPath k_InputData({"Input Data"});
const DataPath k_EbsdScanDataDataPath = k_InputData.createChildPath(k_EbsdScanDataName);
const DataPath k_FeatureIdsDataPath = k_EbsdScanDataDataPath.createChildPath("FeatureIds");
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
// Exemplar Dilate data for A/B testing
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

// Exemplar Erode data for A/B testing
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
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Erode)", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_bad_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    const ErodeDilateBadDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Erode));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsDataPath));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputData));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_erode_dilate_bad_data.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  const std::string k_ExemplarDataContainerName("Exemplar Bad Data Erode");
  const DataPath k_ErodeCellAttributeMatrixDataPath = DataPath({k_ExemplarDataContainerName, "EBSD Scan Data"});

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataDataPath, k_ExemplarDataContainerName);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

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
