#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "SimplnxCore/Filters/RegularizeZSpacingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <algorithm>
#include <array>
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

/**
 * @brief Creates four Z slices of features and independent Feature and Ensemble data.
 * @return Image geometry with FeatureId z + 1 at each cell (0, 0, z).
 */
DataStructure CreateFeatureRegularizeFixture()
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({1, 1, 4});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{4, 1, 1}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", cellData->getShape(), {1}, cellData->getId());
  REQUIRE(featureIds != nullptr);
  for(usize z = 0; z < 4; ++z)
  {
    (*featureIds)[z] = static_cast<int32>(z + 1);
  }

  auto* featureData = AttributeMatrix::Create(dataStructure, "Cell Feature Data", ShapeType{5}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto* numElements = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureData->getId());
  REQUIRE(numElements != nullptr);
  numElements->fill(1);
  (*numElements)[0] = 0;
  auto* neighbors = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureData->getId());
  REQUIRE(neighbors != nullptr);
  neighbors->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureData->getId()) != nullptr);

  auto* ensembleData = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", ShapeType{2}, imageGeom->getId());
  REQUIRE(ensembleData != nullptr);
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleData->getId());
  REQUIRE(crystalStructures != nullptr);
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;
  return dataStructure;
}

/**
 * @brief Samples source Z slices 0 and 2 at positions 0 and 4, removing features 2 and 4.
 * @param clear Whether to clear the destination Feature Attribute Matrix.
 * @param renumber Stored renumber setting, effective only when clear is true.
 * @return Explicit arguments for every parameter; preserves the source geometry.
 */
Arguments MakeFeatureRegularizeArgs(bool clear, bool renumber)
{
  Arguments args;
  args.insert(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insert(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(writeZBoundsFile("RegularizeZSpacing_features.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F})));
  args.insert(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(4.0F));
  args.insert(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(DataPath({"Regularized"})));
  args.insert(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(clear));
  args.insert(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(renumber));
  args.insert(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Feature Data"})));
  args.insert(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Valid Execution (New Geometry)", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = createTestDataStructure();

  // Irregular Z boundaries: slices span [0,1], [1,3], [3,6], [6,10]; total Z extent = 10.
  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_new_geom.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
  const float32 newZRes = 2.0F;

  RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Supported Extensions", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  UnitTest::LoadPlugins();
  const std::string extension = GENERATE(".dat", ".csv");
  CAPTURE(extension);
  DataStructure dataStructure = createTestDataStructure();
  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_supported" + extension, {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});

  const RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Unsupported Extension", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  UnitTest::LoadPlugins();
  const DataStructure dataStructure = createTestDataStructure();
  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_unsupported.zpos", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});

  const RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -3003);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Missing Extension", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  UnitTest::LoadPlugins();
  const DataStructure dataStructure = createTestDataStructure();
  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_no_extension", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
  REQUIRE(fs::exists(zBoundsFile));
  REQUIRE_FALSE(zBoundsFile.has_extension());

  const RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(zBoundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -3002);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Valid Execution (Spacing Exceeds Extent)", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = createTestDataStructure();

  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_clamp.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});

  // Class 1 derivation of the newZDim clamp: newZ = (usize)(10 / 20) = 0 -> clamped to 1.
  // For the single new plane i=0, pos = 0 is not > any interior zBound, so the source is plane 0.
  // Output = one plane holding source tuples {0, 1}; Z spacing becomes 20.
  const float32 newZRes = 20.0F;

  RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
  UnitTest::LoadPlugins();
  DataStructure dataStructure = createTestDataStructure();

  const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_bounds.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});

  RegularizeZSpacingFilter filter;
  Arguments args;
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
  UnitTest::LoadPlugins();
  DataStructure dataStructure = createTestDataStructure();
  RegularizeZSpacingFilter filter;

  SECTION("Non-positive Z spacing")
  {
    const fs::path zBoundsFile = writeZBoundsFile("RegularizeZSpacing_bounds.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F});
    Arguments args;
    args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
    args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
    args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
    args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
    args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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
    args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
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

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: real HDF5 slice-copy oracle", "[SimplnxCore][RegularizeZSpacingFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool inPlace = GENERATE(false, true);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  auto dataStructure = createTestDataStructure();
  for(const auto& path : {k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath(k_DataArrayName), k_InputGeometryPath.createChildPath(k_LooseArrayName)})
  {
    auto array = dataStructure.getSharedDataAs<IDataArray>(path);
    REQUIRE(array != nullptr);
    REQUIRE(ConvertIDataArray(array, "HDF5-OOC"));
    REQUIRE(array->getIDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  const auto boundsFile = writeZBoundsFile("regularize_z_ooc_bounds.txt", {0, 1, 3, 6, 10});
  RegularizeZSpacingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputGeometryPath));
  args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key, std::make_any<fs::path>(boundsFile));
  args.insertOrAssign(RegularizeZSpacingFilter::k_NewZRes_Key, std::make_any<float32>(2.0F));
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(inPlace));
  args.insertOrAssign(RegularizeZSpacingFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_OutputGeometryPath));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto resultPath = inPlace ? k_InputGeometryPath : k_OutputGeometryPath;
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(resultPath));
  REQUIRE(dataStructure.getDataRefAs<ImageGeom>(resultPath).getDimensions() == SizeVec3(2, 1, 5));
  const auto outputPath = resultPath.createChildPath(k_CellDataName).createChildPath(k_DataArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputPath));
  const auto& output = dataStructure.getDataRefAs<Int32Array>(outputPath);
  REQUIRE(output.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  // The independently derived source-plane map is [0,1,2,2,3].
  const std::array<int32, 10> expected = {0, 1, 2, 3, 4, 5, 4, 5, 6, 7};
  REQUIRE(output.getSize() == expected.size());
  for(usize tupleIdx = 0; tupleIdx < expected.size(); tupleIdx++)
  {
    REQUIRE(output[tupleIdx] == expected[tupleIdx]);
  }
  const auto loosePath = resultPath.createChildPath(k_LooseArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(loosePath));
  const auto& loose = dataStructure.getDataRefAs<Int32Array>(loosePath);
  REQUIRE(loose.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  for(usize tupleIdx = 0; tupleIdx < 4; tupleIdx++)
  {
    REQUIRE(loose[tupleIdx] == static_cast<int32>(100 + tupleIdx));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RegularizeZSpacingFilter][BackwardsCompatibility]")
{
  UnitTest::LoadPlugins();
  auto app = Application::GetOrCreateInstance();
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

      Arguments args = pipelineFilter->getArguments();
      // Only the DataContainer name survives conversion of the legacy CellAttributeMatrixPath;
      // the filter binds to the geometry's assigned cell AttributeMatrix (see V&V report, delta 5).
      CHECK(args.value<DataPath>(RegularizeZSpacingFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<FileSystemPathParameter::ValueType>(RegularizeZSpacingFilter::k_InputFile_Key) == fs::path("/test/z_positions.txt"));
      CHECK(args.value<float32>(RegularizeZSpacingFilter::k_NewZRes_Key) == 0.5f);
      CHECK_FALSE(args.value<bool>(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key));
      CHECK(args.value<bool>(RegularizeZSpacingFilter::k_RenumberFeatures_Key) == false);
      CHECK(args.value<DataPath>(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"Cell Data", "FeatureIds"}));
      CHECK(args.value<DataPath>(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key) == DataPath({"Cell Feature Data"}));
      // Legacy had no new-geometry option; the unconverted default must stay in place.
      CHECK(args.value<bool>(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key) == true);

      // Supply only the legacy inputs: a geometry with cell data and no Feature Attribute Matrix.
      auto ds = createTestDataStructure();
      auto* imageGeom = ds.getDataAs<ImageGeom>(k_InputGeometryPath);
      REQUIRE(imageGeom != nullptr);
      REQUIRE(imageGeom->rename("DataContainer"));
      REQUIRE(ds.getDataAs<AttributeMatrix>(DataPath({"DataContainer", "Cell Feature Data"})) == nullptr);
      args.insertOrAssign(RegularizeZSpacingFilter::k_InputFile_Key,
                          std::make_any<FileSystemPathParameter::ValueType>(writeZBoundsFile("RegularizeZSpacing_legacy.txt", {0.0F, 1.0F, 3.0F, 6.0F, 10.0F})));
      pipelineFilter->setArguments(args);
      CHECK(pipeline.preflight(ds, false));
      UnitTest::CheckArraysInheritTupleDims(ds);
    }
  }
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear ON, renumber OFF preflight", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRegularizeArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  const auto it = std::find_if(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; });
  REQUIRE(it != result.outputValues.end());
  CHECK(it->value.find("Image/Cell Feature Data/NumElements") != std::string::npos);
  CHECK(it->value.find("Image/Cell Feature Data/NeighborList") != std::string::npos);
  CHECK(it->value.find("Image/Cell Feature Data/Names") != std::string::npos);
  CHECK(it->value.find("Ensemble") == std::string::npos);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Invalidated NeighborLists" || v.name == "Feature Data Not Cleared"; }));
  CHECK(filter.parametersVersion() == 2);
  CHECK(filter.getDefaultArguments().value<bool>(RegularizeZSpacingFilter::k_ClearFeatureAttributeMatrix_Key));
  CHECK_FALSE(filter.getDefaultArguments().value<bool>(RegularizeZSpacingFilter::k_RenumberFeatures_Key));
  auto applyResult = result.outputActions.value().applyRegular(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  const auto* am = ds.getDataAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"}));
  REQUIRE(am != nullptr);
  CHECK(am->getShape() == ShapeType{5});
  CHECK(am->getSize() == 0);
  const auto* ensemble = ds.getDataAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(ensemble != nullptr);
  REQUIRE(ensemble->getNumberOfTuples() == 2);
  CHECK((*ensemble)[0] == 999);
  CHECK((*ensemble)[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear OFF preflight", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRegularizeArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  CHECK(std::any_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear OFF ignores stored renumber", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  Arguments args = MakeFeatureRegularizeArgs(false, true);
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "DoesNotExist"})));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto* am = ds.getDataAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"}));
  REQUIRE(am != nullptr);
  CHECK(am->getShape() == ShapeType{5});
  CHECK(am->getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: StrictFeatureIds preflight", "[SimplnxCore][RegularizeZSpacingFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  const auto* imageGeom = ds.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(imageGeom != nullptr);
  const auto* cellData = imageGeom->getCellData();
  REQUIRE(cellData != nullptr);
  auto* otherData = AttributeMatrix::Create(ds, "Other Cell Data", cellData->getShape(), imageGeom->getId());
  REQUIRE(otherData != nullptr);
  auto* misplacedIds = UnitTest::CreateTestDataArray<int32>(ds, "MisplacedFeatureIds", otherData->getShape(), {1}, otherData->getId());
  REQUIRE(misplacedIds != nullptr);
  misplacedIds->fill(1);

  RegularizeZSpacingFilter filter;
  Arguments args = MakeFeatureRegularizeArgs(true, true);
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Other Cell Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == -67433);
    CHECK(result.outputActions.errors().front().message ==
          "**Cell Feature Ids** DataArray 'Image/Other Cell Data/MisplacedFeatureIds' must be in the Cell Attribute Matrix 'Image/Cell Data' of the selected geometry 'Image'. "
          "Select the DataArray from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(RegularizeZSpacingFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Cell Attribute Matrix")
  {
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts another path to the same Cell Attribute Matrix")
  {
    auto* aliasParent = DataGroup::Create(ds, "Alias");
    REQUIRE(aliasParent != nullptr);
    REQUIRE(ds.setAdditionalParent(cellData->getId(), aliasParent->getId()));
    args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Alias", "Cell Data", "FeatureIds"})));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto* ids = ds.getDataAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"}));
    REQUIRE(ids != nullptr);
    REQUIRE(ids->getNumberOfTuples() == 2);
    CHECK((*ids)[0] == 1);
    CHECK((*ids)[1] == 2);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Renumber requires one Feature Id per cell", "[SimplnxCore][RegularizeZSpacingFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureRegularizeFixture();
  auto* cellData = ds.getDataAs<AttributeMatrix>(DataPath({"Image", "Cell Data"}));
  REQUIRE(cellData != nullptr);
  auto resizeResult = cellData->resizeTuples({3, 1, 1});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  RegularizeZSpacingFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRegularizeArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors().front().code == -67434);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Renumber validates source Feature Id range", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureRegularizeFixture();
  auto* ids = ds.getDataAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(ids != nullptr);
  const int32 invalidId = GENERATE(-1, 5);
  // Slice 1 is not sampled. Validation must still reject its invalid source ID.
  (*ids)[1] = invalidId;
  RegularizeZSpacingFilter filter;
  auto args = MakeFeatureRegularizeArgs(true, true);
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors().front().code == (invalidId < 0 ? -5355 : -5351));
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear ON Feature AM errors", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  // An Attribute Matrix that is not a child of "Image".
  REQUIRE(AttributeMatrix::Create(ds, "Elsewhere", std::vector<usize>{5}) != nullptr);
  RegularizeZSpacingFilter filter;

  Arguments args = MakeFeatureRegularizeArgs(true, false);
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.outputActions);
  REQUIRE(notChild.outputActions.errors().size() == 1);
  CHECK(notChild.outputActions.errors().front().code == -67431);
  CHECK(notChild.outputActions.errors().front().message.find("Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.") != std::string::npos);

  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data"})));
  auto isCellAm = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isCellAm.outputActions);
  REQUIRE(isCellAm.outputActions.errors().size() == 1);
  CHECK(isCellAm.outputActions.errors().front().code == -67432);

  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Missing"})));
  auto missing = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.outputActions);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear ON accepts shared parent paths", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureRegularizeFixture();
  auto* aliases = DataGroup::Create(ds, "Aliases");
  REQUIRE(aliases != nullptr);
  const auto* imageGeom = ds.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(imageGeom != nullptr);
  const auto* featureAm = ds.getDataAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  REQUIRE(featureAm != nullptr);
  REQUIRE(ds.setAdditionalParent(imageGeom->getCellData()->getId(), aliases->getId()));
  REQUIRE(ds.setAdditionalParent(featureAm->getId(), aliases->getId()));
  const bool renumber = GENERATE(false, true);
  auto args = MakeFeatureRegularizeArgs(true, renumber);
  args.insertOrAssign(RegularizeZSpacingFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Cell Data", "FeatureIds"})));
  args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Aliases", "Cell Feature Data"})));
  RegularizeZSpacingFilter filter;
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getNumberOfTuples() == (renumber ? 3 : 5));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  CHECK(ids[0] == 1);
  CHECK(ids[1] == (renumber ? 2 : 3));
  CHECK(featureAm->getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear ON, renumber OFF execute", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  auto result = filter.execute(ds, MakeFeatureRegularizeArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // The cell FeatureIds keep their original values (1 and 3 for source Z slices 0 and 2).
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {1, 3};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  CHECK(ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})).getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear ON, renumber ON execute", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  auto preflight = filter.preflight(ds, MakeFeatureRegularizeArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto info = std::find_if(preflight.outputValues.begin(), preflight.outputValues.end(), [](const auto& value) { return value.name == "Cleared Feature Data"; });
  REQUIRE(info != preflight.outputValues.end());
  CHECK(info->value.find("Recompute any of the following Data Arrays:\n") != std::string::npos);
  CHECK(info->value.find("\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after the resample.") != std::string::npos);
  auto result = filter.execute(ds, MakeFeatureRegularizeArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"}));
  // Features 1 and 3 remain, plus row 0.
  CHECK(am.getShape() == std::vector<usize>{3});
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(ds, MakeFeatureRegularizeArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Feature Data", "NumElements"})));
  const auto& numElements = ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 1, 1, 1, 1};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Regularized", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(ds.getDataRefAs<NeighborList<int32>>(DataPath({"Regularized", "Cell Feature Data", "NeighborList"})));
  const auto& neighbors = ds.getDataRefAs<NeighborList<int32>>(DataPath({"Regularized", "Cell Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<StringArray>(DataPath({"Regularized", "Cell Feature Data", "Names"})));
  const auto& names = ds.getDataRefAs<StringArray>(DataPath({"Regularized", "Cell Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Regularized", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {1, 3};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Regularized", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Clear ON in place", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRegularizeFixture();
  RegularizeZSpacingFilter filter;
  Arguments args = MakeFeatureRegularizeArgs(true, true);
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getShape() == std::vector<usize>{3});
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Image"})));
  const auto& imageGeom = ds.getDataRefAs<ImageGeom>(DataPath({"Image"}));
  CHECK(imageGeom.getDimensions() == SizeVec3(1, 1, 2));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  CHECK(ds.getData(DataPath({"Regularized"})) == nullptr);
  CHECK(ds.getData(DataPath({".Image"})) == nullptr);
  CHECK(ds.getData(DataPath({".regularized_image_geometry"})) == nullptr);
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Feature AM nested alias regression", "[SimplnxCore][RegularizeZSpacingFilter][NestedFeatureAlias]")
{
  UnitTest::LoadPlugins();
  const bool clear = GENERATE(true, false);
  const bool renumber = GENERATE(false, true);
  const bool selectNested = GENERATE(false, true);
  auto ds = CreateFeatureRegularizeFixture();
  const DataPath sourcePath({"Image"});
  const DataPath outputPath({"Regularized"});
  const auto featurePath = sourcePath.createChildPath("Cell Feature Data");
  const auto* features = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(features != nullptr);
  const auto featureId = features->getId();
  const auto shape = features->getShape();
  const auto countsPath = featurePath.createChildPath("NumElements");
  const auto& counts = ds.getDataRefAs<Int32Array>(countsPath);
  std::vector<int32> originalCounts(counts.getSize());
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    originalCounts[i] = counts[i];
  }
  auto* nested = DataGroup::Create(ds, "Misc", ds.getData(sourcePath)->getId());
  REQUIRE(nested != nullptr);
  auto* keep = UnitTest::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nested->getId());
  REQUIRE(keep != nullptr);
  (*keep)[0] = 123;
  REQUIRE(ds.setAdditionalParent(featureId, nested->getId()));
  auto* outside = DataGroup::Create(ds, "Outside");
  REQUIRE(outside != nullptr);
  REQUIRE(ds.setAdditionalParent(nested->getId(), outside->getId()));
  const auto parents = ds.getData(featurePath)->getParentIds();
  auto args = MakeFeatureRegularizeArgs(clear, renumber);
  if(selectNested)
  {
    args.insertOrAssign(RegularizeZSpacingFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(sourcePath.createChildPath("Misc").createChildPath("Cell Feature Data")));
  }
  RegularizeZSpacingFilter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool foundNestedCopy = false;
  const auto nestedOutputPath = outputPath.createChildPath("Misc").createChildPath("Cell Feature Data");
  for(const auto& action : preflight.outputActions.value().actions)
  {
    const auto* copy = dynamic_cast<const CopyDataObjectAction*>(action.get());
    if(copy != nullptr && copy->path() == sourcePath.createChildPath("Misc"))
    {
      foundNestedCopy = true;
      bool declaresFeature = false;
      for(const auto& created : copy->getAllCreatedPaths())
      {
        if(created.rebase(nestedOutputPath, DataPath{}).has_value())
        {
          declaresFeature = true;
        }
      }
      CHECK(declaresFeature == !clear);
    }
  }
  REQUIRE(foundNestedCopy);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* outputFeatures = ds.getDataAs<AttributeMatrix>(outputPath.createChildPath("Cell Feature Data"));
  REQUIRE(outputFeatures != nullptr);
  CHECK(outputFeatures->getSize() == (clear ? 0 : 3));
  CHECK((ds.getData(nestedOutputPath) == nullptr) == clear);
  CHECK(ds.getDataRefAs<Int32Array>(outputPath.createChildPath("Misc").createChildPath("Keep"))[0] == 123);
  CHECK(ds.getData(featurePath)->getId() == featureId);
  CHECK(ds.getData(featurePath)->getParentIds() == parents);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getShape() == shape);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getSize() == 3);
  CHECK(ds.getData(DataPath({"Outside", "Misc", "Cell Feature Data"}))->getId() == featureId);
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    CHECK(ds.getDataRefAs<Int32Array>(countsPath)[i] == originalCounts[i]);
    if(!clear)
    {
      CHECK(ds.getDataRefAs<Int32Array>(nestedOutputPath.createChildPath("NumElements"))[i] == originalCounts[i]);
    }
  }
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Reject nonnumeric Feature Ids", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool strings = GENERATE(true, false);
  auto ds = CreateFeatureRegularizeFixture();
  const auto idsPath = DataPath({"Image", "Cell Data", "FeatureIds"});
  const auto* original = ds.getDataAs<Int32Array>(idsPath);
  REQUIRE(original != nullptr);
  const auto shape = original->getTupleShape();
  const auto tuples = original->getNumberOfTuples();
  const auto parentId = ds.getData(idsPath.getParent())->getId();
  REQUIRE(ds.removeData(idsPath));
  if(strings)
  {
    REQUIRE(StringArray::CreateWithValues(ds, idsPath.getTargetName(), shape, std::vector<std::string>(tuples), parentId) != nullptr);
  }
  else
  {
    REQUIRE(NeighborList<int32>::Create(ds, idsPath.getTargetName(), shape, parentId) != nullptr);
  }
  RegularizeZSpacingFilter filter;
  auto args = MakeFeatureRegularizeArgs(true, true);
  REQUIRE_NOTHROW(filter.preflight(ds, args));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -5561);
}

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Reject another geometry element Feature AM", "[SimplnxCore][RegularizeZSpacingFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureRegularizeFixture();
  const auto featurePath = DataPath({"Image", "Cell Feature Data"});
  const auto* features = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(features != nullptr);
  auto* other = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(other != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {features->getNumberOfTuples()}, {3}, other->getId());
  REQUIRE(vertices != nullptr);
  other->setVertices(*vertices);
  REQUIRE(ds.setAdditionalParent(features->getId(), other->getId()));
  other->setVertexAttributeMatrix(*features);
  RegularizeZSpacingFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRegularizeArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67460);
  CHECK(result.outputActions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(other->getVertexAttributeMatrix() == features);
  CHECK(features->getSize() > 0);
}

namespace
{
/**
 * @class RegularizeFailingReadStore
 * @brief Fails the second bulk read after one destination slice is written.
 */
class RegularizeFailingReadStore : public DataStore<int32>
{
public:
  /** @brief Creates four source slices without enabling failure until bulk transfer. */
  RegularizeFailingReadStore()
  : DataStore<int32>(ShapeType{4, 1, 1}, ShapeType{1}, 0)
  {
  }
  /** @brief Releases the resident test values. */
  ~RegularizeFailingReadStore() noexcept override = default;
  RegularizeFailingReadStore(const RegularizeFailingReadStore&) = delete;
  RegularizeFailingReadStore(RegularizeFailingReadStore&&) = delete;
  RegularizeFailingReadStore& operator=(const RegularizeFailingReadStore&) = delete;
  RegularizeFailingReadStore& operator=(RegularizeFailingReadStore&&) = delete;

  /**
   * @brief Injects error -95120 on the second slice read.
   * @param startIndex Specifies the first flat value index.
   * @param buffer Receives the source slice values.
   * @return The injected failure or the resident transfer result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    if(++readCalls == 2)
    {
      return MakeErrorResult(-95120, "Injected second slice transfer failure.");
    }
    return DataStore<int32>::copyIntoBuffer(startIndex, buffer);
  }
  mutable usize readCalls = 0;
};
} // namespace

TEST_CASE("SimplnxCore::RegularizeZSpacingFilter: Transfer failure preserves source", "[SimplnxCore][RegularizeZSpacingFilter]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureRegularizeFixture();
  auto* source = ds.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(source != nullptr);
  const auto sourceId = source->getId();
  auto* ids = ds.getDataAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(ids != nullptr);
  const auto arrayId = ids->getId();
  auto store = std::make_shared<RegularizeFailingReadStore>();
  for(usize i = 0; i < 4; ++i)
  {
    store->setValue(i, static_cast<int32>(i + 1));
  }
  auto setResult = ids->setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(setResult);
  auto args = MakeFeatureRegularizeArgs(true, true);
  args.insertOrAssign(RegularizeZSpacingFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  RegularizeZSpacingFilter filter;
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == -95120);
  CHECK(store->readCalls == 2);
  const auto* retained = ds.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(retained != nullptr);
  CHECK(retained->getId() == sourceId);
  CHECK(retained->getDimensions() == SizeVec3{1, 1, 4});
  const auto* retainedIds = ds.getDataAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(retainedIds != nullptr);
  CHECK(retainedIds->getId() == arrayId);
  for(usize i = 0; i < 4; ++i)
  {
    CHECK((*retainedIds)[i] == static_cast<int32>(i + 1));
  }
  const auto* features = ds.getDataAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  REQUIRE(features != nullptr);
  CHECK(features->getNumberOfTuples() == 5);
  CHECK(features->getSize() == 3);
  // The successful first slice remains in the temporary output, without compaction.
  const auto* partialIds = ds.getDataAs<Int32Array>(DataPath({".regularized_image_geometry", "Cell Data", "FeatureIds"}));
  REQUIRE(partialIds != nullptr);
  CHECK((*partialIds)[0] == 1);
  CHECK((*partialIds)[1] == 0);
  const auto* partialFeatures = ds.getDataAs<AttributeMatrix>(DataPath({".regularized_image_geometry", "Cell Feature Data"}));
  REQUIRE(partialFeatures != nullptr);
  CHECK(partialFeatures->getNumberOfTuples() == 5);
}
