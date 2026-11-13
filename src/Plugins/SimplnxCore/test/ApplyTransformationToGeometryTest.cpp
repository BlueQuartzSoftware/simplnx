#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
void executeIdentityTransform(const ApplyTransformationToGeometryFilter& filter, DataStructure& dataStructure, const Arguments& args)
{
  auto result = filter.execute(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) -- The project assertion macro uses a do-while wrapper.
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

void checkTransformDescendantPaths(UnitTest::AlgorithmTestScope* algorithmTestScopePtr)
{
  DataStructure dataStructure;
  REQUIRE((DataGroup::Create(dataStructure, "Grain Group") != nullptr));
  auto* decoyPtr = DataGroup::Create(dataStructure, "Grain");
  REQUIRE((decoyPtr != nullptr));
  auto* decoyChildPtr = DataGroup::Create(dataStructure, "Keep Me", decoyPtr->getId());
  REQUIRE((decoyChildPtr != nullptr));
  const auto decoyId = decoyPtr->getId();
  const auto decoyChildId = decoyChildPtr->getId();
  const DataPath geometryPath({"Grain Group", "Grain"});
  const DataPath cellPath({"Grain Group", "Grain", "Grain Cells", "Grain Pixels"});
  const DataPath valuesPath({"Grain Group", "Grain", "Grain Data", "Grain Details", "Grain Metrics", "Grain Values"});
  const DataPath directPath({"Grain Group", "Grain", "Grain Transform"});
  const auto createGeometryResult = CreateImageGeometryAction(geometryPath, {2, 2, 2}, {0, 0, 0}, {1, 1, 1}, "Grain Cells").apply(dataStructure, IDataAction::Mode::Execute);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) -- The project assertion macro uses a do-while wrapper.
  SIMPLNX_RESULT_REQUIRE_VALID(createGeometryResult);
  const auto& geometry = UnitTest::GetRequiredData<ImageGeom>(dataStructure, geometryPath);
  const auto& cells = UnitTest::GetRequiredData<AttributeMatrix>(dataStructure, DataPath({"Grain Group", "Grain", "Grain Cells"}));
  auto pixelStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, cellPath, {2, 2, 2}, {1});
  REQUIRE((pixelStore != nullptr));
  auto* pixelsPtr = Int32Array::Create(dataStructure, "Grain Pixels", pixelStore, cells.getId());
  REQUIRE((pixelsPtr != nullptr));
  constexpr std::array<int32, 8> expectedPixels = {2, 5, 11, 17, 23, 31, 41, 53};
  for(usize index = 0; index < expectedPixels.size(); ++index)
  {
    (*pixelsPtr)[index] = expectedPixels.at(index);
  }
  auto* dataPtr = DataGroup::Create(dataStructure, "Grain Data", geometry.getId());
  REQUIRE((dataPtr != nullptr));
  auto* detailsPtr = DataGroup::Create(dataStructure, "Grain Details", dataPtr->getId());
  REQUIRE((detailsPtr != nullptr));
  auto* metricsPtr = AttributeMatrix::Create(dataStructure, "Grain Metrics", {2}, detailsPtr->getId());
  REQUIRE((metricsPtr != nullptr));
  auto valuesStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, valuesPath, {2}, {1});
  REQUIRE((valuesStore != nullptr));
  auto* valuesPtr = Float32Array::Create(dataStructure, "Grain Values", valuesStore, metricsPtr->getId());
  REQUIRE((valuesPtr != nullptr));
  (*valuesPtr)[0] = 7.5F;
  (*valuesPtr)[1] = -3.25F;
  auto directStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, directPath, {2}, {1});
  REQUIRE((directStore != nullptr));
  auto* directPtr = Float32Array::Create(dataStructure, "Grain Transform", directStore, geometry.getId());
  REQUIRE((directPtr != nullptr));
  (*directPtr)[0] = 19.0F;
  (*directPtr)[1] = -29.0F;
  if(algorithmTestScopePtr == nullptr)
  {
    UnitTest::RequireDiskBackedStore(*pixelsPtr);
    UnitTest::RequireDiskBackedStore(*valuesPtr);
    UnitTest::RequireDiskBackedStore(*directPtr);
  }

  const ApplyTransformationToGeometryFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Grain Group", "Grain", "Grain Cells"})));
  // The manual identity path creates a replacement geometry and copies non-cell children.
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key,
                      std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  const auto preflight = filter.preflight(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) -- The project assertion macro uses a do-while wrapper.
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

  // Check declarations separately: deepCopy preserves real descendants even if their declared paths are wrong.
  const std::vector<DataPath> expectedGroupPaths = {DataPath({"Grain Group", ".Grain", "Grain Data"}), DataPath({"Grain Group", ".Grain", "Grain Data", "Grain Details"}),
                                                    DataPath({"Grain Group", ".Grain", "Grain Data", "Grain Details", "Grain Metrics"}),
                                                    DataPath({"Grain Group", ".Grain", "Grain Data", "Grain Details", "Grain Metrics", "Grain Values"})};
  const std::vector<DataPath> expectedDirectPaths = {DataPath({"Grain Group", ".Grain", "Grain Transform"})};
  usize copyActionCount = 0;
  for(const auto& action : preflight.outputActions.value().actions)
  {
    const auto* copyActionPtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
    if(copyActionPtr == nullptr)
    {
      continue;
    }
    ++copyActionCount;
    const bool copiesGroup = copyActionPtr->path() == DataPath({"Grain Group", "Grain", "Grain Data"});
    CHECK((copiesGroup || copyActionPtr->path() == directPath));
    const auto& expectedPaths = copiesGroup ? expectedGroupPaths : expectedDirectPaths;
    CHECK((copyActionPtr->newPath() == expectedPaths.front()));
    const auto declaredPaths = copyActionPtr->getAllCreatedPaths();
    CHECK((declaredPaths.size() == expectedPaths.size()));
    for(const auto& expectedPath : expectedPaths)
    {
      CAPTURE(expectedPath.toString());
      CHECK((std::ranges::find(declaredPaths, expectedPath) != declaredPaths.end()));
    }
  }
  CHECK((copyActionCount == 2));
  const auto before = GetAlgorithmPathExecutionCounts();
  if(algorithmTestScopePtr != nullptr)
  {
    algorithmTestScopePtr->execute([&] { executeIdentityTransform(filter, dataStructure, args); });
  }
  else
  {
    executeIdentityTransform(filter, dataStructure, args);
  }
  if(algorithmTestScopePtr == nullptr)
  {
    const auto after = GetAlgorithmPathExecutionCounts();
    CHECK((after.OutOfCoreOnOutOfCoreStore > before.OutOfCoreOnOutOfCoreStore));
    CHECK((after.InCore == before.InCore));
    CHECK((after.OutOfCoreOnInMemoryStore == before.OutOfCoreOnInMemoryStore));
  }
  const auto& outputGeometry = UnitTest::GetRequiredData<ImageGeom>(dataStructure, geometryPath);
  CHECK((outputGeometry.getDimensions() == SizeVec3{2, 2, 2}));
  CHECK((outputGeometry.getOrigin() == FloatVec3{0, 0, 0}));
  CHECK((outputGeometry.getSpacing() == FloatVec3{1, 1, 1}));
  const auto& outputPixels = UnitTest::GetRequiredData<Int32Array>(dataStructure, cellPath);
  const auto& outputValues = UnitTest::GetRequiredData<Float32Array>(dataStructure, valuesPath);
  const auto& outputDirect = UnitTest::GetRequiredData<Float32Array>(dataStructure, directPath);
  REQUIRE((outputPixels.getTupleShape() == ShapeType{2, 2, 2}));
  REQUIRE((outputPixels.getNumberOfComponents() == 1));
  REQUIRE((outputValues.getNumberOfTuples() == 2));
  REQUIRE((outputValues.getNumberOfComponents() == 1));
  REQUIRE((outputDirect.getNumberOfTuples() == 2));
  REQUIRE((outputDirect.getNumberOfComponents() == 1));
  for(usize index = 0; index < expectedPixels.size(); ++index)
  {
    CHECK((outputPixels[index] == expectedPixels.at(index)));
  }
  CHECK((outputValues[0] == 7.5F));
  CHECK((outputValues[1] == -3.25F));
  CHECK((outputDirect[0] == 19.0F));
  CHECK((outputDirect[1] == -29.0F));
  if(algorithmTestScopePtr == nullptr)
  {
    UnitTest::RequireDiskBackedStore(outputPixels);
    UnitTest::RequireDiskBackedStore(outputValues);
    UnitTest::RequireDiskBackedStore(outputDirect);
  }
  CHECK((dataStructure.getTopLevelData().size() == 2));
  CHECK((UnitTest::GetRequiredData<DataGroup>(dataStructure, DataPath({"Grain"})).getId() == decoyId));
  CHECK((UnitTest::GetRequiredData<DataGroup>(dataStructure, DataPath({"Grain", "Keep Me"})).getId() == decoyChildId));
  CHECK_FALSE(dataStructure.containsData(DataPath({"Grain Group", ".Grain"})));
  CHECK_FALSE(dataStructure.containsData(DataPath({".Grain Group"})));
  CHECK_FALSE(dataStructure.containsData(DataPath({".Grain"})));
  CHECK_FALSE(dataStructure.containsData(DataPath({"Grain Group", "Grain", ".Grain Data"})));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

} // namespace

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: nested geometry preserves descendant names", "[SimplnxCore][ApplyTransformationToGeometryFilter][NestedPaths]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  checkTransformDescendantPaths(&algorithmTestScope);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: nested geometry preserves descendant names with real OOC stores",
          "[SimplnxCore][ApplyTransformationToGeometryFilter][NestedPaths][.OocStoreContract]")
{
  REQUIRE(DataStoreUtilities::GetIOCollection().hasDataStoreCreationFunction("HDF5-OOC"));
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  REQUIRE_FALSE(ForceInCoreAlgorithm());
  REQUIRE_FALSE(ForceOocAlgorithm());
  checkTransformDescendantPaths(nullptr);
}

namespace apply_transformation_to_geometry
{
const nx::core::ChoicesParameter::ValueType k_PrecomputedTransformationMatrixIdx = 1ULL;
const nx::core::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;
const nx::core::ChoicesParameter::ValueType k_RotationIdx = 3ULL;
const nx::core::ChoicesParameter::ValueType k_TranslationIdx = 4ULL;
const nx::core::ChoicesParameter::ValueType k_ScaleIdx = 5ULL;

const nx::core::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;

const std::string k_InputGeometryName("InputData");
const std::string k_InputNodeGeometryName("InputNodeData");
const DataPath k_InputCellAttrMatrixPath(DataPath({k_InputGeometryName, "VertexData"}));
const std::string k_Rotation45XGeometryName("Rotation45X");
const std::string k_Rotation45YGeometryName("Rotation45Y");
const std::string k_Rotation45ZGeometryName("Rotation45Z");
const std::string k_Rotation90XGeometryName("Rotation90X");
const std::string k_Rotation90YGeometryName("Rotation90Y");
const std::string k_Rotation90ZGeometryName("Rotation90Z");
const std::string k_ScaleGeometryName("Scale");
const std::string k_ScaleNodeGeometryName("Scale_Node");
const std::string k_TranslationGeometryName("Translation");
const std::string k_TranslationNodeGeometryName("Translation_Node");
const std::string k_ManualGeometryName("Manual");
const std::string k_ManualNodeGeometryName("Manual_Node");
const std::string k_PrecomputedGeometryName("Precomputed");
const std::string k_PrecomputedNodeGeometryName("Precomputed_Node");
const std::string k_Rotation45XNodeGeometryName("Rotation_45X_Node");
const std::string k_Rotation45YNodeGeometryName("Rotation_45Y_Node");
const std::string k_Rotation45ZNodeGeometryName("Rotation_45Z_Node");
const std::string k_Rotation90XNodeGeometryName("Rotation_90X_Node");
const std::string k_Rotation90YNodeGeometryName("Rotation_90Y_Node");
const std::string k_Rotation90ZNodeGeometryName("Rotation_90Z_Node");
const std::string k_Rotation45XGlobalGeometryName("Rotation45X_Global");
const std::string k_Rotation45YGlobalGeometryName("Rotation45Y_Global");
const std::string k_Rotation45ZGlobalGeometryName("Rotation45Z_Global");
const std::string k_Rotation90XGlobalGeometryName("Rotation90X_Global");
const std::string k_Rotation90YGlobalGeometryName("Rotation90Y_Global");
const std::string k_Rotation90ZGlobalGeometryName("Rotation90Z_Global");
const std::string k_ScaleGlobalGeometryName("Scale_Global");
const std::string k_TranslationGlobalGeometryName("Translation_Global");
const std::string k_ManualGlobalGeometryName("Manual_Global");
const std::string k_PrecomputedGlobalGeometryName("Precomputed_Global");
const DataPath k_PrecomputedTransformationMatrixPath({"Transformation Matrices", "Precomputed"});
const std::string k_ExemplaryNNDataName("Data_NN");
const std::string k_ExemplaryLinearDataName("Data_L");

const std::string k_SharedVertexListName("SharedVertexList");

const int32 k_CellAttrMatrixUnusedWarning = -5555;

void CompareImageGeometries(const DataStructure& dataStructure, const ImageGeom& exemplaryGeom, const ImageGeom& calculatedGeom, const std::string& exemplaryDataName)
{
  UnitTest::CompareImageGeometry(&exemplaryGeom, &calculatedGeom);

  REQUIRE_NOTHROW(exemplaryGeom.getCellDataRef());
  REQUIRE_NOTHROW(calculatedGeom.getCellDataRef());
  auto exemplaryAM = exemplaryGeom.getCellDataRef();
  auto calculatedAM = calculatedGeom.getCellDataRef();
  REQUIRE(exemplaryAM.getShape() == calculatedAM.getShape());

  const DataPath exemplarPath({exemplaryGeom.getName(), k_Cell_Data, exemplaryDataName});
  const DataPath calculatedPath({calculatedGeom.getName(), k_Cell_Data, "Data"});
  const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
  const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
  UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
}

} // namespace apply_transformation_to_geometry

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry matrix contract", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  const DataPath geometryPath({"Geometry"});
  const DataPath vertexPath({"Geometry", "Vertices"});
  const DataPath matrixPath({"Matrix"});
  const ApplyTransformationToGeometryFilter filter;

  const auto createMatrix = [&](DataStructure& dataStructure, usize tuples, usize components) {
    auto store = DataStoreUtilities::CreateDataStore<float32>(dataStructure, matrixPath, {tuples}, {components});
    auto* matrixPtr = Float32Array::Create(dataStructure, matrixPath.getTargetName(), std::move(store));
    REQUIRE(matrixPtr != nullptr);
    return matrixPtr;
  };
  const auto createTriangle = [&](DataStructure& dataStructure) {
    auto* geometryPtr = TriangleGeom::Create(dataStructure, geometryPath.getTargetName());
    REQUIRE(geometryPtr != nullptr);
    auto store = DataStoreUtilities::CreateDataStore<float32>(dataStructure, vertexPath, {3}, {3});
    auto* verticesPtr = Float32Array::Create(dataStructure, vertexPath.getTargetName(), std::move(store), geometryPtr->getId());
    REQUIRE(verticesPtr != nullptr);
    constexpr std::array<float32, 9> coordinates = {1.0F, 2.0F, 3.0F, -2.0F, 0.0F, 4.0F, 0.0F, -1.0F, 2.0F};
    for(usize i = 0; i < coordinates.size(); i++)
    {
      (*verticesPtr)[i] = coordinates.at(i);
    }
    geometryPtr->setVertices(*verticesPtr);
  };
  const auto makeArgs = [&]() {
    Arguments args = filter.getDefaultArguments();
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(matrixPath));
    return args;
  };

  SECTION("Node geometry rejects matrices with fewer or more than 16 values")
  {
    for(const usize count : {3, 17})
    {
      CAPTURE(count);
      DataStructure dataStructure;
      createTriangle(dataStructure);
      createMatrix(dataStructure, count, 1);
      const auto result = filter.preflight(dataStructure, makeArgs());
      SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
      REQUIRE(result.outputActions.errors().size() == 1);
      REQUIRE(result.outputActions.errors()[0].code == -82019);
    }
  }

  SECTION("Node geometry accepts all 16-value layouts and translates vertices")
  {
    constexpr std::array<std::array<usize, 2>, 3> layouts = {{{16, 1}, {1, 16}, {4, 4}}};
    constexpr std::array<float32, 16> translation = {1.0F, 0.0F, 0.0F, 10.0F, 0.0F, 1.0F, 0.0F, -3.0F, 0.0F, 0.0F, 1.0F, 5.0F, 0.0F, 0.0F, 0.0F, 1.0F};
    constexpr std::array<float32, 9> expected = {11.0F, -1.0F, 8.0F, 8.0F, -3.0F, 9.0F, 10.0F, -4.0F, 7.0F};
    for(const auto& layout : layouts)
    {
      CAPTURE(layout[0], layout[1]);
      DataStructure dataStructure;
      createTriangle(dataStructure);
      auto* matrixPtr = createMatrix(dataStructure, layout[0], layout[1]);
      for(usize i = 0; i < translation.size(); i++)
      {
        (*matrixPtr)[i] = translation.at(i);
      }
      const auto args = makeArgs();
      const auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
      const auto execute = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
      const Float32Array* verticesPtr = nullptr;
      REQUIRE_NOTHROW(verticesPtr = &dataStructure.getDataRefAs<Float32Array>(vertexPath));
      REQUIRE(verticesPtr != nullptr);
      for(usize i = 0; i < expected.size(); i++)
      {
        CHECK((*verticesPtr)[i] == expected.at(i));
      }
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }

  SECTION("Image geometry retains its matrix size validation")
  {
    DataStructure dataStructure;
    auto* geometryPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
    REQUIRE(geometryPtr != nullptr);
    geometryPtr->setDimensions({1, 1, 1});
    auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 1, 1}, geometryPtr->getId());
    REQUIRE(cellsPtr != nullptr);
    geometryPtr->setCellData(*cellsPtr);
    createMatrix(dataStructure, 3, 1);
    auto args = makeArgs();
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(geometryPath.createChildPath("Cell Data")));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    REQUIRE(result.outputActions.errors()[0].code == -82019);
  }

  SECTION("An inactive precomputed path is not required")
  {
    DataStructure dataStructure;
    createTriangle(dataStructure);
    auto args = makeArgs();
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_translation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_TranslationNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_translation.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});

  VectorFloat32Parameter::ValueType rotation45X = {1.0F, 0.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Y = {0.0F, 1.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Z = {0.0F, 0.0F, 1.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation90X = {1.0F, 0.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Y = {0.0F, 1.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Z = {0.0F, 0.0F, 1.0F, 90.0F};
  auto [exemplaryGeomName, rotation] = GENERATE_REF(
      std::make_tuple(apply_transformation_to_geometry::k_Rotation45XNodeGeometryName, rotation45X), std::make_tuple(apply_transformation_to_geometry::k_Rotation45YNodeGeometryName, rotation45Y),
      std::make_tuple(apply_transformation_to_geometry::k_Rotation45ZNodeGeometryName, rotation45Z), std::make_tuple(apply_transformation_to_geometry::k_Rotation90XNodeGeometryName, rotation90X),
      std::make_tuple(apply_transformation_to_geometry::k_Rotation90YNodeGeometryName, rotation90Y), std::make_tuple(apply_transformation_to_geometry::k_Rotation90ZNodeGeometryName, rotation90Z));

  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>(rotation));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({exemplaryGeomName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_rotation.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_scale.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ScaleNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_scale.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale: Origin_And_Spacing Check", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::string k_SelectedImageGeomName = "Image Geometry";
  const DataPath k_SelectedImageGeomPath = DataPath({k_SelectedImageGeomName});
  const std::string k_SelectedAttrMatrixName = "Cell Data";
  const DataPath k_SelectedAttrMatrixPath = DataPath({k_SelectedImageGeomName, k_SelectedAttrMatrixName});

  DataStructure ds;
  ImageGeom* geom = ImageGeom::Create(ds, "Image Geometry");

  Vec3<float32> origin = {51.0F, 23.0F, -64.0F};
  Vec3<float32> spacing = {3.0F, 5.0F, 8.0F};
  std::vector<float32> scaleFactor;
  SECTION("Scale Increasing")
  {
    scaleFactor = {2.0F, 2.0F, 2.0F};
  }
  SECTION("Scale Decreasing")
  {
    scaleFactor = {0.4F, 0.4F, 0.4F};
  }
  geom->setOrigin(origin);
  geom->setSpacing(spacing);
  AttributeMatrix* cellAM = AttributeMatrix::Create(ds, "Cell Data", {}, geom->getId());

  const ApplyTransformationToGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedImageGeomPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_SelectedAttrMatrixPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>(scaleFactor));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(0));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  Vec3<float32> newOrigin = {1.0f, 1.0f, 1.0f};
  std::transform(origin.begin(), origin.end(), scaleFactor.begin(), newOrigin.begin(), std::multiplies<>());
  Vec3<float32> newSpacing = {1.0f, 1.0f, 1.0f};
  std::transform(spacing.begin(), spacing.end(), scaleFactor.begin(), newSpacing.begin(), std::multiplies<>());
  REQUIRE(geom->getOrigin() == newOrigin);
  REQUIRE(geom->getSpacing() == newSpacing);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<nx::core::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ManualNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_manual.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_precomputed.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/*******************************************************************************
 * @brief This section is for Image Geometry with Nearest Neighbor Interpolation
 ******************************************************************************/
TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_TranslationGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_TranslationGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_TranslationGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_TranslationGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
    const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({-10.0F, 10.0F, 20.0F}));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_translation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  VectorFloat32Parameter::ValueType rotation45X = {1.0F, 0.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Y = {0.0F, 1.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Z = {0.0F, 0.0F, 1.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation90X = {1.0F, 0.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Y = {0.0F, 1.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Z = {0.0F, 0.0F, 1.0F, 90.0F};
  auto [translateGeomToGlobalOrigin, exemplaryGeomName, rotation, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45XGeometryName, rotation45X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45XGlobalGeometryName, rotation45X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45YGeometryName, rotation45Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45YGlobalGeometryName, rotation45Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45ZGeometryName, rotation45Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45ZGlobalGeometryName, rotation45Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90XGeometryName, rotation90X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90XGlobalGeometryName, rotation90X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90YGeometryName, rotation90Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90YGlobalGeometryName, rotation90Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90ZGeometryName, rotation90Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90ZGlobalGeometryName, rotation90Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45XGeometryName, rotation45X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45XGlobalGeometryName, rotation45X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45YGeometryName, rotation45Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45YGlobalGeometryName, rotation45Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45ZGeometryName, rotation45Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45ZGlobalGeometryName, rotation45Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90XGeometryName, rotation90X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90XGlobalGeometryName, rotation90X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90YGeometryName, rotation90Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90YGlobalGeometryName, rotation90Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90ZGeometryName, rotation90Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90ZGlobalGeometryName, rotation90Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Rotation = [{}, {}, {}, {}], Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, rotation[0], rotation[1], rotation[2],
                              rotation[3], interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>(rotation));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
      UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, dataStructure.getDataRefAs<AttributeMatrix>(inputCellAMPath).getSize());
      std::vector<DataPath> expectedArrayPaths;
      const DataPath outputCellPath({fmt::format(".{}", inputGeometryPath.getTargetName()), inputCellAMPath.getTargetName()});
      for([[maybe_unused]] const auto& [id, child] : dataStructure.getDataRefAs<AttributeMatrix>(inputCellAMPath))
      {
        expectedArrayPaths.push_back(outputCellPath.createChildPath(child->getName()));
      }
      UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, expectedArrayPaths);

      // Execute the filter and check the result
      UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_ScaleGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ScaleGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_ScaleGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ScaleGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.05F, 0.05F, 0.05F}));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_scale.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_ManualGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ManualGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_ManualGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ManualGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                          std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath)); // This should reflect the geometry across the x-axis.
      const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<nx::core::DynamicTableParameter::ValueType>(dynamicTable));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      // Execute the filter and check the result
      UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_PrecomputedGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_PrecomputedGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_PrecomputedGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_PrecomputedGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Precomputed Matrix Path = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName,
                              apply_transformation_to_geometry::k_PrecomputedTransformationMatrixPath.toString(), interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                          std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      // Execute the filter and check the result
      UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ApplyTransformationToGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ApplyTransformationToGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ApplyTransformationToGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ApplyTransformationToGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK_FALSE(args.value<bool>(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key));
      CHECK_FALSE(args.value<bool>(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key));
      // Complex type (FloatVec3p1FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<ChoicesParameter::ValueType>(ApplyTransformationToGeometryFilter::k_TransformationType_Key) == 0);
      CHECK(args.value<ChoicesParameter::ValueType>(ApplyTransformationToGeometryFilter::k_InterpolationType_Key) == 0);
      // Complex type (DynamicTableFilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
    }
  }
}

namespace
{
const DataPath k_FeatureGeomPath({"Geometry"});
const DataPath k_FeatureAMPath({"Geometry", "Feature Data"});
const DataPath k_ElementAMPath({"Geometry", "Element Data"});
const DataPath k_TransformFeatureIdsPath({"Geometry", "Element Data", "FeatureIds"});

// Three cells or vertices carry sparse IDs 1, 3, 5. Feature data has a non-flat shape.
DataStructure CreateTransformFeatureFixture(bool node = false)
{
  DataStructure ds;
  ds.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  IGeometry* geom = nullptr;
  if(node)
  {
    auto* vertices = VertexGeom::Create(ds, "Geometry");
    REQUIRE(vertices != nullptr);
    auto* coords = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {3}, {3}, vertices->getId());
    REQUIRE(coords != nullptr);
    coords->fill(0);
    (*coords)[3] = 1;
    (*coords)[6] = 2;
    vertices->setVertices(*coords);
    geom = vertices;
  }
  else
  {
    auto* image = ImageGeom::Create(ds, "Geometry");
    REQUIRE(image != nullptr);
    image->setDimensions({3, 1, 1});
    image->setOrigin({0, 0, 0});
    image->setSpacing({1, 1, 1});
    geom = image;
  }
  auto* elements = AttributeMatrix::Create(ds, "Element Data", node ? ShapeType{3} : ShapeType{1, 1, 3}, geom->getId());
  REQUIRE(elements != nullptr);
  if(node)
  {
    dynamic_cast<VertexGeom*>(geom)->setVertexAttributeMatrix(*elements);
  }
  else
  {
    dynamic_cast<ImageGeom*>(geom)->setCellData(*elements);
  }
  auto* ids = UnitTest::CreateTestDataArray<int32>(ds, "FeatureIds", elements->getShape(), {1}, elements->getId());
  REQUIRE(ids != nullptr);
  (*ids)[0] = 1;
  (*ids)[1] = 3;
  (*ids)[2] = 5;
  auto* features = AttributeMatrix::Create(ds, "Feature Data", ShapeType{2, 3}, geom->getId());
  REQUIRE(features != nullptr);
  auto* values = UnitTest::CreateTestDataArray<int32>(ds, "Values", features->getShape(), {1}, features->getId());
  REQUIRE(values != nullptr);
  for(usize i = 0; i < 6; ++i)
  {
    (*values)[i] = static_cast<int32>(10 + i);
  }
  REQUIRE(StringArray::CreateWithValues(ds, "Names", ShapeType{2, 3}, {"zero", "one", "two", "three", "four", "five"}, features->getId()) != nullptr);
  auto* neighbors = NeighborList<int32>::Create(ds, "Neighbors", ShapeType{2, 3}, features->getId());
  REQUIRE(neighbors != nullptr);
  neighbors->setList(1, std::vector<int32>{3});
  neighbors->setList(3, std::vector<int32>{1, 5});
  neighbors->setList(5, std::vector<int32>{3});
  auto* ensemble = AttributeMatrix::Create(ds, "Ensemble", ShapeType{2}, geom->getId());
  REQUIRE(ensemble != nullptr);
  auto* phases = UnitTest::CreateTestDataArray<int32>(ds, "Phases", {2}, {1}, ensemble->getId());
  REQUIRE(phases != nullptr);
  (*phases)[0] = 42;
  (*phases)[1] = 99;
  return ds;
}

Arguments MakeTransformFeatureArgs(bool clear, bool renumber, uint64 transform = 3, bool node = false)
{
  Arguments args;
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(clear));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(renumber));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_TransformFeatureIdsPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_FeatureGeomPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(node ? DataPath{} : k_ElementAMPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(node ? 2 : 0));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(transform));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0, 0, 1, 90}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{2, 3, 4}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{2, 3, 4}));
  return args;
}

void CheckTransformFeatureData(const DataStructure& ds, bool clear, const ShapeType& shape)
{
  const auto* features = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(features != nullptr);
  CHECK(features->getShape() == shape);
  CHECK(features->getSize() == (clear ? 0 : 3));
  if(!clear)
  {
    const auto* values = ds.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath("Values"));
    REQUIRE(values != nullptr);
    for(usize i = 0; i < 6; ++i)
    {
      CHECK((*values)[i] == static_cast<int32>(10 + i));
    }
    const auto* names = ds.getDataAs<StringArray>(k_FeatureAMPath.createChildPath("Names"));
    REQUIRE(names != nullptr);
    CHECK((*names)[5] == "five");
    const auto* neighbors = ds.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath("Neighbors"));
    REQUIRE(neighbors != nullptr);
    CHECK(neighbors->getList(3) == std::vector<int32>{1, 5});
  }
  const auto* phases = ds.getDataAs<Int32Array>(DataPath({"Geometry", "Ensemble", "Phases"}));
  REQUIRE(phases != nullptr);
  REQUIRE(phases->getNumberOfTuples() == 2);
  CHECK((*phases)[0] == 42);
  CHECK((*phases)[1] == 99);
  CHECK(ds.getData(DataPath({".Geometry"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM replacement", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool clear = GENERATE(true, false);
  const uint64 transform = GENERATE(1, 2, 3);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto args = MakeTransformFeatureArgs(clear, false, transform);
  // Manual and precomputed matrices rotate 90 degrees about Z, as does the axis-angle default.
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key,
                      std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{0, -1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
  auto* matrices = DataGroup::Create(ds, "Matrices");
  REQUIRE(matrices != nullptr);
  auto* matrix = UnitTest::CreateTestDataArray<float32>(ds, "Matrix", {16}, {1}, matrices->getId());
  REQUIRE(matrix != nullptr);
  const std::array<float32, 16> entries = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  for(usize i = 0; i < entries.size(); ++i)
  {
    (*matrix)[i] = entries[i];
  }
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(DataPath({"Matrices", "Matrix"})));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, clear, {2, 3});
  const auto* ids = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(ids != nullptr);
  REQUIRE(ids->getNumberOfTuples() == 3);
  CHECK((*ids)[0] == 1);
  CHECK((*ids)[1] == 3);
  CHECK((*ids)[2] == 5);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM in place", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool node = GENERATE(false, true);
  const uint64 transform = GENERATE(0, 4, 5);
  const bool clear = GENERATE(true, false);
  const bool renumber = GENERATE(true, false);
  auto ds = CreateTransformFeatureFixture(node);
  const auto geomId = ds.getData(k_FeatureGeomPath)->getId();
  const auto featureAmId = ds.getData(k_FeatureAMPath)->getId();
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(ds, MakeTransformFeatureArgs(clear, renumber, transform, node));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, clear, clear && renumber ? ShapeType{4} : ShapeType{2, 3});
  CHECK(ds.getData(k_FeatureGeomPath)->getId() == geomId);
  CHECK(ds.getData(k_FeatureAMPath)->getId() == featureAmId);
  const auto* ids = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(ids != nullptr);
  CHECK((*ids)[0] == 1);
  CHECK((*ids)[1] == (clear && renumber ? 2 : 3));
  CHECK((*ids)[2] == (clear && renumber ? 3 : 5));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM rotation drops feature", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool renumber = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto args = MakeTransformFeatureArgs(true, renumber);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0, 0, 1, 45}));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  // With c=sqrt(1/2), the rotated box has origin (-c,0,0) and a 3x3x1 grid.
  // Inverse centers are ((i+j+1)*c-1/2, (j-i)*c+1/2, 1/2).
  // Only (i,j)=(0,0),(1,1) lie inside the source: x=0.207,1.621, y=0.5.
  // Thus source cell 2 (ID 5) disappears. Background stays 0; ID 3 becomes 2.
  const std::vector<int32> expected = {1, 0, 0, 0, renumber ? 2 : 3, 0, 0, 0, 0};
  const auto* ids = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(ids != nullptr);
  REQUIRE(ids->getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    CHECK((*ids)[i] == expected[i]);
  }
  CheckTransformFeatureData(ds, true, renumber ? ShapeType{3} : ShapeType{2, 3});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM preflight", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool clear = GENERATE(true, false);
  auto ds = CreateTransformFeatureFixture();
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, MakeTransformFeatureArgs(clear, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  const auto it =
      std::find_if(result.outputValues.begin(), result.outputValues.end(), [clear](const auto& value) { return value.name == (clear ? "Cleared Feature Data" : "Feature Data Not Cleared"); });
  REQUIRE(it != result.outputValues.end());
  if(clear)
  {
    CHECK(it->value.find("Geometry/Feature Data/Values") != std::string::npos);
    CHECK(it->value.find("Geometry/Feature Data/Names") != std::string::npos);
    CHECK(it->value.find("Geometry/Feature Data/Neighbors") != std::string::npos);
    CHECK(it->value.find("Ensemble") == std::string::npos);
  }
  CHECK(filter.parametersVersion() == 3);
  CHECK(filter.getDefaultArguments().value<bool>(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key));
  CHECK_FALSE(filter.getDefaultArguments().value<bool>(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM errors", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool node = GENERATE(false, true);
  auto ds = CreateTransformFeatureFixture(node);
  auto args = MakeTransformFeatureArgs(true, true, 4, node);
  int32 expectedCode = 0;
  SECTION("Feature AM outside geometry")
  {
    REQUIRE(AttributeMatrix::Create(ds, "Outside", ShapeType{6}) != nullptr);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Outside"})));
    expectedCode = -67451;
  }
  SECTION("Element AM cannot be cleared")
  {
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_ElementAMPath));
    expectedCode = -67452;
  }
  SECTION("Feature IDs outside geometry")
  {
    auto* outside = AttributeMatrix::Create(ds, "Outside", ShapeType{3});
    REQUIRE(outside != nullptr);
    REQUIRE(UnitTest::CreateTestDataArray<int32>(ds, "Ids", {3}, {1}, outside->getId()) != nullptr);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Outside", "Ids"})));
    expectedCode = -67453;
  }
  SECTION("Feature IDs tuple count")
  {
    auto* elements = ds.getDataAs<AttributeMatrix>(k_ElementAMPath);
    REQUIRE(elements != nullptr);
    auto resize = elements->resizeTuples({2});
    SIMPLNX_RESULT_REQUIRE_VALID(resize);
    expectedCode = -67454;
  }
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  if(expectedCode == -67451)
  {
    CHECK(result.outputActions.errors()[0].message.find("Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.") != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM shared parent", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto* group = DataGroup::Create(ds, "Alias");
  REQUIRE(group != nullptr);
  REQUIRE(ds.setAdditionalParent(ds.getData(k_FeatureAMPath)->getId(), group->getId()));
  auto args = MakeTransformFeatureArgs(true, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Alias", "Feature Data"})));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM validates dropped source ID", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const int32 invalidId = GENERATE(-1, 6);
  auto ds = CreateTransformFeatureFixture();
  auto* ids = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(ids != nullptr);
  (*ids)[2] = invalidId;
  auto args = MakeTransformFeatureArgs(true, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0, 0, 1, 45}));
  ApplyTransformationToGeometryFilter filter;
  // Source validation must reject even the cell that resampling drops, before dispatch.
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == (invalidId < 0 ? -5355 : -5351));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM node rotation", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateTransformFeatureFixture(true);
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(ds, MakeTransformFeatureArgs(true, true, 3, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
  const auto* coords = ds.getDataAs<Float32Array>(DataPath({"Geometry", "Vertices"}));
  REQUIRE(coords != nullptr);
  CHECK((*coords)[3] == Approx(0).margin(1e-6));
  CHECK((*coords)[4] == Approx(1));
  CHECK((*coords)[6] == Approx(0).margin(1e-6));
  CHECK((*coords)[7] == Approx(2));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM nested alias", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto* nested = DataGroup::Create(ds, "Misc", ds.getData(k_FeatureGeomPath)->getId());
  REQUIRE(nested != nullptr);
  auto* keep = UnitTest::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nested->getId());
  REQUIRE(keep != nullptr);
  (*keep)[0] = 123;
  REQUIRE(ds.setAdditionalParent(ds.getData(k_FeatureAMPath)->getId(), nested->getId()));
  auto* outside = DataGroup::Create(ds, "Outside");
  REQUIRE(outside != nullptr);
  REQUIRE(ds.setAdditionalParent(nested->getId(), outside->getId()));
  auto args = MakeTransformFeatureArgs(true, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "Misc", "Feature Data"})));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
  CHECK(ds.getData(DataPath({"Geometry", "Misc", "Feature Data"})) == nullptr);
  const auto* copied = ds.getDataAs<Int32Array>(DataPath({"Geometry", "Misc", "Keep"}));
  REQUIRE(copied != nullptr);
  CHECK((*copied)[0] == 123);
  const auto* original = ds.getDataAs<AttributeMatrix>(DataPath({"Outside", "Misc", "Feature Data"}));
  REQUIRE(original != nullptr);
  CHECK(original->getShape() == ShapeType{2, 3});
  CHECK(original->getSize() == 3);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM shared input survives", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool renumber = GENERATE(false, true);
  auto ds = CreateTransformFeatureFixture(true);
  auto* features = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(features != nullptr);
  auto resized = features->resizeTuples({3});
  SIMPLNX_RESULT_REQUIRE_VALID(resized);
  REQUIRE(ds.setAdditionalParent(ds.getData(k_TransformFeatureIdsPath)->getId(), features->getId()));
  REQUIRE(ds.setAdditionalParent(ds.getData(DataPath({"Geometry", "Vertices"}))->getId(), features->getId()));
  auto* sourceIds = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(sourceIds != nullptr);
  (*sourceIds)[0] = 0;
  (*sourceIds)[1] = 2;
  (*sourceIds)[2] = 2;
  ApplyTransformationToGeometryFilter filter;
  auto args = MakeTransformFeatureArgs(true, renumber, 3, true);
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  DataStructure staged(ds);
  auto actions = preflight.outputActions.value().applyRegular(staged, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actions);
  REQUIRE(staged.getData(k_TransformFeatureIdsPath) != nullptr);
  REQUIRE(staged.getData(DataPath({"Geometry", "Vertices"})) != nullptr);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, renumber ? ShapeType{2} : ShapeType{3});
  const auto* ids = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(ids != nullptr);
  CHECK((*ids)[2] == (renumber ? 1 : 2));
  const auto* coords = ds.getDataAs<Float32Array>(DataPath({"Geometry", "Vertices"}));
  REQUIRE(coords != nullptr);
  CHECK((*coords)[7] == Approx(2));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM precomputed input survives", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateTransformFeatureFixture(true);
  auto* features = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(features != nullptr);
  auto resized = features->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resized);
  auto* matrix = UnitTest::CreateTestDataArray<float32>(ds, "Matrix", {4}, {4}, features->getId());
  REQUIRE(matrix != nullptr);
  const std::array<float32, 16> entries = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  for(usize i = 0; i < entries.size(); ++i)
  {
    (*matrix)[i] = entries[i];
  }
  auto args = MakeTransformFeatureArgs(true, false, 1, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_FeatureAMPath.createChildPath("Matrix")));
  ApplyTransformationToGeometryFilter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  DataStructure staged(ds);
  auto actions = preflight.outputActions.value().applyRegular(staged, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actions);
  REQUIRE(staged.getData(k_FeatureAMPath.createChildPath("Matrix")) != nullptr);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
  const auto* coords = ds.getDataAs<Float32Array>(DataPath({"Geometry", "Vertices"}));
  REQUIRE(coords != nullptr);
  CHECK((*coords)[7] == Approx(2));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: No Transform keeps non-interpolated cell arrays", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool clear = GENERATE(true, false);
  auto ds = CreateTransformFeatureFixture();
  auto* mask = UnitTest::CreateTestDataArray<bool>(ds, "Mask", {1, 1, 3}, {1}, ds.getData(k_ElementAMPath)->getId());
  REQUIRE(mask != nullptr);
  (*mask)[1] = true;
  auto args = MakeTransformFeatureArgs(clear, false, 0);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  ApplyTransformationToGeometryFilter filter;
  // No Transform keeps the geometry in place, so Linear interpolation drops no cell array.
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  for(const auto& warning : preflight.outputActions.warnings())
  {
    CHECK(warning.code != 82010);
  }
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* keptMask = ds.getDataAs<BoolArray>(k_ElementAMPath.createChildPath("Mask"));
  REQUIRE(keptMask != nullptr);
  CHECK((*keptMask)[1]);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: StrictFeatureIds preflight", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateTransformFeatureFixture(true);
  // A direct child AM with one tuple per vertex that is not the registered Vertex AM.
  auto* otherData = AttributeMatrix::Create(ds, "Other Data", ShapeType{3}, ds.getData(k_FeatureGeomPath)->getId());
  REQUIRE(otherData != nullptr);
  auto* misplacedIds = UnitTest::CreateTestDataArray<int32>(ds, "MisplacedFeatureIds", {3}, {1}, otherData->getId());
  REQUIRE(misplacedIds != nullptr);
  misplacedIds->fill(1);

  ApplyTransformationToGeometryFilter filter;
  auto args = MakeTransformFeatureArgs(true, true, 4, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "Other Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == -67453);
    CHECK(result.outputActions.errors().front().message ==
          "**Feature Ids** DataArray 'Geometry/Other Data/MisplacedFeatureIds' must be in a Vertex, Edge, Face, or Polyhedron Attribute Matrix of the selected geometry 'Geometry'. "
          "Select an array from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Vertex Attribute Matrix")
  {
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_TransformFeatureIdsPath));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Linear interpolation preserves source labels", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto* elements = ds.getDataAs<AttributeMatrix>(k_ElementAMPath);
  REQUIRE(elements != nullptr);
  auto* signal = UnitTest::CreateTestDataArray<float32>(ds, "Signal", {1, 1, 3}, {1}, elements->getId());
  REQUIRE(signal != nullptr);
  (*signal)[0] = 0;
  (*signal)[1] = 20;
  (*signal)[2] = 40;
  auto* sourceIds = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(sourceIds != nullptr);
  (*sourceIds)[2] = 3;
  auto args = MakeTransformFeatureArgs(true, true, 2);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key,
                      std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1, 0.75, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  // The shear maps the box to [0,3.75]x[0,1]x[0,1], with Y spacing 1.25.
  // Inverse centers have x=i+0.03125, y=0.625.
  // Source cells are 0,1,2,outside: labels 1,3,3,0 become 1,2,2,0.
  const std::array<int32, 4> expected = {1, 2, 2, 0};
  const auto* ids = ds.getDataAs<Int32Array>(k_TransformFeatureIdsPath);
  REQUIRE(ids != nullptr);
  REQUIRE(ids->getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    CHECK((*ids)[i] == expected[i]);
  }
  const auto* outputSignal = ds.getDataAs<Float32Array>(k_ElementAMPath.createChildPath("Signal"));
  REQUIRE(outputSignal != nullptr);
  CHECK((*outputSignal)[1] == Approx(10.625));
  CHECK((*outputSignal)[2] == Approx(30.625));
  CheckTransformFeatureData(ds, true, {3});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Reject nonnumeric Feature Ids", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool strings = GENERATE(true, false);
  auto ds = CreateTransformFeatureFixture();
  const auto idsPath = k_TransformFeatureIdsPath;
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
  ApplyTransformationToGeometryFilter filter;
  auto args = MakeTransformFeatureArgs(true, true);
  REQUIRE_NOTHROW(filter.preflight(ds, args));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67455);
  CHECK(result.outputActions.errors()[0].message.find(idsPath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("single-component int32 DataArray") != std::string::npos);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Reject matrix output in cleared Feature AM", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool alias = GENERATE(false, true);
  auto ds = CreateTransformFeatureFixture();
  auto* features = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(features != nullptr);
  auto resizeResult = features->resizeTuples({16});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  DataPath parentPath = k_FeatureAMPath;
  if(alias)
  {
    auto* other = DataGroup::Create(ds, "Alias");
    REQUIRE(other != nullptr);
    REQUIRE(ds.setAdditionalParent(features->getId(), other->getId()));
    parentPath = DataPath({"Alias", "Feature Data"});
  }
  auto args = MakeTransformFeatureArgs(true, false, 4);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SaveTransformMatrix_Key, std::make_any<bool>(true));
  const auto outputPath = parentPath.createChildPath("Matrix");
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformMatrixOutputPath_Key, std::make_any<DataPath>(outputPath));
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67456);
  CHECK(result.outputActions.errors()[0].message.find(outputPath.toString()) != std::string::npos);
  CHECK(ds.getData(outputPath) == nullptr);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Reject another geometry element Feature AM", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateTransformFeatureFixture();
  const auto featurePath = k_FeatureAMPath;
  const auto* features = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(features != nullptr);
  auto* other = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(other != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {features->getNumberOfTuples()}, {3}, other->getId());
  REQUIRE(vertices != nullptr);
  other->setVertices(*vertices);
  REQUIRE(ds.setAdditionalParent(features->getId(), other->getId()));
  other->setVertexAttributeMatrix(*features);
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, MakeTransformFeatureArgs(true, false, 4));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67460);
  CHECK(result.outputActions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(other->getVertexAttributeMatrix() == features);
  CHECK(features->getSize() > 0);
}
