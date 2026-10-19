#include "SimplnxCore/Filters/ResampleImageGeomFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"

#include <algorithm>
#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const ChoicesParameter::ValueType k_SpacingModeIndex = 0;
const ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;

/**
 * @brief Creates four columns of features and independent Feature and Ensemble data.
 * @return Image geometry with FeatureId x + 1 at each cell (x, y, 0).
 */
DataStructure CreateFeatureResampleFixture()
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({4, 4, 1});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 4, 4}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", cellData->getShape(), {1}, cellData->getId());
  REQUIRE(featureIds != nullptr);
  for(usize y = 0; y < 4; ++y)
  {
    for(usize x = 0; x < 4; ++x)
    {
      (*featureIds)[y * 4 + x] = static_cast<int32>(x + 1);
    }
  }

  auto* featureData = AttributeMatrix::Create(dataStructure, "Cell Feature Data", ShapeType{5}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto* numElements = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureData->getId());
  REQUIRE(numElements != nullptr);
  numElements->fill(4);
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
 * @brief Doubles X spacing so the output samples source columns 0 and 2.
 * @param clear Whether to clear the destination Feature Attribute Matrix.
 * @param renumber Stored renumber setting, effective only when clear is true.
 * @return Arguments that preserve the source and create the Resampled geometry.
 */
Arguments MakeFeatureResampleArgs(bool clear, bool renumber)
{
  Arguments args;
  args.insert(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
  args.insert(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(std::vector<float32>{2.0F, 1.0F, 1.0F}));
  args.insert(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insert(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(DataPath({"Resampled"})));
  args.insert(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(clear));
  args.insert(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(renumber));
  args.insert(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Feature Data"})));
  args.insert(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  return args;
}
} // namespace

struct CompareDataArrayFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right, usize start = 0)
  {
    UnitTest::CompareDataArrays<T>(left, right, start);
  }
};

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON, renumber OFF preflight", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto result = filter.preflight(ds, MakeFeatureResampleArgs(true, false));
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
  auto applyResult = result.outputActions.value().applyRegular(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  const auto* am = ds.getDataAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  REQUIRE(am != nullptr);
  CHECK(am->getShape() == ShapeType{5});
  CHECK(am->getSize() == 0);
  const auto* ensemble = ds.getDataAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(ensemble != nullptr);
  REQUIRE(ensemble->getNumberOfTuples() == 2);
  CHECK((*ensemble)[0] == 999);
  CHECK((*ensemble)[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear OFF preflight", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto result = filter.preflight(ds, MakeFeatureResampleArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  CHECK(std::any_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear OFF ignores stored renumber", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  Arguments args = MakeFeatureResampleArgs(false, true);
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "DoesNotExist"})));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto* am = ds.getDataAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  REQUIRE(am != nullptr);
  CHECK(am->getShape() == ShapeType{5});
  CHECK(am->getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: StrictFeatureIds preflight", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  const auto* imageGeom = ds.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(imageGeom != nullptr);
  const auto* cellData = imageGeom->getCellData();
  REQUIRE(cellData != nullptr);
  auto* otherData = AttributeMatrix::Create(ds, "Other Cell Data", cellData->getShape(), imageGeom->getId());
  REQUIRE(otherData != nullptr);
  auto* misplacedIds = UnitTest::CreateTestDataArray<int32>(ds, "MisplacedFeatureIds", otherData->getShape(), {1}, otherData->getId());
  REQUIRE(misplacedIds != nullptr);
  misplacedIds->fill(1);

  ResampleImageGeomFilter filter;
  Arguments args = MakeFeatureResampleArgs(true, true);
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Other Cell Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == -55505);
    CHECK(result.outputActions.errors().front().message ==
          "**Cell Feature Ids** DataArray 'Image/Other Cell Data/MisplacedFeatureIds' must be in the Cell Attribute Matrix 'Image/Cell Data' of the selected Image Geometry 'Image'. "
          "Select the DataArray from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Cell Attribute Matrix")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON Feature AM errors", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  // An Attribute Matrix that is not a child of "Image".
  REQUIRE(AttributeMatrix::Create(ds, "Elsewhere", std::vector<usize>{5}) != nullptr);
  ResampleImageGeomFilter filter;

  Arguments args = MakeFeatureResampleArgs(true, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.outputActions);
  REQUIRE(notChild.outputActions.errors().size() == 1);
  CHECK(notChild.outputActions.errors().front().code == -55503);

  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data"})));
  auto isCellAm = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isCellAm.outputActions);
  REQUIRE(isCellAm.outputActions.errors().size() == 1);
  CHECK(isCellAm.outputActions.errors().front().code == -55504);

  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Missing"})));
  auto missing = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.outputActions);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON, renumber OFF execute", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto result = filter.execute(ds, MakeFeatureResampleArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // The cell FeatureIds keep their original values (1 and 3 for source x = 0 and 2).
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 3, 1, 3, 1, 3, 1, 3};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  CHECK(ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})).getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON, renumber ON execute", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto preflight = filter.preflight(ds, MakeFeatureResampleArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto info = std::find_if(preflight.outputValues.begin(), preflight.outputValues.end(), [](const auto& value) { return value.name == "Cleared Feature Data"; });
  REQUIRE(info != preflight.outputValues.end());
  CHECK(info->value.find("Recompute any of the following Data Arrays:\n") != std::string::npos);
  CHECK(info->value.find("\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after the resample.") != std::string::npos);
  auto result = filter.execute(ds, MakeFeatureResampleArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  // Features 1 and 3 remain, plus row 0.
  CHECK(am.getShape() == std::vector<usize>{3});
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 2, 1, 2, 1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(ds, MakeFeatureResampleArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Feature Data", "NumElements"})));
  const auto& numElements = ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 4, 4, 4, 4};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(ds.getDataRefAs<NeighborList<int32>>(DataPath({"Resampled", "Cell Feature Data", "NeighborList"})));
  const auto& neighbors = ds.getDataRefAs<NeighborList<int32>>(DataPath({"Resampled", "Cell Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<StringArray>(DataPath({"Resampled", "Cell Feature Data", "Names"})));
  const auto& names = ds.getDataRefAs<StringArray>(DataPath({"Resampled", "Cell Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 3, 1, 3, 1, 3, 1, 3};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON in place", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  Arguments args = MakeFeatureResampleArgs(true, true);
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getShape() == std::vector<usize>{3});
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Image"})));
  const auto& imageGeom = ds.getDataRefAs<ImageGeom>(DataPath({"Image"}));
  CHECK(imageGeom.getDimensions() == SizeVec3(2, 4, 1));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 2, 1, 2, 1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  CHECK(ds.getData(DataPath({"Resampled"})) == nullptr);
  CHECK(ds.getData(DataPath({".Image"})) == nullptr);
  CHECK(ds.getData(DataPath({".resampled_image_geometry"})) == nullptr);
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Shared utility defaults preserve Feature data", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureResampleFixture();
  // Image-stack readers leave both new fields at their defaults.
  ResampleImageGeomInputValues inputValues{};
  CHECK_FALSE(inputValues.ClearFeatureAttributeMatrix);
  CHECK_FALSE(inputValues.ReportUnclearedFeatureData);
  inputValues.SelectedImageGeometryPath = DataPath({"Image"});
  inputValues.CreatedImageGeometryPath = DataPath({"Resampled"});
  inputValues.Spacing = {2.0F, 1.0F, 1.0F};
  auto result = PreflightImageGeometryResample(ds, inputValues);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(),
                     [](const auto& value) { return value.name == "Feature Data Not Cleared" || value.name == "Cleared Feature Data" || value.name == "Invalidated NeighborLists"; }));
  auto applyResult = result.outputActions.value().applyRegular(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  const auto* am = ds.getDataAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  REQUIRE(am != nullptr);
  CHECK(am->getShape() == ShapeType{5});
  CHECK(am->getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Invalid Parameters", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  int errCode;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11502;
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11500;
  }

  // We do not need to test the "Exact Dimensions" resampling mode because the parameter type "VectorUInt64Parameter" automatically keeps the input values in range

  // Assign values to the filter parameters
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == errCode);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 3D In Place", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{47, 50, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto destFeatureData = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  const auto destNewGrainData = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  const auto destPhaseData = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);
  REQUIRE(destFeatureData != nullptr);
  REQUIRE(destNewGrainData != nullptr);
  REQUIRE(destPhaseData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();
  const auto destNewGrainDataDims = destNewGrainData->getShape();
  const auto destPhaseDataDims = destPhaseData->getShape();
  // Get the exemplar data
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  const auto exemplarFeatureData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(exemplarFeatureData != nullptr);
  REQUIRE(destFeatureData->getNumberOfTuples() == exemplarFeatureData->getNumberOfTuples());
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());
  REQUIRE(destFeatureData->getSize() == 0);
  REQUIRE(destNewGrainData->getSize() == 1);
  REQUIRE(destPhaseData->getSize() == 2);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
  const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, destNewGrainDataPath).value();
  for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
  const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, destPhaseDataPath).value();
  for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 3D Save Geometry", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100.str() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{47, 50, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto destFeatureData = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  const auto destNewGrainData = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  const auto destPhaseData = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  const auto exemplarFeatureData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);
  REQUIRE(destFeatureData != nullptr);
  REQUIRE(destNewGrainData != nullptr);
  REQUIRE(destPhaseData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();
  const auto destNewGrainDataDims = destNewGrainData->getShape();
  const auto destPhaseDataDims = destPhaseData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(exemplarFeatureData != nullptr);
  REQUIRE(destFeatureData->getNumberOfTuples() == exemplarFeatureData->getNumberOfTuples());
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());
  REQUIRE(destFeatureData->getSize() == 0);
  REQUIRE(destNewGrainData->getSize() == 1);
  REQUIRE(destPhaseData->getSize() == 2);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
  const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, destNewGrainDataPath).value();
  for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
  const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, destPhaseDataPath).value();
  for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 2D In Place", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({"Image2dDataContainer"});
  DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
  DataPath destGeomPath({srcGeomPath.getTargetName()});
  DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{15, 15, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());

  // check that the geometry(s) and the attribute matrices are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 2D Save Geometry", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({"Image2dDataContainer"});
  DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
  DataPath destGeomPath({srcGeomPath.getTargetName() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());
  DataPath exemplarGeoPath;

  // Create default Parameters for the filter.
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{15, 15, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // check that the geometry(s) and the attribute matrices are all there
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ResampleImageGeomFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ResampleImageGeomFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ResampleImageGeomFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ResampleImageGeomFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key) == true);
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_RenumberFeatures_Key) == true);
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key) == false);
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CreatedImageGeometry_Key) == DataPath({"DataContainer"}));
    }
  }
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Child names containing the geometry name are kept", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  const DataPath destGeomPath({"Geom Out"});
  auto* geomPtr = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  geomPtr->setOrigin({0.0F, 0.0F, 0.0F});
  geomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setCellData(*elementAmPtr);
  auto* dataPtr =
      Int32Array::Create(dataStructure, "Data", DataStoreUtilities::CreateDataStore<int32>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                         elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr = Int32Array::Create(dataStructure, "Geom Values",
                                       DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(42);
  ResampleImageGeomFilter filter;
  Arguments args;
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0.5F, 0.5F, 0.5F}));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.outputActions.value().actions)
    {
      const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
      if(candidatePtr != nullptr && candidatePtr->path() == srcGeomPath.createChildPath("Geom Feature Data"))
      {
        copyActionPtr = candidatePtr;
        break;
      }
    }
    REQUIRE(copyActionPtr != nullptr);
    const auto createdPaths = copyActionPtr->getAllCreatedPaths();
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedValuesPath) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), renamedAmPath.createChildPath("Geom Out Values")) == createdPaths.end());
  }
  SECTION("Copied objects keep child names")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataAs<AttributeMatrix>(copiedAmPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Int32Array>(copiedValuesPath) != nullptr);
    REQUIRE_FALSE(dataStructure.containsData(renamedAmPath));
    REQUIRE(dataStructure.getDataAs<Int32Array>(destGeomPath.createChildPath("Cell Data").createChildPath("Data")) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
