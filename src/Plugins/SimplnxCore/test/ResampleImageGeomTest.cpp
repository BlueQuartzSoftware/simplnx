#include "SimplnxCore/Filters/ResampleImageGeomFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Output.hpp"
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
#include <any>
#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::unit_test;

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
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 4, 1});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());

  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 4, 4}, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* featureIdsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "FeatureIds", cellDataPtr->getShape(), {1}, cellDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  for(usize y = 0; y < 4; ++y)
  {
    for(usize x = 0; x < 4; ++x)
    {
      (*featureIdsPtr)[y * 4 + x] = static_cast<int32>(x + 1);
    }
  }

  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Cell Feature Data", ShapeType{5}, imageGeomPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* numElementsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureDataPtr->getId());
  REQUIRE(numElementsPtr != nullptr);
  numElementsPtr->fill(4);
  (*numElementsPtr)[0] = 0;
  auto* neighborsPtr = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureDataPtr->getId());
  REQUIRE(neighborsPtr != nullptr);
  neighborsPtr->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureDataPtr->getId()) != nullptr);

  auto* ensembleDataPtr = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", ShapeType{2}, imageGeomPtr->getId());
  REQUIRE(ensembleDataPtr != nullptr);
  auto* crystalStructuresPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleDataPtr->getId());
  REQUIRE(crystalStructuresPtr != nullptr);
  (*crystalStructuresPtr)[0] = 999;
  (*crystalStructuresPtr)[1] = 1;
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
    unit_test::CompareDataArrays<T>(left, right, start);
  }
};

TEST_CASE("SimplnxCore::ResampleImageGeom: Output keeps the input length unit", "[SimplnxCore][ResampleImageGeom]")
{
  auto app = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool removeOriginal = GENERATE(false, true);
  const auto units = GENERATE(IGeometry::LengthUnit::Millimeter, IGeometry::LengthUnit::Meter, IGeometry::LengthUnit::Unspecified);
  CAPTURE(removeOriginal, units);

  DataStructure dataStructure;
  const DataPath srcGeomPath({"Input Geometry"});
  const DataPath destGeomPath({"Resampled Geometry"});
  const DataPath inputDataPath = srcGeomPath.createChildPath("Cell Data").createChildPath("Values");
  const FloatVec3Type origin = {1.0F, -2.0F, 3.0F};
  const CreateImageGeometryAction inputGeometryAction(srcGeomPath, {2, 2, 2}, {1.0F, -2.0F, 3.0F}, {1.0F, 1.0F, 1.0F}, "Cell Data", units);
  auto inputGeometryResult = inputGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(inputGeometryResult);
  const auto* inputGeomPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  REQUIRE((inputGeomPtr != nullptr));
  REQUIRE((inputGeomPtr->getUnits() == units));
  const auto inputGeomId = inputGeomPtr->getId();
  const auto* inputCellDataPtr = inputGeomPtr->getCellData();
  REQUIRE((inputCellDataPtr != nullptr));
  auto* inputValuesPtr = Int32ArrayType::Create(dataStructure, "Values", data_store_utilities::CreateDataStore<int32>(dataStructure, inputDataPath, {2, 2, 2}, {1}), inputCellDataPtr->getId());
  REQUIRE((inputValuesPtr != nullptr));
  inputValuesPtr->fill(7);

  const ResampleImageGeomFilter filter;
  Arguments args;
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
  args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{0.5F, 0.5F, 0.5F}));
  args.insertOrAssign(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(removeOriginal));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const CreateImageGeometryAction* plannedGeometryPtr = nullptr;
  for(const auto& action : preflightResult.output_actions.value().actions)
  {
    if(const auto* candidatePtr = dynamic_cast<const CreateImageGeometryAction*>(action.get()); candidatePtr != nullptr)
    {
      plannedGeometryPtr = candidatePtr;
      break;
    }
  }
  REQUIRE((plannedGeometryPtr != nullptr));
  CHECK((plannedGeometryPtr->units() == units));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputGeomPath = removeOriginal ? srcGeomPath : destGeomPath;
  const auto* outputGeomPtr = dataStructure.getDataAs<ImageGeom>(outputGeomPath);
  REQUIRE((outputGeomPtr != nullptr));
  CHECK((outputGeomPtr->getUnits() == units));
  CHECK((outputGeomPtr->getDimensions() == SizeVec3Type{4, 4, 4}));
  CHECK((outputGeomPtr->getSpacing() == FloatVec3Type{0.5F, 0.5F, 0.5F}));
  CHECK((outputGeomPtr->getOrigin() == origin));
  const auto* outputValuesPtr = dataStructure.getDataAs<Int32ArrayType>(outputGeomPath.createChildPath("Cell Data").createChildPath("Values"));
  REQUIRE((outputValuesPtr != nullptr));
  REQUIRE((outputValuesPtr->getNumberOfTuples() == 64));
  REQUIRE((outputValuesPtr->getNumberOfComponents() == 1));
  for(usize index = 0; index < outputValuesPtr->getSize(); ++index)
  {
    CHECK(((*outputValuesPtr)[index] == 7));
  }

  if(!removeOriginal)
  {
    const auto* retainedGeomPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
    REQUIRE((retainedGeomPtr != nullptr));
    CHECK((retainedGeomPtr->getId() == inputGeomId));
    CHECK((retainedGeomPtr->getUnits() == units));
    CHECK((retainedGeomPtr->getDimensions() == SizeVec3Type{2, 2, 2}));
    CHECK((retainedGeomPtr->getSpacing() == FloatVec3Type{1.0F, 1.0F, 1.0F}));
    CHECK((retainedGeomPtr->getOrigin() == origin));
    const auto* retainedValuesPtr = dataStructure.getDataAs<Int32ArrayType>(inputDataPath);
    REQUIRE((retainedValuesPtr != nullptr));
    REQUIRE((retainedValuesPtr->getNumberOfTuples() == 8));
    REQUIRE((retainedValuesPtr->getNumberOfComponents() == 1));
    for(usize index = 0; index < retainedValuesPtr->getSize(); ++index)
    {
      CHECK(((*retainedValuesPtr)[index] == 7));
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON, renumber OFF preflight", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  REQUIRE(AttributeMatrix::Create(dataStructure, "Other Feature Data", {5}, dataStructure.getId(DataPath({"Image"}))) != nullptr);
  ResampleImageGeomFilter filter;
  auto result = filter.preflight(dataStructure, MakeFeatureResampleArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  REQUIRE(result.output_actions.warnings().size() == 1);
  const auto& unchangedWarning = result.output_actions.warnings().front();
  CHECK(unchangedWarning.code == -4041);
  CHECK(unchangedWarning.message == "These Attribute Matrices are copied to the resampled geometry unchanged:\n"
                                    "Image/Cell Ensemble Data\n"
                                    "Image/Other Feature Data\n"
                                    "If one holds Feature data, its values describe the original geometry. Recompute them. Ensemble arrays retain their stored values. "
                                    "Recompute any Ensemble statistics that depend on the geometry's cells or elements.");
  CHECK(std::none_of(result.output_values.begin(), result.output_values.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  const auto it = std::find_if(result.output_values.begin(), result.output_values.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; });
  REQUIRE(it != result.output_values.end());
  CHECK(it->value.find("Image/Cell Feature Data/NumElements") != std::string::npos);
  CHECK(it->value.find("Image/Cell Feature Data/NeighborList") != std::string::npos);
  CHECK(it->value.find("Image/Cell Feature Data/Names") != std::string::npos);
  CHECK(it->value.find("Ensemble") == std::string::npos);
  CHECK(std::none_of(result.output_values.begin(), result.output_values.end(), [](const auto& v) { return v.name == "Invalidated NeighborLists" || v.name == "Feature Data Not Cleared"; }));
  CHECK(filter.parametersVersion() == 2);
  auto applyResult = result.output_actions.value().applyRegular(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  const auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  REQUIRE(amPtr != nullptr);
  CHECK(amPtr->getShape() == ShapeType{5});
  CHECK(amPtr->getSize() == 0);
  const auto* ensemblePtr = dataStructure.getDataAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(ensemblePtr != nullptr);
  REQUIRE(ensemblePtr->getNumberOfTuples() == 2);
  CHECK((*ensemblePtr)[0] == 999);
  CHECK((*ensemblePtr)[1] == 1);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear OFF preflight", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto result = filter.preflight(dataStructure, MakeFeatureResampleArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  REQUIRE(result.output_actions.warnings().size() == 1);
  const auto& warning = result.output_actions.warnings().front();
  CHECK(warning.code == -4041);
  CHECK(warning.message.find("\nImage/Cell Feature Data\n") != std::string::npos);
  CHECK(std::none_of(result.output_values.begin(), result.output_values.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  CHECK(std::any_of(result.output_values.begin(), result.output_values.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
  CHECK(std::none_of(result.output_values.begin(), result.output_values.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear OFF ignores stored renumber", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  Arguments args = MakeFeatureResampleArgs(false, true);
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "DoesNotExist"})));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  REQUIRE(amPtr != nullptr);
  CHECK(amPtr->getShape() == ShapeType{5});
  CHECK(amPtr->getSize() == 3);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
  REQUIRE(result.output_actions.warnings().size() == 1);
  CHECK(result.output_actions.warnings().front().code == -4041);
  CHECK(std::none_of(result.output_values.begin(), result.output_values.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: StrictFeatureIds preflight", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][StrictFeatureIds]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(imageGeomPtr != nullptr);
  const auto* cellDataPtr = imageGeomPtr->getCellData();
  REQUIRE(cellDataPtr != nullptr);
  auto* otherDataPtr = AttributeMatrix::Create(dataStructure, "Other Cell Data", cellDataPtr->getShape(), imageGeomPtr->getId());
  REQUIRE(otherDataPtr != nullptr);
  auto* misplacedIdsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "MisplacedFeatureIds", otherDataPtr->getShape(), {1}, otherDataPtr->getId());
  REQUIRE(misplacedIdsPtr != nullptr);
  misplacedIdsPtr->fill(1);

  ResampleImageGeomFilter filter;
  Arguments args = MakeFeatureResampleArgs(true, true);
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Other Cell Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    CHECK(result.output_actions.errors().front().code == -55505);
    CHECK(result.output_actions.errors().front().message ==
          "**Cell Feature Ids** DataArray 'Image/Other Cell Data/MisplacedFeatureIds' must be in the Cell Attribute Matrix 'Image/Cell Data' of the selected Image Geometry 'Image'. "
          "Select the DataArray from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Cell Attribute Matrix")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON Feature AM errors", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  // An Attribute Matrix that is not a child of "Image".
  REQUIRE(AttributeMatrix::Create(dataStructure, "Elsewhere", std::vector<usize>{5}) != nullptr);
  ResampleImageGeomFilter filter;

  Arguments args = MakeFeatureResampleArgs(true, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.output_actions);
  REQUIRE(notChild.output_actions.errors().size() == 1);
  CHECK(notChild.output_actions.errors().front().code == -55503);
  CHECK(notChild.output_actions.errors().front().message ==
        "The Attribute Matrix 'Elsewhere' selected for **Feature Attribute Matrix** must be a direct child of the selected geometry 'Image' and must not be its Cell Attribute Matrix 'Image/Cell "
        "Data'. Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.");

  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data"})));
  auto isCellAm = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isCellAm.output_actions);
  REQUIRE(isCellAm.output_actions.errors().size() == 1);
  CHECK(isCellAm.output_actions.errors().front().code == -55503);
  CHECK(isCellAm.output_actions.errors().front().message ==
        "The Attribute Matrix 'Image/Cell Data' selected for **Feature Attribute Matrix** must be a direct child of the selected geometry 'Image' and must not be its Cell Attribute Matrix "
        "'Image/Cell Data'. Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.");

  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Image", "Missing"})));
  auto missing = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.output_actions);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON, renumber OFF execute", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureResampleArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // The cell FeatureIds keep their original values (1 and 3 for source x = 0 and 2).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 3, 1, 3, 1, 3, 1, 3};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  CHECK(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})).getSize() == 3);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON, renumber ON execute", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  auto preflight = filter.preflight(dataStructure, MakeFeatureResampleArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto info = std::find_if(preflight.output_values.begin(), preflight.output_values.end(), [](const auto& value) { return value.name == "Cleared Feature Data"; });
  REQUIRE(info != preflight.output_values.end());
  CHECK(info->value.find("Recompute any of the following Data Arrays:\n") != std::string::npos);
  CHECK(info->value.find("\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after the resample.") != std::string::npos);
  auto result = filter.execute(dataStructure, MakeFeatureResampleArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  // Features 1 and 3 remain, plus row 0.
  CHECK(am.getShape() == std::vector<usize>{3});
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 2, 1, 2, 1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(dataStructure, MakeFeatureResampleArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Feature Data", "NumElements"})));
  const auto& numElements = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 4, 4, 4, 4};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Resampled", "Cell Feature Data", "NeighborList"})));
  const auto& neighbors = dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Resampled", "Cell Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(DataPath({"Resampled", "Cell Feature Data", "Names"})));
  const auto& names = dataStructure.getDataRefAs<StringArray>(DataPath({"Resampled", "Cell Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 3, 1, 3, 1, 3, 1, 3};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Resampled", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Clear ON in place", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  ResampleImageGeomFilter filter;
  Arguments args = MakeFeatureResampleArgs(true, true);
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getShape() == std::vector<usize>{3});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image"})));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image"}));
  CHECK(imageGeom.getDimensions() == SizeVec3Type(2, 4, 1));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 2, 1, 2, 1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  CHECK(dataStructure.getData(DataPath({"Resampled"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({".Image"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({".resampled_image_geometry"})) == nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32ArrayType>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Shared utility defaults preserve Feature data", "[SimplnxCore][ResampleImageGeom][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  // Image-stack readers leave both new fields at their defaults.
  ResampleImageGeomInputValues inputValues{};
  CHECK_FALSE(inputValues.clear_feature_attribute_matrix);
  CHECK_FALSE(inputValues.report_uncleared_feature_data);
  inputValues.selected_image_geometry_path = DataPath({"Image"});
  inputValues.created_image_geometry_path = DataPath({"Resampled"});
  inputValues.spacing = {2.0F, 1.0F, 1.0F};
  auto result = PreflightImageGeometryResample(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  REQUIRE(result.output_actions.warnings().size() == 1);
  CHECK(result.output_actions.warnings().front().code == -4041);
  CHECK(std::none_of(result.output_values.begin(), result.output_values.end(),
                     [](const auto& value) { return value.name == "Feature Data Not Cleared" || value.name == "Cleared Feature Data" || value.name == "Invalidated NeighborLists"; }));
  auto applyResult = result.output_actions.value().applyRegular(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  const auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  REQUIRE(amPtr != nullptr);
  CHECK(amPtr->getShape() == ShapeType{5});
  CHECK(amPtr->getSize() == 3);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Invalid Parameters", "[SimplnxCore][ResampleImageGeom]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(constants::k_FeatureIds);
  DataPath destGeomPath({constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  int errCode = 0;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11502;
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
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
  auto preflightErrors = preflightResult.output_actions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == errCode);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 3D In Place", "[SimplnxCore][ResampleImageGeom]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(constants::k_FeatureIds);
  DataPath destGeomPath({constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>(std::vector<uint64>{47, 50, 1}));
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  auto* const srcGeoPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  auto* const destGeoPtr = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  auto* const destCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  auto* const destFeatureDataPtr = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  auto* const destNewGrainDataPtr = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  auto* const destPhaseDataPtr = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  REQUIRE(srcGeoPtr != nullptr);
  REQUIRE(destGeoPtr != nullptr);
  REQUIRE(destCellDataPtr != nullptr);
  REQUIRE(destFeatureDataPtr != nullptr);
  REQUIRE(destNewGrainDataPtr != nullptr);
  REQUIRE(destPhaseDataPtr != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeoPtr->getSpacing();
  const auto destDims = destGeoPtr->getDimensions();
  const auto destCellDataDims = destCellDataPtr->getShape();
  const auto destNewGrainDataDims = destNewGrainDataPtr->getShape();
  const auto destPhaseDataDims = destPhaseDataPtr->getShape();
  // Get the exemplar data
  auto* const exemplarGeoPtr = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  auto* const exemplarCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  auto* const exemplarFeatureDataPtr = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto exemplarSpacing = exemplarGeoPtr->getSpacing();
  const auto exemplarDims = exemplarGeoPtr->getDimensions();
  const auto exemplarCellDataDims = exemplarCellDataPtr->getShape();
  REQUIRE(exemplarGeoPtr->getOrigin() == destGeoPtr->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < unit_test::k_Epsilon);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(exemplarFeatureDataPtr != nullptr);
  REQUIRE(destFeatureDataPtr->getNumberOfTuples() == exemplarFeatureDataPtr->getNumberOfTuples());
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellDataPtr->getSize() == exemplarCellDataPtr->getSize());
  REQUIRE(destFeatureDataPtr->getSize() == 0);
  REQUIRE(destNewGrainDataPtr->getSize() == 1);
  REQUIRE(destPhaseDataPtr->getSize() == 2);

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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 3D Save Geometry", "[SimplnxCore][ResampleImageGeom]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(constants::k_FeatureIds);
  DataPath destGeomPath({constants::k_SmallIN100.str() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>(std::vector<uint64>{47, 50, 1}));
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  auto* const srcGeoPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  auto* const destGeoPtr = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  auto* const destCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  auto* const destFeatureDataPtr = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  auto* const destNewGrainDataPtr = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  auto* const destPhaseDataPtr = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  auto* const exemplarGeoPtr = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  auto* const exemplarCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  auto* const exemplarFeatureDataPtr = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  REQUIRE(srcGeoPtr != nullptr);
  REQUIRE(destGeoPtr != nullptr);
  REQUIRE(destCellDataPtr != nullptr);
  REQUIRE(destFeatureDataPtr != nullptr);
  REQUIRE(destNewGrainDataPtr != nullptr);
  REQUIRE(destPhaseDataPtr != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeoPtr->getSpacing();
  const auto destDims = destGeoPtr->getDimensions();
  const auto destCellDataDims = destCellDataPtr->getShape();
  const auto destNewGrainDataDims = destNewGrainDataPtr->getShape();
  const auto destPhaseDataDims = destPhaseDataPtr->getShape();

  const auto exemplarSpacing = exemplarGeoPtr->getSpacing();
  const auto exemplarDims = exemplarGeoPtr->getDimensions();
  const auto exemplarCellDataDims = exemplarCellDataPtr->getShape();
  REQUIRE(exemplarGeoPtr->getOrigin() == destGeoPtr->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < unit_test::k_Epsilon);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(exemplarFeatureDataPtr != nullptr);
  REQUIRE(destFeatureDataPtr->getNumberOfTuples() == exemplarFeatureDataPtr->getNumberOfTuples());
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellDataPtr->getSize() == exemplarCellDataPtr->getSize());
  REQUIRE(destFeatureDataPtr->getSize() == 0);
  REQUIRE(destNewGrainDataPtr->getSize() == 1);
  REQUIRE(destPhaseDataPtr->getSize() == 2);

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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 2D In Place", "[SimplnxCore][ResampleImageGeom]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

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
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>(std::vector<uint64>{15, 15, 1}));
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());

  // check that the geometry(s) and the attribute matrices are all there
  auto* const srcGeoPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  auto* const destGeoPtr = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  auto* const destCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  auto* const exemplarGeoPtr = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  auto* const exemplarCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeoPtr != nullptr);
  REQUIRE(destGeoPtr != nullptr);
  REQUIRE(destCellDataPtr != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeoPtr->getSpacing();
  const auto destDims = destGeoPtr->getDimensions();
  const auto destCellDataDims = destCellDataPtr->getShape();

  const auto exemplarSpacing = exemplarGeoPtr->getSpacing();
  const auto exemplarDims = exemplarGeoPtr->getDimensions();
  const auto exemplarCellDataDims = exemplarCellDataPtr->getShape();
  REQUIRE(exemplarGeoPtr->getOrigin() == destGeoPtr->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < unit_test::k_Epsilon);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellDataPtr->getSize() == exemplarCellDataPtr->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 2D Save Geometry", "[SimplnxCore][ResampleImageGeom]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

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
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>(std::vector<uint64>{15, 15, 1}));
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // check that the geometry(s) and the attribute matrices are all there
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());
  auto* const srcGeoPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  auto* const destGeoPtr = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  auto* const destCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  auto* const exemplarGeoPtr = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  auto* const exemplarCellDataPtr = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeoPtr != nullptr);
  REQUIRE(destGeoPtr != nullptr);
  REQUIRE(destCellDataPtr != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeoPtr->getSpacing();
  const auto destDims = destGeoPtr->getDimensions();
  const auto destCellDataDims = destCellDataPtr->getShape();

  const auto exemplarSpacing = exemplarGeoPtr->getSpacing();
  const auto exemplarDims = exemplarGeoPtr->getDimensions();
  const auto exemplarCellDataDims = exemplarCellDataPtr->getShape();
  REQUIRE(exemplarGeoPtr->getOrigin() == destGeoPtr->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < unit_test::k_Epsilon);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < unit_test::k_Epsilon);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellDataPtr->getSize() == exemplarCellDataPtr->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ResampleImageGeomFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ResampleImageGeomFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ResampleImageGeomFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ResampleImageGeomFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key) == true);
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_RenumberFeatures_Key) == true);
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key) == false);
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key) == DataPath({"DataContainer", "CellFeatureData"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CreatedImageGeometry_Key) == DataPath({"DataContainer"}));
    }
  }
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Child names containing the geometry name are kept", "[SimplnxCore][ResampleImageGeom]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  const DataPath destGeomPath({"Geom Out"});
  auto* geomPtr = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  REQUIRE(geomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setCellData(*elementAmPtr);
  auto* dataPtr =
      Int32ArrayType::Create(dataStructure, "Data", data_store_utilities::CreateDataStore<int32>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                         elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Geom Values",
                                       data_store_utilities::CreateDataStore<int32>(dataStructure, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.output_actions.value().actions)
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
    REQUIRE(dataStructure.getDataAs<Int32ArrayType>(copiedValuesPath) != nullptr);
    REQUIRE_FALSE(dataStructure.containsData(renamedAmPath));
    REQUIRE(dataStructure.getDataAs<Int32ArrayType>(destGeomPath.createChildPath("Cell Data").createChildPath("Data")) != nullptr);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Feature AM nested alias regression", "[SimplnxCore][ResampleImageGeomFilter][NestedFeatureAlias]")
{
  unit_test::LoadPlugins();
  const bool clear = GENERATE(true, false);
  const bool renumber = GENERATE(false, true);
  auto ds = CreateFeatureResampleFixture();
  const DataPath sourcePath({"Image"});
  const DataPath outputPath({"Resampled"});
  const auto featurePath = sourcePath.createChildPath("Cell Feature Data");
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  const auto featureId = featuresPtr->getId();
  const auto shape = featuresPtr->getShape();
  const auto countsPath = featurePath.createChildPath("NumElements");
  const auto& counts = ds.getDataRefAs<Int32ArrayType>(countsPath);
  std::vector<int32> originalCounts(counts.getSize());
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    originalCounts[i] = counts[i];
  }
  auto* nestedPtr = DataGroup::Create(ds, "Misc", ds.getData(sourcePath)->getId());
  REQUIRE(nestedPtr != nullptr);
  auto* keepPtr = unit_test::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nestedPtr->getId());
  REQUIRE(keepPtr != nullptr);
  (*keepPtr)[0] = 123;
  REQUIRE(ds.setAdditionalParent(featureId, nestedPtr->getId()));
  auto* outsidePtr = DataGroup::Create(ds, "Outside");
  REQUIRE(outsidePtr != nullptr);
  REQUIRE(ds.setAdditionalParent(nestedPtr->getId(), outsidePtr->getId()));
  const auto parents = ds.getData(featurePath)->getParentIds();
  auto args = MakeFeatureResampleArgs(clear, renumber);
  ResampleImageGeomFilter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  bool foundNestedCopy = false;
  const auto nestedOutputPath = outputPath.createChildPath("Misc").createChildPath("Cell Feature Data");
  for(const auto& action : preflight.output_actions.value().actions)
  {
    const auto* copyPtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
    if(copyPtr != nullptr && copyPtr->path() == sourcePath.createChildPath("Misc"))
    {
      foundNestedCopy = true;
      bool declaresFeature = false;
      for(const auto& created : copyPtr->getAllCreatedPaths())
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
  const auto* outputFeaturesPtr = ds.getDataAs<AttributeMatrix>(outputPath.createChildPath("Cell Feature Data"));
  REQUIRE(outputFeaturesPtr != nullptr);
  CHECK(outputFeaturesPtr->getSize() == (clear ? 0 : 3));
  CHECK((ds.getData(nestedOutputPath) == nullptr) == clear);
  CHECK(ds.getDataRefAs<Int32ArrayType>(outputPath.createChildPath("Misc").createChildPath("Keep"))[0] == 123);
  CHECK(ds.getData(featurePath)->getId() == featureId);
  CHECK(ds.getData(featurePath)->getParentIds() == parents);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getShape() == shape);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getSize() == 3);
  CHECK(ds.getData(DataPath({"Outside", "Misc", "Cell Feature Data"}))->getId() == featureId);
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    CHECK(ds.getDataRefAs<Int32ArrayType>(countsPath)[i] == originalCounts[i]);
    if(!clear)
    {
      CHECK(ds.getDataRefAs<Int32ArrayType>(nestedOutputPath.createChildPath("NumElements"))[i] == originalCounts[i]);
    }
  }
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Feature AM selected through second parent", "[SimplnxCore][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateFeatureResampleFixture();
  const bool renumber = GENERATE(false, true);
  auto* parentPtr = DataGroup::Create(dataStructure, "Alias");
  REQUIRE(parentPtr != nullptr);
  const auto& source = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  REQUIRE(dataStructure.setAdditionalParent(source.getId(), parentPtr->getId()));
  auto args = MakeFeatureResampleArgs(true, renumber);
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath({"Alias", "Cell Feature Data"})));
  ResampleImageGeomFilter filter;
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto& output = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Resampled", "Cell Feature Data"}));
  CHECK(output.getSize() == 0);
  CHECK(output.getShape() == ShapeType{renumber ? usize{3} : usize{5}});
  const auto& ids = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Resampled", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  for(usize i = 0; i < 8; ++i)
  {
    CHECK(ids[i] == (renumber ? (i % 2 == 0 ? 1 : 2) : (i % 2 == 0 ? 1 : 3)));
  }
  CHECK(source.getSize() == 3);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: SIMPL conversion Feature options", "[SimplnxCore][ResampleImageGeomFilter][SIMPL]")
{
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test/simpl_conversion/6_5/ResampleImageGeomFilter.json";
  std::ifstream stream(fixturePath);
  REQUIRE(stream.is_open());
  auto json = nlohmann::json::parse(stream).at("0");
  json["CellFeatureAttributeMatrixPath"] = {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Feature Data"}, {"Data Array Name", ""}};

  SECTION("Present Feature paths are converted for either RenumberFeatures value")
  {
    const bool renumber = GENERATE(true, false);
    json["RenumberFeatures"] = renumber ? 1 : 0;
    auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
    SIMPLNX_RESULT_REQUIRE_VALID(converted);
    const auto& args = converted.value();
    CHECK(args.value<bool>(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key) == renumber);
    CHECK(args.value<bool>(ResampleImageGeomFilter::k_RenumberFeatures_Key) == renumber);
    CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key) == DataPath({"Image", "Cell Feature Data"}));
    CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  }

  SECTION("Missing Feature paths keep their defaults when renumbering is off")
  {
    const auto [omitFeatureMatrix, omitFeatureIds] = GENERATE(table<bool, bool>({{true, true}, {true, false}, {false, true}}));
    CAPTURE(omitFeatureMatrix, omitFeatureIds);
    json["RenumberFeatures"] = 0;
    if(omitFeatureMatrix)
    {
      json.erase("CellFeatureAttributeMatrixPath");
    }
    if(omitFeatureIds)
    {
      json.erase("FeatureIdsArrayPath");
    }
    auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
    SIMPLNX_RESULT_REQUIRE_VALID(converted);
    const auto& args = converted.value();
    CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    CHECK(args.value<VectorFloat32ParameterType::ValueType>(ResampleImageGeomFilter::k_Spacing_Key) == std::vector<float32>{1.0F, 2.0F, 3.0F});
    CHECK_FALSE(args.value<bool>(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key));
    CHECK_FALSE(args.value<bool>(ResampleImageGeomFilter::k_RenumberFeatures_Key));
    const auto defaults = ResampleImageGeomFilter().getDefaultArguments();
    const auto defaultFeatureMatrixPath = defaults.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key);
    const auto defaultFeatureIdsPath = defaults.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key);
    CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key) == (omitFeatureMatrix ? defaultFeatureMatrixPath : DataPath({"Image", "Cell Feature Data"})));
    CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key) == (omitFeatureIds ? defaultFeatureIdsPath : DataPath({"DataContainer", "CellData", "TestArray"})));
  }

  SECTION("Missing Feature paths report a conversion error when renumbering is on")
  {
    const auto& missingKey = GENERATE(as<std::string>{}, "CellFeatureAttributeMatrixPath", "FeatureIdsArrayPath");
    CAPTURE(missingKey);
    json["RenumberFeatures"] = 1;
    json.erase(missingKey);
    auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
    SIMPLNX_RESULT_REQUIRE_INVALID(converted);
    REQUIRE(converted.errors().size() == 1);
    CHECK(converted.errors().front().code == -571);
    CHECK(converted.errors().front().message.starts_with(fmt::format("Key `{}` not found", missingKey)));
  }

  SECTION("Missing Feature paths report a conversion error when RenumberFeatures is missing or invalid")
  {
    const bool omitRenumber = GENERATE(true, false);
    CAPTURE(omitRenumber);
    if(omitRenumber)
    {
      json.erase("RenumberFeatures");
    }
    else
    {
      json["RenumberFeatures"] = "0";
    }
    json.erase("FeatureIdsArrayPath");
    auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
    SIMPLNX_RESULT_REQUIRE_INVALID(converted);
    const auto& errors = converted.errors();
    CHECK(std::any_of(errors.cbegin(), errors.cend(), [](const Error& error) { return error.code == -571 && error.message.starts_with("Key `FeatureIdsArrayPath` not found"); }));
  }

  SECTION("Malformed Feature paths report a conversion error when renumbering is off")
  {
    const auto& invalidKey = GENERATE(as<std::string>{}, "CellFeatureAttributeMatrixPath", "FeatureIdsArrayPath");
    CAPTURE(invalidKey);
    json["RenumberFeatures"] = 0;
    json[invalidKey]["Data Container Name"] = 7;
    auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
    SIMPLNX_RESULT_REQUIRE_INVALID(converted);
    REQUIRE(converted.errors().size() == 1);
    CHECK(converted.errors().front().code == -2);
  }
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: SIMPL converted preflight", "[SimplnxCore][ResampleImageGeomFilter][SIMPL]")
{
  unit_test::LoadPlugins();
  auto dataStructure = CreateFeatureResampleFixture();
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test/simpl_conversion/6_5/ResampleImageGeomFilter.json";
  std::ifstream stream(fixturePath);
  REQUIRE(stream.is_open());
  auto json = nlohmann::json::parse(stream).at("0");
  json["CellAttributeMatrixPath"] = {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Data"}, {"Data Array Name", ""}};
  json["CellFeatureAttributeMatrixPath"] = {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Feature Data"}, {"Data Array Name", ""}};
  json["FeatureIdsArrayPath"] = {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Data"}, {"Data Array Name", "FeatureIds"}};
  json["NewDataContainerPath"] = {{"Data Container Name", "Resampled"}};
  json["RenumberFeatures"] = 1;
  auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(converted);
  auto preflight = ResampleImageGeomFilter().preflight(dataStructure, converted.value());
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto cleared = std::find_if(preflight.output_values.begin(), preflight.output_values.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; });
  REQUIRE(cleared != preflight.output_values.end());
  CHECK(cleared->value.find("Image/Cell Feature Data/NumElements") != std::string::npos);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: SIMPL 6.5 ChangeResolution keys", "[SimplnxCore][ResampleImageGeomFilter][SIMPL]")
{
  // Keys and value shapes written by DREAM.3D 6.5.171 ChangeResolution: "Resolution" and a string "NewDataContainerName".
  // Flags are unsigned because that is how nlohmann parses the 0 and 1 in a legacy pipeline file.
  const bool renumber = GENERATE(true, false);
  const bool saveAsNew = GENERATE(true, false);
  nlohmann::json json = {{"Filter_Name", "ChangeResolution"},
                         {"Filter_Uuid", "{1966e540-759c-5798-ae26-0c6a3abc65c0}"},
                         {"Resolution", {{"x", 2.0}, {"y", 3.0}, {"z", 4.0}}},
                         {"RenumberFeatures", renumber ? 1U : 0U},
                         {"SaveAsNewDataContainer", saveAsNew ? 1U : 0U},
                         {"NewDataContainerName", "Resampled"},
                         {"CellAttributeMatrixPath", {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Data"}, {"Data Array Name", ""}}},
                         {"CellFeatureAttributeMatrixPath", {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Feature Data"}, {"Data Array Name", ""}}},
                         {"FeatureIdsArrayPath", {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Data"}, {"Data Array Name", "FeatureIds"}}}};
  auto converted = ResampleImageGeomFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(converted);
  const Arguments& args = converted.value();
  CHECK(args.value<bool>(ResampleImageGeomFilter::k_ClearFeatureAttributeMatrix_Key) == renumber);
  CHECK(args.value<bool>(ResampleImageGeomFilter::k_RenumberFeatures_Key) == renumber);
  CHECK(args.value<bool>(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key) == !saveAsNew);
  CHECK(args.value<VectorFloat32ParameterType::ValueType>(ResampleImageGeomFilter::k_Spacing_Key) == std::vector<float32>{2.0F, 3.0F, 4.0F});
  CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key) == DataPath({"Image"}));
  CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key) == DataPath({"Image", "Cell Feature Data"}));
  CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"Image", "Cell Data", "FeatureIds"}));
  CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CreatedImageGeometry_Key) == DataPath({"Resampled"}));

  unit_test::LoadPlugins();
  auto dataStructure = CreateFeatureResampleFixture();
  auto preflight = ResampleImageGeomFilter().preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Reject another geometry element Feature AM", "[SimplnxCore][ResampleImageGeomFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  auto ds = CreateFeatureResampleFixture();
  const auto featurePath = DataPath({"Image", "Cell Feature Data"});
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(otherPtr != nullptr);
  auto* verticesPtr = unit_test::CreateTestDataArray<float32>(ds, "Vertices", {featuresPtr->getNumberOfTuples()}, {3}, otherPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  otherPtr->setVertices(*verticesPtr);
  REQUIRE(ds.setAdditionalParent(featuresPtr->getId(), otherPtr->getId()));
  otherPtr->setVertexAttributeMatrix(*featuresPtr);
  ResampleImageGeomFilter filter;
  auto result = filter.preflight(ds, MakeFeatureResampleArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors().size() == 1);
  CHECK(result.output_actions.errors()[0].code == -67460);
  CHECK(result.output_actions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.output_actions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(otherPtr->getVertexAttributeMatrix() == featuresPtr);
  CHECK(featuresPtr->getSize() > 0);
}
