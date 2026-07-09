#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/RegularizeZSpacingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_ImageGeometryName = "ImageGeometry";
const std::string k_CellDataName = "Cell Data";
const std::string k_DataArrayName = "Data";
const std::string k_LooseArrayName = "LooseData";
const std::string k_OutputGeometryName = "Regularized Geometry";

const DataPath k_InputGeometryPath({k_ImageGeometryName});
const DataPath k_OutputGeometryPath({k_OutputGeometryName});

// Builds a 2 x 1 x 4 (X,Y,Z) Image Geometry whose single-component int32 cell array
// is filled so that each tuple's value equals its tuple index. This makes the
// remapping trivial to verify: an output tuple copied from source tuple S has value S.
DataStructure createTestDataStructure()
{
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometryName);
  imageGeom->setDimensions(SizeVec3{2, 1, 4}); // X, Y, Z
  imageGeom->setSpacing(FloatVec3{0.5F, 0.75F, 1.0F});
  imageGeom->setOrigin(FloatVec3{1.0F, 2.0F, 3.0F});

  // AttributeMatrix tuple shape is slowest-to-fastest (Z, Y, X)
  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, std::vector<usize>{4, 1, 2}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto* dataArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_DataArrayName, {4, 1, 2}, {1}, cellAM->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  for(usize i = 0; i < dataStore.getNumberOfTuples(); i++)
  {
    dataStore[i] = static_cast<int32>(i);
  }

  // A loose DataArray directly under the geometry (outside the cell AttributeMatrix) to exercise
  // the loose-child copy path in preflight.
  auto* looseArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_LooseArrayName, {4}, {1}, imageGeom->getId());
  auto& looseStore = looseArray->getDataStoreRef();
  for(usize i = 0; i < looseStore.getNumberOfTuples(); i++)
  {
    looseStore[i] = static_cast<int32>(100 + i);
  }

  return dataStructure;
}

// Writes the Z boundary positions file and returns its path.
fs::path writeZBoundsFile(const std::string& fileName, const std::vector<float32>& values)
{
  fs::path outputDir(unit_test::k_BinaryTestOutputDir.view());
  fs::create_directories(outputDir);
  fs::path filePath = outputDir / fileName;
  std::ofstream outFile(filePath);
  for(float32 value : values)
  {
    outFile << value << "\n";
  }
  return filePath;
}
} // namespace

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Valid Execution (New Geometry)", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  DataStructure dataStructure = createTestDataStructure();

  // Irregular Z boundaries: slices span [0,1], [1,3], [3,6], [6,10]; total Z extent = 10.
  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_new_geom.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
  const float32 newZRes = 2.0F;

  RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(newZRes));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_OutputGeometryPath));
  const auto& outputGeom = dataStructure.getDataRefAs<ImageGeom>(k_OutputGeometryPath);

  // newZDim = (size_t)(10 / 2) = 5; X and Y unchanged.
  const SizeVec3 outDims = outputGeom.getDimensions();
  REQUIRE(outDims[0] == 2);
  REQUIRE(outDims[1] == 1);
  REQUIRE(outDims[2] == 5);

  // Spacing: X and Y preserved, Z becomes newZRes.
  const FloatVec3 outSpacing = outputGeom.getSpacing();
  REQUIRE(outSpacing[0] == Approx(0.5F));
  REQUIRE(outSpacing[1] == Approx(0.75F));
  REQUIRE(outSpacing[2] == Approx(2.0F));

  // Origin preserved.
  const FloatVec3 outOrigin = outputGeom.getOrigin();
  REQUIRE(outOrigin[0] == Approx(1.0F));
  REQUIRE(outOrigin[1] == Approx(2.0F));
  REQUIRE(outOrigin[2] == Approx(3.0F));

  // Class 1 (Analytical) oracle — closed-form indirection lookup output[i] = input[map[i]].
  // Hand derivation of the new-plane -> old-plane map. For new plane i the source plane is the
  // largest iter in [1, origZ=4) with (i * newZRes) > zBound[iter], else 0. zBound = {0,1,3,6,10}:
  //   i=0: pos=0  -> 0 > {1,3,6}? no,no,no          -> plane 0
  //   i=1: pos=2  -> 2 > {1,3,6}? yes,no,no         -> plane 1
  //   i=2: pos=4  -> 4 > {1,3,6}? yes,yes,no        -> plane 2
  //   i=3: pos=6  -> 6 > {1,3,6}? yes,yes,no(strict)-> plane 2
  //   i=4: pos=8  -> 8 > {1,3,6}? yes,yes,yes       -> plane 3
  // map = [0,1,2,2,3]. Each source tuple's value equals its tuple index and each plane holds 2
  // tuples (X=2,Y=1), so plane p contributes source tuples {2p, 2p+1}. Expected output:
  //   plane0<-src0: 0,1 | plane1<-src1: 2,3 | plane2<-src2: 4,5 | plane3<-src2: 4,5 | plane4<-src3: 6,7
  const DataPath outputArrayPath = k_OutputGeometryPath.createChildPath(k_CellDataName).createChildPath(k_DataArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputArrayPath));
  const auto& outStore = dataStructure.getDataRefAs<Int32Array>(outputArrayPath).getDataStoreRef();

  const std::vector<int32> expected = {0, 1, 2, 3, 4, 5, 4, 5, 6, 7};
  REQUIRE(outStore.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    INFO(fmt::format("tuple index {}", i));
    REQUIRE(outStore[i] == expected[i]);
  }

  // Loose child data (outside the cell AM) is copied into the new geometry with values intact.
  const DataPath outputLoosePath = k_OutputGeometryPath.createChildPath(k_LooseArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputLoosePath));
  const auto& outLooseStore = dataStructure.getDataRefAs<Int32Array>(outputLoosePath).getDataStoreRef();
  REQUIRE(outLooseStore.getNumberOfTuples() == 4);
  for(usize i = 0; i < 4; i++)
  {
    REQUIRE(outLooseStore[i] == static_cast<int32>(100 + i));
  }

  // Original geometry should still exist (not in place).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_InputGeometryPath));
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Valid Execution (Spacing Exceeds Extent)", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  DataStructure dataStructure = createTestDataStructure();

  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_clamp.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});

  // Class 1 derivation of the newZDim clamp: newZ = (usize)(10 / 20) = 0 -> clamped to 1.
  // For the single new plane i=0, pos = 0 is not > any interior zBound, so the source is plane 0.
  // Output = one plane holding source tuples {0, 1}; Z spacing becomes 20.
  const float32 newZRes = 20.0F;

  RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(newZRes));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& outputGeom = dataStructure.getDataRefAs<ImageGeom>(k_OutputGeometryPath);
  const SizeVec3 outDims = outputGeom.getDimensions();
  REQUIRE(outDims[0] == 2);
  REQUIRE(outDims[1] == 1);
  REQUIRE(outDims[2] == 1);
  REQUIRE(outputGeom.getSpacing()[2] == Approx(20.0F));

  const DataPath outputArrayPath = k_OutputGeometryPath.createChildPath(k_CellDataName).createChildPath(k_DataArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputArrayPath));
  const auto& outStore = dataStructure.getDataRefAs<Int32Array>(outputArrayPath).getDataStoreRef();
  REQUIRE(outStore.getNumberOfTuples() == 2);
  REQUIRE(outStore[0] == 0);
  REQUIRE(outStore[1] == 1);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Valid Execution (In Place)", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  DataStructure dataStructure = createTestDataStructure();

  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_bounds.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});

  RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The result should have replaced the original geometry (same name), and no output-name geometry exists.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_InputGeometryPath));
  REQUIRE(dataStructure.getDataAs<ImageGeom>(k_OutputGeometryPath) == nullptr);

  const auto& resultGeom = dataStructure.getDataRefAs<ImageGeom>(k_InputGeometryPath);
  REQUIRE(resultGeom.getDimensions()[2] == 5);
  REQUIRE(resultGeom.getSpacing()[2] == Approx(2.0F));
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Invalid Parameters", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  DataStructure dataStructure = createTestDataStructure();
  RegularizeZSpacingFilter filter;

  SECTION("Non-positive Z spacing")
  {
    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_bounds.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
    args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
    args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(0.0F));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5555);
  }

  SECTION("File with too few values")
  {
    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_short.txt", {0.0F, 1.0F, 3.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
    args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
    args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5557);
  }

  SECTION("Non-monotonic Z boundary values")
  {
    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_nonmono.txt", {0.0F, 3.0F, 1.0F, 6.0F, 10.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
    args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
    args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5558);
  }

  SECTION("Zero total Z extent")
  {
    // Equal values are monotonically non-decreasing, so this reaches the extent check (-5559).
    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_zeroextent.txt", {0.0F, 0.0F, 0.0F, 0.0F, 0.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
    args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
    args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5559);
  }

  SECTION("Missing cell Attribute Matrix")
  {
    DataStructure geomOnlyDataStructure;
    auto* geomOnly = ImageGeom::Create(geomOnlyDataStructure, k_ImageGeometryName);
    geomOnly->setDimensions(SizeVec3{2, 1, 4});
    geomOnly->setSpacing(FloatVec3{0.5F, 0.75F, 1.0F});
    geomOnly->setOrigin(FloatVec3{1.0F, 2.0F, 3.0F});

    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_noam.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
    args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
    args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

    auto preflightResult = filter.preflight(geomOnlyDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5560);
  }

  SECTION("Non-DataArray member in cell Attribute Matrix")
  {
    // A StringArray cannot be resampled; preflight must reject it cleanly rather than throw.
    const DataPath cellAMPath = k_InputGeometryPath.createChildPath(k_CellDataName);
    auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(cellAMPath);
    StringArray::CreateWithValues(dataStructure, "Labels", {4, 1, 2}, std::vector<std::string>(8, "label"), cellAM.getId());

    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_stringarray.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
    args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
    args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5561);
  }
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RegularizeZSpacingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RegularizeZSpacingFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RegularizeZSpacingFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RegularizeZSpacingFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      // Only the DataContainer name survives conversion of the legacy CellAttributeMatrixPath;
      // the filter binds to the geometry's assigned cell AttributeMatrix (see V&V report, delta 5).
      CHECK(args.value<DataPath>(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<FileSystemPathParameter::ValueType>(RegularizeZSpacingFilter::k_InputFile_Key) == fs::path("/test/z_positions.txt"));
      CHECK(args.value<float32>(RegularizeZSpacingFilter::k_NewZRes_Key) == 0.5f);
      // Legacy had no new-geometry option; the unconverted default must stay in place.
      CHECK(args.value<bool>(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key) == true);
    }
  }
}
