#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/Filters/SetImageGeomOriginScalingFilter.hpp"
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
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Uneven scaling keeps the classified source axis", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  DataStructure dataStructure;
  const DataPath inputGeometryPath({"Input"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);
  auto* image = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 2, 2});
  image->setSpacing({2.0F, 3.0F, 5.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellAM = AttributeMatrix::Create(dataStructure, k_Cell_Data, {2, 2, 2}, image->getId());
  REQUIRE(cellAM != nullptr);
  image->setCellData(*cellAM);
  auto* values = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Data", {2, 2, 2}, {1}, cellAM->getId());
  REQUIRE(values != nullptr);
  values->fill(1);

  const ApplyTransformationToGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
  const DynamicTableParameter::ValueType matrix{{1.0, 5.0, 0.0, 0.0}, {0.0, 100000.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0}};
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(matrix));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Hand calculation: (x,y,z) -> (x+5y,100000y,z). The normalized Y column's
  // off-axis entry is about 5e-5, so the classified source axes remain X, Y, Z.
  // Spacing is (2, 3*sqrt(100000^2+5^2), 5), or (2,300000,5) in float32.
  // Bounds have extents (4+5*6,100000*6,10) = (34,600000,10).
  // Extents divided by spacing give dimensions (17,2,2).
  const auto& output = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  const FloatVec3 expectedSpacing = {2.0F, 300000.0F, 5.0F};
  const SizeVec3 expectedDims = {17, 2, 2};
  for(usize axis = 0; axis < 3; ++axis)
  {
    CAPTURE(axis);
    CHECK(output.getSpacing()[axis] == Approx(expectedSpacing[axis]));
    CHECK(output.getDimensions()[axis] == expectedDims[axis]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
ImageGeom* MakeValidationImage(DataStructure& ds, const SizeVec3& dims = {2, 2, 2}, const FloatVec3& spacing = {1, 1, 1}, const FloatVec3& origin = {0, 0, 0})
{
  auto* geom = ImageGeom::Create(ds, "ValidationImage");
  REQUIRE(geom != nullptr);
  geom->setDimensions(dims);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
  auto* cells = AttributeMatrix::Create(ds, "Cells", {dims[2], dims[1], dims[0]}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  auto store = DataStoreUtilities::CreateDataStore<int32>(ds, DataPath({"ValidationImage", "Cells", "Samples"}), {dims[2], dims[1], dims[0]}, {1});
  REQUIRE(store != nullptr);
  auto* samples = Int32Array::Create(ds, "Samples", store, cells->getId());
  REQUIRE(samples != nullptr);
  for(usize index = 0; index < samples->getNumberOfTuples(); ++index)
  {
    (*samples)[index] = static_cast<int32>(index + 1);
  }
  return geom;
}
} // namespace

namespace
{
VertexGeom* MakeValidationVertices(DataStructure& ds)
{
  auto* geom = VertexGeom::Create(ds, "ValidationVertices");
  REQUIRE(geom != nullptr);
  auto* coords = Float32Array::CreateWithStore<Float32DataStore>(ds, "Vertices", {4}, {3}, geom->getId());
  REQUIRE(coords != nullptr);
  const std::array<float32, 12> values = {1, 2, 3, -1, 0, 2, 2, -3, 1, 0, 1, -2};
  for(usize idx = 0; idx < values.size(); ++idx)
  {
    (*coords)[idx] = values[idx];
  }
  geom->setVertices(*coords);
  return geom;
}
Arguments ValidationTransformArgs(usize type, bool image)
{
  ApplyTransformationToGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, DataPath({image ? "ValidationImage" : "ValidationVertices"}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, DataPath({"ValidationImage", "Cells"}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, ChoicesParameter::ValueType{image ? 0ULL : 2ULL});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, static_cast<ChoicesParameter::ValueType>(type));
  return args;
}
} // namespace
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Parameter domain regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8);
  CAPTURE(row);
  const bool image = row < 3 || row == 5;
  DataStructure ds;
  if(image)
  {
    MakeValidationImage(ds);
  }
  else
  {
    MakeValidationVertices(ds);
  }
  const usize type = row < 4 ? 5 : row < 6 ? 4 : 3;
  auto args = ValidationTransformArgs(type, image);
  const float32 nan = std::numeric_limits<float32>::quiet_NaN(), inf = std::numeric_limits<float32>::infinity();
  const std::vector<VectorFloat32Parameter::ValueType> values = {{0, 1, 1}, {-1, 1, 1}, {nan, 1, 1}, {nan, 1, 1}, {inf, 0, 0}, {nan, 0, 0}, {0, 0, 0, 90}, {nan, 0, 1, 90}, {0, 0, 1, inf}};
  args.insertOrAssign(type == 5 ? ApplyTransformationToGeometryFilter::k_Scale_Key :
                      type == 4 ? ApplyTransformationToGeometryFilter::k_Translation_Key :
                                  ApplyTransformationToGeometryFilter::k_Rotation_Key,
                      values[row]);
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  const std::array<int32, 9> codes = {-86004, -86004, -86004, -86003, -86002, -86002, -86052, -86051, -86053};
  REQUIRE(result.outputActions.errors()[0].code == codes[row]);
  CHECK(result.outputActions.errors()[0].message.find(type == 5 ? "Scale" : type == 4 ? "Translation" : "Rotation") != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(row == 8 ? "angle" : "X") != std::string::npos);
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Result domain regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2);
  CAPTURE(row);
  DataStructure ds;
  auto* geom = MakeValidationImage(ds, {1, 1, 1}, {row == 0 ? 2.0f : row == 1 ? 0.25f : 1.0f, 1, 1}, {row == 2 ? 3e38f : 0, 0, 0});
  const auto oldSpacing = geom->getSpacing(), oldOrigin = geom->getOrigin();
  auto args = ValidationTransformArgs(row == 2 ? 4 : 5, true);
  args.insertOrAssign(row == 2 ? ApplyTransformationToGeometryFilter::k_Translation_Key : ApplyTransformationToGeometryFilter::k_Scale_Key,
                      VectorFloat32Parameter::ValueType{row == 1 ? std::numeric_limits<float32>::denorm_min() : 3e38f, row == 2 ? 0.0f : 1.0f, row == 2 ? 0.0f : 1.0f});
  ApplyTransformationToGeometryFilter filter;
  auto pre = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  REQUIRE_FALSE(pre.outputActions.warnings().empty());
  CHECK(pre.outputActions.warnings()[0].code == (row == 2 ? -86005 : -86006));
  CHECK(pre.outputActions.warnings()[0].message.find("X") != std::string::npos);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors()[0].code == (row == 2 ? -86029 : -86028));
  const auto& message = result.result.errors()[0].message;
  CHECK(message.find("'ValidationImage'") != std::string::npos);
  CHECK(message.find("X") != std::string::npos);
  CHECK(message.find(row == 2 ? "existing origin 3e+38" : row == 0 ? "existing spacing 2" : "existing spacing 0.25") != std::string::npos);
  CHECK(message.find(row == 2 ? "assigned origin inf" : row == 0 ? "assigned spacing inf" : "assigned spacing 0") != std::string::npos);
  CHECK(message.find(row == 1 ? "computed result 0" : "computed result inf") != std::string::npos);
  CHECK(geom->getSpacing() == oldSpacing);
  CHECK(geom->getOrigin() == oldOrigin);
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Direct domain regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2);
  CAPTURE(row);
  DataStructure ds;
  if(row == 0)
  {
    MakeValidationImage(ds);
  }
  else
  {
    MakeValidationVertices(ds);
  }
  ApplyTransformationToGeometryInputValues input{};
  input.SelectedGeometryPath = DataPath({row == 0 ? "ValidationImage" : "ValidationVertices"});
  input.RemoveOriginalGeometry = true;
  input.TransformationSelection = row == 0 ? 5 : row == 1 ? 4 : 3;
  input.Scale = {0, 1, 1};
  input.Translation = {std::numeric_limits<float32>::quiet_NaN(), 0, 0};
  input.Rotation = {0, 0, 0, 90};
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  auto result = ApplyTransformationToGeometry(ds, handler, cancel, &input)();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors()[0].code == (row == 0 ? -86004 : row == 1 ? -86002 : -86052));
  if(row != 0)
  {
    auto* geom = ds.getDataAs<VertexGeom>(input.SelectedGeometryPath);
    const std::array<float32, 12> expected = {1, 2, 3, -1, 0, 2, 2, -3, 1, 0, 1, -2};
    for(usize index = 0; index < expected.size(); ++index)
    {
      CHECK(geom->getVerticesRef()[index] == expected[index]);
    }
  }
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Large axis vertex regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const float32 magnitude = GENERATE(1.0f, 1e20f);
  DataStructure ds;
  auto* geom = MakeValidationVertices(ds);
  auto args = ValidationTransformArgs(3, false);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32Parameter::ValueType{0, 0, magnitude, 90});
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const std::array<float32, 12> expected = {-2, 1, 3, 0, -1, 2, 3, 2, 1, -1, 0, -2};
  for(usize index = 0; index < expected.size(); ++index)
  {
    CHECK(geom->getVerticesRef()[index] == Approx(expected[index]).margin(1e-6));
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Direct vector lengths and image rotation", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto row = GENERATE(0, 1, 2);
  DataStructure ds;
  if(row == 2)
  {
    MakeValidationImage(ds);
  }
  else
  {
    MakeValidationVertices(ds);
  }
  ApplyTransformationToGeometryInputValues input{};
  input.SelectedGeometryPath = DataPath({row == 2 ? "ValidationImage" : "ValidationVertices"});
  input.RemoveOriginalGeometry = true;
  input.TransformationSelection = row == 0 ? 4 : row == 1 ? 5 : 3;
  input.Translation = {1, 2};
  input.Scale = {1, 2};
  input.Rotation = {0, 0, 0, 90};
  SizeVec3 destinationDims{};
  FloatVec3 destinationOrigin{};
  if(row == 2)
  {
    auto args = ValidationTransformArgs(3, true);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32Parameter::ValueType{0, 0, 1, 90});
    ApplyTransformationToGeometryFilter filter;
    auto pre = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
    auto applied = pre.outputActions.value().applyRegular(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(applied);
    auto* destination = ds.getDataAs<ImageGeom>(DataPath({".ValidationImage"}));
    REQUIRE(destination != nullptr);
    destinationDims = destination->getDimensions();
    destinationOrigin = destination->getOrigin();
    auto* samples = ds.getDataAs<Int32Array>(DataPath({".ValidationImage", "Cells", "Samples"}));
    REQUIRE(samples != nullptr);
    samples->fill(99);
    auto invalidArgs = ValidationTransformArgs(3, true);
    for(const auto& values : std::vector<VectorFloat32Parameter::ValueType>{{0, 0, 0, 90}, {std::numeric_limits<float32>::quiet_NaN(), 0, 1, 90}, {0, 0, 1, std::numeric_limits<float32>::infinity()}})
    {
      invalidArgs.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, values);
      auto rejected = filter.preflight(ds, invalidArgs);
      SIMPLNX_RESULT_REQUIRE_INVALID(rejected.outputActions);
      CHECK(rejected.outputActions.errors()[0].code == (values[0] != values[0] ? -86051 : values[3] != 90 ? -86053 : -86052));
    }
  }
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  auto result = ApplyTransformationToGeometry(ds, handler, cancel, &input)();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors()[0].code == (row == 0 ? -86000 : row == 1 ? -86001 : -86052));
  if(row != 2)
  {
    auto* geom = ds.getDataAs<VertexGeom>(input.SelectedGeometryPath);
    const std::array<float32, 12> expected = {1, 2, 3, -1, 0, 2, 2, -3, 1, 0, 1, -2};
    for(usize index = 0; index < expected.size(); ++index)
    {
      CHECK(geom->getVerticesRef()[index] == expected[index]);
    }
  }
  else
  {
    CHECK(ds.getDataAs<ImageGeom>(DataPath({"ValidationImage"}))->getOrigin() == FloatVec3{0, 0, 0});
    auto* destination = ds.getDataAs<ImageGeom>(DataPath({".ValidationImage"}));
    REQUIRE(destination != nullptr);
    CHECK(destination->getDimensions() == destinationDims);
    CHECK(destination->getOrigin() == destinationOrigin);
    auto* sourceSamples = ds.getDataAs<Int32Array>(DataPath({"ValidationImage", "Cells", "Samples"}));
    auto* destinationSamples = ds.getDataAs<Int32Array>(DataPath({".ValidationImage", "Cells", "Samples"}));
    REQUIRE(sourceSamples != nullptr);
    REQUIRE(destinationSamples != nullptr);
    for(usize index = 0; index < 8; ++index)
    {
      CHECK((*sourceSamples)[index] == index + 1);
      CHECK((*destinationSamples)[index] == 99);
    }
  }
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Finite extremes and inactive vectors", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5);
  DataStructure ds;
  const bool image = row < 3 || row == 5;
  ImageGeom* geom = nullptr;
  VertexGeom* vertices = nullptr;
  if(image)
  {
    geom = MakeValidationImage(ds, {1, 1, 1}, {row == 5 ? 0.0f : 1.0f, 1, 1});
  }
  else
  {
    vertices = MakeValidationVertices(ds);
  }
  auto args = ValidationTransformArgs(row == 2 || row == 5 ? 4 : 5, image);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32Parameter::ValueType{0, 0, 0, 90});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, VectorFloat32Parameter::ValueType{row == 0 ? std::numeric_limits<float32>::denorm_min() :
                                                                                                          row == 1 ? 1e20f :
                                                                                                          row == 3 ? -1.0f :
                                                                                                                     0.0f,
                                                                                                          1, 1});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, VectorFloat32Parameter::ValueType{row == 2 ? 2e38f : 1.0f, 0, 0});
  ApplyTransformationToGeometryFilter filter;
  auto pre = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  CHECK(std::none_of(pre.outputActions.warnings().begin(), pre.outputActions.warnings().end(), [](const Warning& warning) { return warning.code <= -86000 && warning.code >= -86069; }));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(row == 0)
  {
    CHECK(geom->getSpacing()[0] == std::numeric_limits<float32>::denorm_min());
  }
  if(row == 1)
  {
    CHECK(geom->getSpacing()[0] == 1e20f);
  }
  if(row == 2)
  {
    CHECK(geom->getOrigin()[0] == 2e38f);
  }
  if(row == 3 || row == 4)
  {
    const std::array<float32, 12> reflected = {-1, 2, 3, 1, 0, 2, -2, -3, 1, 0, 1, -2};
    const std::array<float32, 12> collapsed = {0, 2, 3, 0, 0, 2, 0, -3, 1, 0, 1, -2};
    const auto& expected = row == 3 ? reflected : collapsed;
    for(usize index = 0; index < expected.size(); ++index)
    {
      CHECK(vertices->getVerticesRef()[index] == expected[index]);
    }
  }
  if(row == 5)
  {
    CHECK(geom->getSpacing()[0] == 0);
    CHECK(geom->getOrigin()[0] == 1);
  }
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Pipeline stand-in metadata", "[SimplnxCore][ApplyTransformationToGeometryFilter][SetImageGeomOriginScalingFilter]")
{
  const auto row = GENERATE(0, 1);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope algorithmScope(scenario);
  Pipeline pipeline;
  auto first = ValidationTransformArgs(1, true);
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, DataPath({"Matrix"}));
  REQUIRE(pipeline.push_back(std::make_unique<ApplyTransformationToGeometryFilter>(), first));
  if(row == 0)
  {
    auto second = ValidationTransformArgs(5, true);
    second.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, VectorFloat32Parameter::ValueType{3e38f, 1, 1});
    REQUIRE(pipeline.push_back(std::make_unique<ApplyTransformationToGeometryFilter>(), second));
  }
  else
  {
    SetImageGeomOriginScalingFilter filter;
    auto second = filter.getDefaultArguments();
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, true);
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, true);
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, true);
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, VectorFloat32Parameter::ValueType{3e38f, 1, 1});
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, VectorFloat32Parameter::ValueType{0, 0, 0});
    REQUIRE(pipeline.push_back(std::make_unique<SetImageGeomOriginScalingFilter>(), second));
  }
  for(bool execute : {false, true})
  {
    DataStructure ds;
    MakeValidationImage(ds, row == 0 ? SizeVec3{1, 1, 1} : SizeVec3{10, 1, 1}, row == 0 ? FloatVec3{2, 1, 1} : FloatVec3{1, 1, 1});
    auto* matrix = Float32Array::CreateWithStore<Float32DataStore>(ds, "Matrix", {1}, {16});
    REQUIRE(matrix != nullptr);
    const std::array<float32, 16> values =
        row == 0 ? std::array<float32, 16>{0.5f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} : std::array<float32, 16>{0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    for(usize idx = 0; idx < 16; ++idx)
    {
      (*matrix)[idx] = values[idx];
    }
    if(!execute)
    {
      REQUIRE(pipeline.preflight(ds, false));
    }
    else
    {
      REQUIRE(algorithmScope.execute([&] { return pipeline.execute(ds, false); }));
      auto* geom = ds.getDataAs<ImageGeom>(DataPath({"ValidationImage"}));
      REQUIRE(geom != nullptr);
      if(row == 0)
      {
        CHECK(geom->getSpacing()[0] == 3e38f);
      }
      else
      {
        CHECK(geom->getDimensions() == SizeVec3{1, 10, 1});
        CHECK(geom->getOrigin()[0] == Approx(-1.5e38f));
        CHECK(geom->getOrigin()[1] == -5);
        CHECK(geom->getOrigin()[2] == -0.5f);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Principal rotation controls", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  MakeValidationImage(ds);
  auto args = ValidationTransformArgs(3, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32Parameter::ValueType{0, 0, 1, 90});
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  auto* geom = ds.getDataAs<ImageGeom>(DataPath({"ValidationImage"}));
  REQUIRE(geom != nullptr);
  CHECK(geom->getDimensions() == SizeVec3{2, 2, 2});
  auto* samples = ds.getDataAs<Int32Array>(DataPath({"ValidationImage", "Cells", "Samples"}));
  REQUIRE(samples != nullptr);
  const std::array<int32, 8> expected = {3, 1, 4, 2, 7, 5, 8, 6};
  for(usize index = 0; index < expected.size(); ++index)
  {
    CHECK((*samples)[index] == expected[index]);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}
