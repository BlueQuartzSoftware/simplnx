#include "SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.hpp"
#include "SimplnxCore/Filters/RotateSampleRefFrameFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <Eigen/Dense>
#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

// =============================================================================
// V&V test suite for RotateSampleRefFrame.
//
// Oracle: Class 1 (Analytical) primary + Class 4 (Invariant) companion.
//
// RotateSampleRefFrame performs a reference-frame rotation of an ImageGeom by
// nearest-neighbor resampling. As shown in the V&V report, this is a *lossless
// voxel permutation* if and only if the rotation is a multiple of 90 degrees
// about a principal (X/Y/Z) axis. A preflight guard now rejects every other
// rotation, so the entire supported domain is analytically provable:
//
//   * Class 1: the output is an exact, hand-derivable permutation of the input
//     (e.g. a 180-degree rotation about Z of a single slice is a full reversal),
//     and the output dimensions are a deterministic permutation of the input
//     dimensions.
//   * Class 4: a principal-90 rotation is a bijection on the voxel set, so the
//     multiset of output values equals the multiset of input values, no
//     background (0) is introduced, and composing the rotation back to 360
//     degrees returns the original array exactly.
//
// Input cell values are 1..N (never 0) so a 0 anywhere in the output
// unambiguously signals background fill introduced by a (disallowed) lossy
// resample. No external exemplar file is used.
// =============================================================================

namespace
{
const std::string k_CellDataName = "CellData";
const std::string k_ValuesName = "Values";
const DataPath k_InputPath({"Input"});
const DataPath k_OutputPath({"Output"});

// Build an ImageGeom whose single Int32 cell array is filled 1..N in ZYX order.
ImageGeom* CreateSequentialImageGeom(DataStructure& dataStructure, const std::string& name, const SizeVec3& dims, const FloatVec3& spacing = FloatVec3{1.0f, 1.0f, 1.0f},
                                     const FloatVec3& origin = FloatVec3{0.0f, 0.0f, 0.0f})
{
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, name);
  imageGeom->setDimensions(dims);
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);

  AttributeMatrix* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, {dims[2], dims[1], dims[0]}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  Int32Array* values = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_ValuesName, {dims[2], dims[1], dims[0]}, {1}, cellAM->getId());
  auto& valuesStore = values->getDataStoreRef();
  for(usize i = 0; i < valuesStore.getNumberOfTuples(); i++)
  {
    valuesStore[i] = static_cast<int32>(i + 1); // 1..N, all distinct, none == 0
  }
  return imageGeom;
}

// Convert a 3x3 rotation matrix into the 4x4 DynamicTable expected by the Rotation Matrix representation.
std::vector<std::vector<float64>> ConvertMatrixToTable(const Eigen::Matrix3f& matrix)
{
  std::vector<std::vector<float64>> data;
  for(Eigen::Index i = 0; i < matrix.rows(); i++)
  {
    std::vector<float64> row;
    for(Eigen::Index j = 0; j < matrix.cols(); j++)
    {
      row.push_back(matrix(i, j));
    }
    row.push_back(0.0);
    data.push_back(row);
  }
  data.push_back({0.0, 0.0, 0.0, 1.0});
  return data;
}

// Base arguments common to every run (axis-angle representation).
Arguments MakeAxisAngleArgs(const DataPath& inputPath, const DataPath& outputPath, const VectorFloat32Parameter::ValueType& axisAngle, bool sliceBySlice = false, bool keepOrigin = false)
{
  Arguments args;
  args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::AxisAngle)));
  args.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(outputPath));
  args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(axisAngle));
  args.insertOrAssign(RotateSampleRefFrameFilter::k_RotateSliceBySlice_Key, std::make_any<bool>(sliceBySlice));
  args.insertOrAssign(RotateSampleRefFrameFilter::k_KeepInputGeometryOrigin_Key, std::make_any<bool>(keepOrigin));
  return args;
}

// Read the output Int32 "Values" array (ZYX order) into a vector.
std::vector<int32> ReadOutputValues(const DataStructure& dataStructure, const DataPath& geomPath)
{
  const auto* values = dataStructure.getDataAs<Int32Array>(geomPath.createChildPath(k_CellDataName).createChildPath(k_ValuesName));
  REQUIRE(values != nullptr);
  const auto& store = values->getDataStoreRef();
  std::vector<int32> out(store.getNumberOfTuples());
  for(usize i = 0; i < out.size(); i++)
  {
    out[i] = store[i];
  }
  return out;
}

// A principal-90 rotation is a bijection on the voxel set: the output values are exactly the input
// values as a multiset, and no background (0) was introduced.
void RequireValueBijection(const std::vector<int32>& output, usize inputCount)
{
  REQUIRE(output.size() == inputCount);
  std::vector<int32> sorted = output;
  std::sort(sorted.begin(), sorted.end());
  std::vector<int32> expected(inputCount);
  for(usize i = 0; i < inputCount; i++)
  {
    expected[i] = static_cast<int32>(i + 1);
  }
  REQUIRE(std::count(output.begin(), output.end(), 0) == 0); // no background introduced
  REQUIRE(sorted == expected);                               // multiset conserved
}
} // namespace

// -----------------------------------------------------------------------------
// Class 1: an explicit, hand-derived permutation.
// A 180-degree rotation about Z maps in-plane (x, y) -> (-x, -y). On a single
// Z-slice this reverses the row-major order of the plane, so a slice filled
// 1..6 becomes 6..1. Verified for both the Axis-Angle and Rotation-Matrix
// representations (the latter also cross-checks GenerateRotationTransformationMatrix
// against an Eigen-built matrix, an incidental Class 2 check).
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: Class 1 - 180 about Z reverses a slice", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 dims = {3, 2, 1}; // 6 cells: [1 2 3 / 4 5 6]
  const std::vector<int32> expected = {6, 5, 4, 3, 2, 1};

  RotateSampleRefFrameFilter filter;

  SECTION("Axis-Angle representation")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", dims);
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 180.0f});

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(ReadOutputValues(dataStructure, k_OutputPath) == expected);
  }

  SECTION("Rotation-Matrix representation")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", dims);

    Eigen::Matrix3f rotationMatrix = Eigen::AngleAxisf(180.0f * (numbers::pi_v<float> / 180.0f), Eigen::Vector3f(0.0f, 0.0f, 1.0f)).toRotationMatrix();
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 180.0f});
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(ConvertMatrixToTable(rotationMatrix)));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(ReadOutputValues(dataStructure, k_OutputPath) == expected);
  }
}

// -----------------------------------------------------------------------------
// Class 1 (dimensions) + Class 4 (value bijection): sweep every principal-90
// rotation. Output dimensions are a deterministic axis permutation, and the
// output is a lossless permutation of the input (multiset conserved, no
// background). Covered for both representations.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: Class 1/4 - principal-90 rotations are lossless permutations", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 inDims = {4, 3, 2};
  const usize inCount = 4 * 3 * 2;

  // {label, axis-angle, expected output dims}
  auto [label, axisAngle, expectedDims] = GENERATE(
      std::make_tuple("90 about Z", VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 90.0f}, SizeVec3{3, 4, 2}),
      std::make_tuple("180 about Z", VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 180.0f}, SizeVec3{4, 3, 2}),
      std::make_tuple("270 about Z", VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 270.0f}, SizeVec3{3, 4, 2}),
      std::make_tuple("90 about X", VectorFloat32Parameter::ValueType{1.0f, 0.0f, 0.0f, 90.0f}, SizeVec3{4, 2, 3}),
      std::make_tuple("180 about X", VectorFloat32Parameter::ValueType{1.0f, 0.0f, 0.0f, 180.0f}, SizeVec3{4, 3, 2}),
      std::make_tuple("270 about X", VectorFloat32Parameter::ValueType{1.0f, 0.0f, 0.0f, 270.0f}, SizeVec3{4, 2, 3}),
      std::make_tuple("90 about Y", VectorFloat32Parameter::ValueType{0.0f, 1.0f, 0.0f, 90.0f}, SizeVec3{2, 3, 4}),
      std::make_tuple("180 about Y", VectorFloat32Parameter::ValueType{0.0f, 1.0f, 0.0f, 180.0f}, SizeVec3{4, 3, 2}),
      std::make_tuple("270 about Y", VectorFloat32Parameter::ValueType{0.0f, 1.0f, 0.0f, 270.0f}, SizeVec3{2, 3, 4}));

  RotateSampleRefFrameFilter filter;

  DYNAMIC_SECTION(label << " (Axis-Angle)")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, axisAngle);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto* outputGeom = dataStructure.getDataAs<ImageGeom>(k_OutputPath);
    REQUIRE(outputGeom != nullptr);
    REQUIRE(outputGeom->getDimensions() == expectedDims);
    RequireValueBijection(ReadOutputValues(dataStructure, k_OutputPath), inCount);
  }

  DYNAMIC_SECTION(label << " (Rotation-Matrix)")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);

    Eigen::Vector3f axis(axisAngle[0], axisAngle[1], axisAngle[2]);
    Eigen::Matrix3f rotationMatrix = Eigen::AngleAxisf(axisAngle[3] * (numbers::pi_v<float> / 180.0f), axis).toRotationMatrix();
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, axisAngle);
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(ConvertMatrixToTable(rotationMatrix)));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto* outputGeom = dataStructure.getDataAs<ImageGeom>(k_OutputPath);
    REQUIRE(outputGeom != nullptr);
    REQUIRE(outputGeom->getDimensions() == expectedDims);
    RequireValueBijection(ReadOutputValues(dataStructure, k_OutputPath), inCount);
  }
}

// -----------------------------------------------------------------------------
// Class 4 (composition / group invariant): applying a rotation until it sums to
// 360 degrees must return the original array and dimensions exactly. This pins
// the exact permutation convention without hand-encoding it.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: Class 4 - full-circle composition is identity", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 inDims = {4, 3, 2};

  auto [label, axisAngle, repeats] = GENERATE(std::make_tuple("four 90 about Z", VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 90.0f}, 4),
                                              std::make_tuple("two 180 about Z", VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 180.0f}, 2),
                                              std::make_tuple("four 90 about X", VectorFloat32Parameter::ValueType{1.0f, 0.0f, 0.0f, 90.0f}, 4),
                                              std::make_tuple("four 90 about Y", VectorFloat32Parameter::ValueType{0.0f, 1.0f, 0.0f, 90.0f}, 4));

  DYNAMIC_SECTION(label)
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    const std::vector<int32> original = ReadOutputValues(dataStructure, k_InputPath);

    RotateSampleRefFrameFilter filter;
    DataPath currentInput = k_InputPath;
    for(int32 step = 0; step < repeats; step++)
    {
      DataPath stepOutput({fmt::format("Step_{}", step)});
      Arguments args = MakeAxisAngleArgs(currentInput, stepOutput, axisAngle);
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      currentInput = stepOutput;
    }

    const auto* finalGeom = dataStructure.getDataAs<ImageGeom>(currentInput);
    REQUIRE(finalGeom != nullptr);
    REQUIRE(finalGeom->getDimensions() == inDims);
    REQUIRE(ReadOutputValues(dataStructure, currentInput) == original);
  }
}

// -----------------------------------------------------------------------------
// Class 1 (origin): KeepInputGeometryOrigin controls the output origin.
// For a 90-degree rotation about Z, R maps (x,y)->(-y,x). A 4x3x2 volume at
// origin (0,0,0) has its rotated bounding box spanning x' in [-3,0] (from y in
// [0,3]) and y' in [0,4], so the transform-derived origin is (-3,0,0). With
// KeepInputGeometryOrigin the output keeps the input origin (0,0,0).
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: KeepInputGeometryOrigin controls output origin", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 inDims = {4, 3, 2};
  const VectorFloat32Parameter::ValueType axisAngle{0.0f, 0.0f, 1.0f, 90.0f};
  RotateSampleRefFrameFilter filter;

  SECTION("Keep input origin -> (0,0,0)")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, axisAngle, /*sliceBySlice=*/false, /*keepOrigin=*/true);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto origin = dataStructure.getDataAs<ImageGeom>(k_OutputPath)->getOrigin();
    REQUIRE(origin[0] == Approx(0.0f).margin(1e-4f));
    REQUIRE(origin[1] == Approx(0.0f).margin(1e-4f));
    REQUIRE(origin[2] == Approx(0.0f).margin(1e-4f));
  }

  SECTION("Transform-derived origin -> (-3,0,0)")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, axisAngle, /*sliceBySlice=*/false, /*keepOrigin=*/false);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto origin = dataStructure.getDataAs<ImageGeom>(k_OutputPath)->getOrigin();
    REQUIRE(origin[0] == Approx(-3.0f).margin(1e-4f));
    REQUIRE(origin[1] == Approx(0.0f).margin(1e-4f));
    REQUIRE(origin[2] == Approx(0.0f).margin(1e-4f));
  }
}

// -----------------------------------------------------------------------------
// Slice-by-slice: a 180-degree rotation about an in-plane axis (X/Y) preserves
// the Z (slice) axis, so slice-by-slice is a valid, lossless per-slice flip. It
// must still be a value bijection, and it must differ from the true 3D rotation
// (which reverses slice order).
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: slice-by-slice 180 about Y is a lossless per-slice flip", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 inDims = {4, 3, 2};
  const usize inCount = 4 * 3 * 2;
  const VectorFloat32Parameter::ValueType axisAngle{0.0f, 1.0f, 0.0f, 180.0f};

  RotateSampleRefFrameFilter filter;

  DataStructure dataStructure;
  CreateSequentialImageGeom(dataStructure, "Input", inDims);

  // Slice-by-slice per-slice flip
  DataPath slicePath({"SliceBySlice"});
  {
    Arguments args = MakeAxisAngleArgs(k_InputPath, slicePath, axisAngle, /*sliceBySlice=*/true);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    RequireValueBijection(ReadOutputValues(dataStructure, slicePath), inCount);
  }

  // True 3D rotation (reverses slice order)
  DataPath fullPath({"Full3D"});
  {
    Arguments args = MakeAxisAngleArgs(k_InputPath, fullPath, axisAngle, /*sliceBySlice=*/false);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    RequireValueBijection(ReadOutputValues(dataStructure, fullPath), inCount);
  }

  // The two modes produce genuinely different permutations (slice order preserved vs reversed).
  REQUIRE(ReadOutputValues(dataStructure, slicePath) != ReadOutputValues(dataStructure, fullPath));
}

// -----------------------------------------------------------------------------
// Guard (negative tests): arbitrary rotations are rejected in preflight because
// they would produce a lossy resampled result rather than a reference-frame
// rotation. Error code -6850.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: rejects non-principal-90 rotations", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 inDims = {4, 3, 2};
  RotateSampleRefFrameFilter filter;

  SECTION("45 degrees about Z (Axis-Angle)")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 45.0f});
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
    REQUIRE(preflightResult.outputActions.errors().at(0).code == -6850);
  }

  SECTION("90 degrees about a non-principal axis (Axis-Angle)")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, VectorFloat32Parameter::ValueType{1.0f, 1.0f, 1.0f, 90.0f});
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
    REQUIRE(preflightResult.outputActions.errors().at(0).code == -6850);
  }

  SECTION("Arbitrary (45 degree) Rotation Matrix")
  {
    DataStructure dataStructure;
    CreateSequentialImageGeom(dataStructure, "Input", inDims);
    Eigen::Matrix3f rotationMatrix = Eigen::AngleAxisf(45.0f * (numbers::pi_v<float> / 180.0f), Eigen::Vector3f(0.0f, 0.0f, 1.0f)).toRotationMatrix();
    Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 45.0f});
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(ConvertMatrixToTable(rotationMatrix)));
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
    REQUIRE(preflightResult.outputActions.errors().at(0).code == -6850);
  }
}

// -----------------------------------------------------------------------------
// Guard (negative test): slice-by-slice combined with a rotation that reorders
// slices (90 degrees about X) is rejected. Error code -6851.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::RotateSampleRefFrame: rejects slice-by-slice with a slice-reordering rotation", "[SimplnxCore][RotateSampleRefFrameFilter]")
{
  const SizeVec3 inDims = {4, 3, 2};
  RotateSampleRefFrameFilter filter;

  DataStructure dataStructure;
  CreateSequentialImageGeom(dataStructure, "Input", inDims);
  Arguments args = MakeAxisAngleArgs(k_InputPath, k_OutputPath, VectorFloat32Parameter::ValueType{1.0f, 0.0f, 0.0f, 90.0f}, /*sliceBySlice=*/true);
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().at(0).code == -6851);
}

TEST_CASE("SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RotateSampleRefFrameFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RotateSampleRefFrameFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RotateSampleRefFrameFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RotateSampleRefFrameFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<ChoicesParameter::ValueType>(RotateSampleRefFrameFilter::k_RotationRepresentation_Key) == 0);
        // Complex type (DynamicTableFilterParameterConverter) - verified by successful pipeline loading
        // Complex type (FloatVec3p1FilterParameterConverter) - verified by successful pipeline loading
      }
      CHECK(args.value<DataPath>(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
