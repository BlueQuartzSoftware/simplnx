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
#include "simplnx/DataStructure/DataStore.hpp"
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
#include "simplnx/Filter/IParameter.hpp"
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
using namespace nx::core::constants;
using namespace nx::core::unit_test;

namespace
{
void ExecuteIdentityTransform(const ApplyTransformationToGeometryFilter& filter, DataStructure& dataStructure, const Arguments& args)
{
  auto result = filter.execute(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) -- The project assertion macro uses a do-while wrapper.
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

void CheckTransformDescendantPaths(unit_test::AlgorithmTestScope* algorithmTestScopePtr)
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
  const auto& geometry = unit_test::GetRequiredData<ImageGeom>(dataStructure, geometryPath);
  const auto& cells = unit_test::GetRequiredData<AttributeMatrix>(dataStructure, DataPath({"Grain Group", "Grain", "Grain Cells"}));
  auto pixelStore = data_store_utilities::CreateDataStore<int32>(dataStructure, cellPath, {2, 2, 2}, {1});
  REQUIRE((pixelStore != nullptr));
  auto* pixelsPtr = Int32ArrayType::Create(dataStructure, "Grain Pixels", pixelStore, cells.getId());
  REQUIRE((pixelsPtr != nullptr));
  constexpr std::array<int32, 8> k_ExpectedPixels = {2, 5, 11, 17, 23, 31, 41, 53};
  for(usize index = 0; index < k_ExpectedPixels.size(); ++index)
  {
    (*pixelsPtr)[index] = k_ExpectedPixels.at(index);
  }
  auto* dataPtr = DataGroup::Create(dataStructure, "Grain Data", geometry.getId());
  REQUIRE((dataPtr != nullptr));
  auto* detailsPtr = DataGroup::Create(dataStructure, "Grain Details", dataPtr->getId());
  REQUIRE((detailsPtr != nullptr));
  auto* metricsPtr = AttributeMatrix::Create(dataStructure, "Grain Metrics", {2}, detailsPtr->getId());
  REQUIRE((metricsPtr != nullptr));
  auto valuesStore = data_store_utilities::CreateDataStore<float32>(dataStructure, valuesPath, {2}, {1});
  REQUIRE((valuesStore != nullptr));
  auto* valuesPtr = Float32ArrayType::Create(dataStructure, "Grain Values", valuesStore, metricsPtr->getId());
  REQUIRE((valuesPtr != nullptr));
  (*valuesPtr)[0] = 7.5F;
  (*valuesPtr)[1] = -3.25F;
  auto directStore = data_store_utilities::CreateDataStore<float32>(dataStructure, directPath, {2}, {1});
  REQUIRE((directStore != nullptr));
  auto* directPtr = Float32ArrayType::Create(dataStructure, "Grain Transform", directStore, geometry.getId());
  REQUIRE((directPtr != nullptr));
  (*directPtr)[0] = 19.0F;
  (*directPtr)[1] = -29.0F;
  if(algorithmTestScopePtr == nullptr)
  {
    unit_test::RequireDiskBackedStore(*pixelsPtr);
    unit_test::RequireDiskBackedStore(*valuesPtr);
    unit_test::RequireDiskBackedStore(*directPtr);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

  // Check declarations separately: deepCopy preserves real descendants even if their declared paths are wrong.
  const std::vector<DataPath> expectedGroupPaths = {DataPath({"Grain Group", ".Grain", "Grain Data"}), DataPath({"Grain Group", ".Grain", "Grain Data", "Grain Details"}),
                                                    DataPath({"Grain Group", ".Grain", "Grain Data", "Grain Details", "Grain Metrics"}),
                                                    DataPath({"Grain Group", ".Grain", "Grain Data", "Grain Details", "Grain Metrics", "Grain Values"})};
  const std::vector<DataPath> expectedDirectPaths = {DataPath({"Grain Group", ".Grain", "Grain Transform"})};
  usize copyActionCount = 0;
  for(const auto& action : preflight.output_actions.value().actions)
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
    algorithmTestScopePtr->execute([&] { ExecuteIdentityTransform(filter, dataStructure, args); });
  }
  else
  {
    ExecuteIdentityTransform(filter, dataStructure, args);
  }
  if(algorithmTestScopePtr == nullptr)
  {
    const auto after = GetAlgorithmPathExecutionCounts();
    CHECK((after.out_of_core_on_out_of_core_store > before.out_of_core_on_out_of_core_store));
    CHECK((after.in_core == before.in_core));
    CHECK((after.out_of_core_on_in_memory_store == before.out_of_core_on_in_memory_store));
  }
  const auto& outputGeometry = unit_test::GetRequiredData<ImageGeom>(dataStructure, geometryPath);
  CHECK((outputGeometry.getDimensions() == SizeVec3Type{2, 2, 2}));
  CHECK((outputGeometry.getOrigin() == FloatVec3Type{0, 0, 0}));
  CHECK((outputGeometry.getSpacing() == FloatVec3Type{1, 1, 1}));
  const auto& outputPixels = unit_test::GetRequiredData<Int32ArrayType>(dataStructure, cellPath);
  const auto& outputValues = unit_test::GetRequiredData<Float32ArrayType>(dataStructure, valuesPath);
  const auto& outputDirect = unit_test::GetRequiredData<Float32ArrayType>(dataStructure, directPath);
  REQUIRE((outputPixels.getTupleShape() == ShapeType{2, 2, 2}));
  REQUIRE((outputPixels.getNumberOfComponents() == 1));
  REQUIRE((outputValues.getNumberOfTuples() == 2));
  REQUIRE((outputValues.getNumberOfComponents() == 1));
  REQUIRE((outputDirect.getNumberOfTuples() == 2));
  REQUIRE((outputDirect.getNumberOfComponents() == 1));
  for(usize index = 0; index < k_ExpectedPixels.size(); ++index)
  {
    CHECK((outputPixels[index] == k_ExpectedPixels.at(index)));
  }
  CHECK((outputValues[0] == 7.5F));
  CHECK((outputValues[1] == -3.25F));
  CHECK((outputDirect[0] == 19.0F));
  CHECK((outputDirect[1] == -29.0F));
  if(algorithmTestScopePtr == nullptr)
  {
    unit_test::RequireDiskBackedStore(outputPixels);
    unit_test::RequireDiskBackedStore(outputValues);
    unit_test::RequireDiskBackedStore(outputDirect);
  }
  CHECK((dataStructure.getTopLevelData().size() == 2));
  CHECK((unit_test::GetRequiredData<DataGroup>(dataStructure, DataPath({"Grain"})).getId() == decoyId));
  CHECK((unit_test::GetRequiredData<DataGroup>(dataStructure, DataPath({"Grain", "Keep Me"})).getId() == decoyChildId));
  CHECK_FALSE(dataStructure.containsData(DataPath({"Grain Group", ".Grain"})));
  CHECK_FALSE(dataStructure.containsData(DataPath({".Grain Group"})));
  CHECK_FALSE(dataStructure.containsData(DataPath({".Grain"})));
  CHECK_FALSE(dataStructure.containsData(DataPath({"Grain Group", "Grain", ".Grain Data"})));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

} // namespace

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: nested geometry preserves descendant names", "[SimplnxCore][ApplyTransformationToGeometryFilter][NestedPaths]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  CheckTransformDescendantPaths(&algorithmTestScope);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: nested geometry preserves descendant names with real OOC stores",
          "[SimplnxCore][ApplyTransformationToGeometryFilter][NestedPaths][.OocStoreContract]")
{
  REQUIRE(data_store_utilities::GetIOCollection().hasDataStoreCreationFunction("HDF5-OOC"));
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  REQUIRE_FALSE(ForceInCoreAlgorithm());
  REQUIRE_FALSE(ForceOocAlgorithm());
  CheckTransformDescendantPaths(nullptr);
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

static void CompareImageGeometries(const DataStructure& dataStructure, const ImageGeom& exemplaryGeom, const ImageGeom& calculatedGeom, const std::string& exemplaryDataName)
{
  unit_test::CompareImageGeometry(&exemplaryGeom, &calculatedGeom);

  REQUIRE_NOTHROW(exemplaryGeom.getCellDataRef());
  REQUIRE_NOTHROW(calculatedGeom.getCellDataRef());
  const auto& exemplaryAM = exemplaryGeom.getCellDataRef();
  const auto& calculatedAM = calculatedGeom.getCellDataRef();
  REQUIRE(exemplaryAM.getShape() == calculatedAM.getShape());

  const DataPath exemplarPath({exemplaryGeom.getName(), k_CellDataWithSpaces, exemplaryDataName});
  const DataPath calculatedPath({calculatedGeom.getName(), k_CellDataWithSpaces, "Data"});
  const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
  const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
  unit_test::CompareDataArrays<int32>(exemplarData, calculatedData);
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
    auto store = data_store_utilities::CreateDataStore<float32>(dataStructure, matrixPath, {tuples}, {components});
    auto* matrixPtr = Float32ArrayType::Create(dataStructure, matrixPath.getTargetName(), std::move(store));
    REQUIRE(matrixPtr != nullptr);
    return matrixPtr;
  };
  const auto createTriangle = [&](DataStructure& dataStructure) {
    auto* geometryPtr = TriangleGeom::Create(dataStructure, geometryPath.getTargetName());
    REQUIRE(geometryPtr != nullptr);
    auto store = data_store_utilities::CreateDataStore<float32>(dataStructure, vertexPath, {3}, {3});
    auto* verticesPtr = Float32ArrayType::Create(dataStructure, vertexPath.getTargetName(), std::move(store), geometryPtr->getId());
    REQUIRE(verticesPtr != nullptr);
    constexpr std::array<float32, 9> k_Coordinates = {1.0F, 2.0F, 3.0F, -2.0F, 0.0F, 4.0F, 0.0F, -1.0F, 2.0F};
    for(usize i = 0; i < k_Coordinates.size(); i++)
    {
      (*verticesPtr)[i] = k_Coordinates.at(i);
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
      SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
      REQUIRE(result.output_actions.errors().size() == 1);
      REQUIRE(result.output_actions.errors()[0].code == -82019);
    }
  }

  SECTION("Node geometry accepts all 16-value layouts and translates vertices")
  {
    constexpr std::array<std::array<usize, 2>, 3> k_Layouts = {{{16, 1}, {1, 16}, {4, 4}}};
    constexpr std::array<float32, 16> k_Translation = {1.0F, 0.0F, 0.0F, 10.0F, 0.0F, 1.0F, 0.0F, -3.0F, 0.0F, 0.0F, 1.0F, 5.0F, 0.0F, 0.0F, 0.0F, 1.0F};
    constexpr std::array<float32, 9> k_Expected = {11.0F, -1.0F, 8.0F, 8.0F, -3.0F, 9.0F, 10.0F, -4.0F, 7.0F};
    for(const auto& layout : k_Layouts)
    {
      CAPTURE(layout[0], layout[1]);
      DataStructure dataStructure;
      createTriangle(dataStructure);
      auto* matrixPtr = createMatrix(dataStructure, layout[0], layout[1]);
      for(usize i = 0; i < k_Translation.size(); i++)
      {
        (*matrixPtr)[i] = k_Translation.at(i);
      }
      const auto args = makeArgs();
      const auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
      const auto execute = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
      const Float32ArrayType* verticesPtr = nullptr;
      REQUIRE_NOTHROW(verticesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(vertexPath));
      REQUIRE(verticesPtr != nullptr);
      for(usize i = 0; i < k_Expected.size(); i++)
      {
        CHECK((*verticesPtr)[i] == k_Expected.at(i));
      }
      unit_test::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    REQUIRE(result.output_actions.errors()[0].code == -82019);
  }

  SECTION("An inactive precomputed path is not required")
  {
    DataStructure dataStructure;
    createTriangle(dataStructure);
    auto args = makeArgs();
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>({100.0F, 50.0F, -100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
    REQUIRE(preflightResult.output_actions.warnings().size() == 2);
    REQUIRE(preflightResult.output_actions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);
    const auto& unchangedWarning = preflightResult.output_actions.warnings()[1];
    CHECK(unchangedWarning.code == -4041);
    CHECK(unchangedWarning.message.find("\n" + inputGeometryPath.createChildPath("Precomputed AM").toString() + "\n") != std::string::npos);

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
    unit_test::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_translation.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});

  VectorFloat32ParameterType::ValueType rotation45X = {1.0F, 0.0F, 0.0F, 45.0F};
  VectorFloat32ParameterType::ValueType rotation45Y = {0.0F, 1.0F, 0.0F, 45.0F};
  VectorFloat32ParameterType::ValueType rotation45Z = {0.0F, 0.0F, 1.0F, 45.0F};
  VectorFloat32ParameterType::ValueType rotation90X = {1.0F, 0.0F, 0.0F, 90.0F};
  VectorFloat32ParameterType::ValueType rotation90Y = {0.0F, 1.0F, 0.0F, 90.0F};
  VectorFloat32ParameterType::ValueType rotation90Z = {0.0F, 0.0F, 1.0F, 90.0F};
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
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>(rotation));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
    REQUIRE(preflightResult.output_actions.warnings().size() == 2);
    REQUIRE(preflightResult.output_actions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);
    const auto& unchangedWarning = preflightResult.output_actions.warnings()[1];
    CHECK(unchangedWarning.code == -4041);
    CHECK(unchangedWarning.message.find("\n" + inputGeometryPath.createChildPath("Precomputed AM").toString() + "\n") != std::string::npos);

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
    unit_test::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_rotation.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
    REQUIRE(preflightResult.output_actions.warnings().size() == 2);
    REQUIRE(preflightResult.output_actions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);
    const auto& unchangedWarning = preflightResult.output_actions.warnings()[1];
    CHECK(unchangedWarning.code == -4041);
    CHECK(unchangedWarning.message.find("\n" + inputGeometryPath.createChildPath("Precomputed AM").toString() + "\n") != std::string::npos);

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
    unit_test::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_scale.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale: Origin_And_Spacing Check", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const std::string kSelectedImageGeomName = "Image Geometry";
  const DataPath kSelectedImageGeomPath = DataPath({kSelectedImageGeomName});
  const std::string kSelectedAttrMatrixName = "Cell Data";
  const DataPath kSelectedAttrMatrixPath = DataPath({kSelectedImageGeomName, kSelectedAttrMatrixName});

  DataStructure dataStructure;
  ImageGeom* geomPtr = ImageGeom::Create(dataStructure, "Image Geometry");

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
  REQUIRE(geomPtr->setOrigin(origin).valid());
  REQUIRE(geomPtr->setSpacing(spacing).valid());
  AttributeMatrix* cellAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {}, geomPtr->getId());

  const ApplyTransformationToGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kSelectedImageGeomPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(kSelectedAttrMatrixPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>(scaleFactor));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(0));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  Vec3<float32> newOrigin = {1.0f, 1.0f, 1.0f};
  std::transform(origin.begin(), origin.end(), scaleFactor.begin(), newOrigin.begin(), std::multiplies<>());
  Vec3<float32> newSpacing = {1.0f, 1.0f, 1.0f};
  std::transform(spacing.begin(), spacing.end(), scaleFactor.begin(), newSpacing.begin(), std::multiplies<>());
  REQUIRE(geomPtr->getOrigin() == newOrigin);
  REQUIRE(geomPtr->getSpacing() == newSpacing);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
    REQUIRE(preflightResult.output_actions.warnings().size() == 2);
    REQUIRE(preflightResult.output_actions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);
    const auto& unchangedWarning = preflightResult.output_actions.warnings()[1];
    CHECK(unchangedWarning.code == -4041);
    CHECK(unchangedWarning.message.find("\n" + inputGeometryPath.createChildPath("Precomputed AM").toString() + "\n") != std::string::npos);

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
    unit_test::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_manual.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    REQUIRE(preflightResult.output_actions.warnings().size() == 2);
    REQUIRE(preflightResult.output_actions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);
    const auto& unchangedWarning = preflightResult.output_actions.warnings()[1];
    CHECK(unchangedWarning.code == -4041);
    CHECK(unchangedWarning.message.find("\n" + inputGeometryPath.createChildPath("Precomputed AM").toString() + "\n") != std::string::npos);

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
    unit_test::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_precomputed.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

/*******************************************************************************
 * @brief This section is for Image Geometry with Nearest Neighbor Interpolation
 ******************************************************************************/
TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

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
    const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellDataWithSpaces);

    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>({-10.0F, 10.0F, 20.0F}));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
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

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellDataWithSpaces);

  VectorFloat32ParameterType::ValueType rotation45X = {1.0F, 0.0F, 0.0F, 45.0F};
  VectorFloat32ParameterType::ValueType rotation45Y = {0.0F, 1.0F, 0.0F, 45.0F};
  VectorFloat32ParameterType::ValueType rotation45Z = {0.0F, 0.0F, 1.0F, 45.0F};
  VectorFloat32ParameterType::ValueType rotation90X = {1.0F, 0.0F, 0.0F, 90.0F};
  VectorFloat32ParameterType::ValueType rotation90Y = {0.0F, 1.0F, 0.0F, 90.0F};
  VectorFloat32ParameterType::ValueType rotation90Z = {0.0F, 0.0F, 1.0F, 90.0F};
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
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>(rotation));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
      unit_test::RequireAutomaticCreateArrayActions(preflightResult.output_actions, dataStructure.getDataRefAs<AttributeMatrix>(inputCellAMPath).getSize());
      std::vector<DataPath> expectedArrayPaths;
      const DataPath outputCellPath({fmt::format(".{}", inputGeometryPath.getTargetName()), inputCellAMPath.getTargetName()});
      for([[maybe_unused]] const auto& [id, child] : dataStructure.getDataRefAs<AttributeMatrix>(inputCellAMPath))
      {
        expectedArrayPaths.push_back(outputCellPath.createChildPath(child->getName()));
      }
      unit_test::RequireAutomaticCreateArrayActions(preflightResult.output_actions, expectedArrayPaths);

      // Execute the filter and check the result
      unit_test::AlgorithmTestScope algorithmTestScope(scenario);
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

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellDataWithSpaces);

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
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32ParameterType::ValueType>({0.05F, 0.05F, 0.05F}));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

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

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellDataWithSpaces);

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
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

      // Execute the filter and check the result
      unit_test::AlgorithmTestScope algorithmTestScope(scenario);
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

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Uneven scaling keeps the classified source axis", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  DataStructure dataStructure;
  const DataPath inputGeometryPath({"Input"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellDataWithSpaces);
  auto* imagePtr = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(imagePtr != nullptr);
  imagePtr->setDimensions({2, 2, 2});
  REQUIRE(imagePtr->setSpacing({2.0F, 3.0F, 5.0F}).valid());
  REQUIRE(imagePtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, k_CellDataWithSpaces, {2, 2, 2}, imagePtr->getId());
  REQUIRE(cellAmPtr != nullptr);
  imagePtr->setCellData(*cellAmPtr);
  auto* valuesPtr = Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, "Data", {2, 2, 2}, {1}, cellAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(1);

  const ApplyTransformationToGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
  const DynamicTableParameter::ValueType matrix{{1.0, 5.0, 0.0, 0.0}, {0.0, 100000.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0}};
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(matrix));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<bool>(false));
  // This fixture has no Feature Attribute Matrix.
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Hand calculation: (x,y,z) -> (x+5y,100000y,z). The normalized Y column's
  // off-axis entry is about 5e-5, so the classified source axes remain X, Y, Z.
  // Spacing is (2, 3*sqrt(100000^2+5^2), 5), or (2,300000,5) in float32.
  // Bounds have extents (4+5*6,100000*6,10) = (34,600000,10).
  // Extents divided by spacing give dimensions (17,2,2).
  const auto& output = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  const FloatVec3Type expectedSpacing = {2.0F, 300000.0F, 5.0F};
  const SizeVec3Type expectedDims = {17, 2, 2};
  for(usize axis = 0; axis < 3; ++axis)
  {
    CAPTURE(axis);
    CHECK(output.getSpacing()[axis] == Approx(expectedSpacing[axis]));
    CHECK(output.getDimensions()[axis] == expectedDims[axis]);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz", "apply_transformation_to_geometry.dream3d");

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellDataWithSpaces);

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
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

      // Execute the filter and check the result
      unit_test::AlgorithmTestScope algorithmTestScope(scenario);
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

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ApplyTransformationToGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ApplyTransformationToGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ApplyTransformationToGeometryFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ApplyTransformationToGeometryFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
ImageGeom* MakeValidationImage(DataStructure& dataStructure, const SizeVec3Type& dims = {2, 2, 2}, const FloatVec3Type& spacing = {1, 1, 1}, const FloatVec3Type& origin = {0, 0, 0})
{
  auto* geomPtr = ImageGeom::Create(dataStructure, "ValidationImage");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions(dims);
  REQUIRE(geomPtr->setSpacing(spacing).valid());
  REQUIRE(geomPtr->setOrigin(origin).valid());
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", {dims[2], dims[1], dims[0]}, geomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geomPtr->setCellData(*cellsPtr);
  auto store = data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"ValidationImage", "Cells", "Samples"}), {dims[2], dims[1], dims[0]}, {1});
  REQUIRE(store != nullptr);
  auto* samplesPtr = Int32ArrayType::Create(dataStructure, "Samples", store, cellsPtr->getId());
  REQUIRE(samplesPtr != nullptr);
  for(usize index = 0; index < samplesPtr->getNumberOfTuples(); ++index)
  {
    (*samplesPtr)[index] = static_cast<int32>(index + 1);
  }
  return geomPtr;
}
} // namespace

namespace
{
VertexGeom* MakeValidationVertices(DataStructure& dataStructure)
{
  auto* geomPtr = VertexGeom::Create(dataStructure, "ValidationVertices");
  REQUIRE(geomPtr != nullptr);
  auto* coordsPtr = Float32ArrayType::CreateWithStore<Float32DataStoreType>(dataStructure, "Vertices", {4}, {3}, geomPtr->getId());
  REQUIRE(coordsPtr != nullptr);
  const std::array<float32, 12> values = {1, 2, 3, -1, 0, 2, 2, -3, 1, 0, 1, -2};
  for(usize idx = 0; idx < values.size(); ++idx)
  {
    (*coordsPtr)[idx] = values[idx];
  }
  geomPtr->setVertices(*coordsPtr);
  return geomPtr;
}
Arguments ValidationTransformArgs(usize type, bool image)
{
  ApplyTransformationToGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, DataPath({image ? "ValidationImage" : "ValidationVertices"}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, DataPath({"ValidationImage", "Cells"}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, ChoicesParameter::ValueType{image ? 0ULL : 2ULL});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, static_cast<ChoicesParameter::ValueType>(type));
  // The validation fixtures have no Feature Attribute Matrix.
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key, false);
  return args;
}
} // namespace
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Parameter domain regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8);
  CAPTURE(row);
  const bool image = row < 3 || row == 5;
  DataStructure dataStructure;
  if(image)
  {
    MakeValidationImage(dataStructure);
  }
  else
  {
    MakeValidationVertices(dataStructure);
  }
  const usize type = row < 4 ? 5 : row < 6 ? 4 : 3;
  auto args = ValidationTransformArgs(type, image);
  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  const float32 inf = std::numeric_limits<float32>::infinity();
  const std::vector<VectorFloat32ParameterType::ValueType> values = {{0, 1, 1}, {-1, 1, 1}, {nan, 1, 1}, {nan, 1, 1}, {inf, 0, 0}, {nan, 0, 0}, {0, 0, 0, 90}, {nan, 0, 1, 90}, {0, 0, 1, inf}};
  args.insertOrAssign(type == 5 ? ApplyTransformationToGeometryFilter::k_Scale_Key :
                      type == 4 ? ApplyTransformationToGeometryFilter::k_Translation_Key :
                                  ApplyTransformationToGeometryFilter::k_Rotation_Key,
                      values[row]);
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  const std::array<int32, 9> codes = {-86004, -86004, -86004, -86003, -86002, -86002, -86052, -86051, -86053};
  REQUIRE(result.output_actions.errors()[0].code == codes[row]);
  CHECK(result.output_actions.errors()[0].message.find(type == 5 ? "Scale" : type == 4 ? "Translation" : "Rotation") != std::string::npos);
  CHECK(result.output_actions.errors()[0].message.find(row == 8 ? "angle" : "X") != std::string::npos);
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Result domain regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2);
  CAPTURE(row);
  DataStructure dataStructure;
  auto* geomPtr = MakeValidationImage(dataStructure, {1, 1, 1}, {row == 0 ? 2.0f : row == 1 ? 0.25f : 1.0f, 1, 1}, {row == 2 ? 3e38f : 0, 0, 0});
  const auto oldSpacing = geomPtr->getSpacing();
  const auto oldOrigin = geomPtr->getOrigin();
  auto args = ValidationTransformArgs(row == 2 ? 4 : 5, true);
  args.insertOrAssign(row == 2 ? ApplyTransformationToGeometryFilter::k_Translation_Key : ApplyTransformationToGeometryFilter::k_Scale_Key,
                      VectorFloat32ParameterType::ValueType{row == 1 ? std::numeric_limits<float32>::denorm_min() : 3e38f, row == 2 ? 0.0f : 1.0f, row == 2 ? 0.0f : 1.0f});
  ApplyTransformationToGeometryFilter filter;
  auto pre = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.output_actions);
  REQUIRE_FALSE(pre.output_actions.warnings().empty());
  const auto& preflightWarning = pre.output_actions.warnings().front();
  CHECK(preflightWarning.code == (row == 2 ? -86005 : -86006));
  if(row == 2)
  {
    CHECK(preflightWarning.message.find("Translation' X value 3e+38 with origin 3e+38 produces result inf") != std::string::npos);
  }
  else
  {
    CHECK(preflightWarning.message.find("Scale' X value") != std::string::npos);
    CHECK(preflightWarning.message.find(row == 0 ? "with spacing 2 and origin 0 produces spacing inf" : "with spacing 0.25 and origin 0 produces spacing 0") != std::string::npos);
  }
  const int32 expectedCode = row == 2 ? -1557 : -1556;
  const std::string expectedValue = row == 1 ? "Received 0." : "Received inf.";
  DataStructure preflightDs = dataStructure;
  const auto preflightActionResult = pre.output_actions.value().applyRegular(preflightDs, IDataAction::Mode::Preflight);
  const auto& preflightGeom = preflightDs.getDataRefAs<ImageGeom>(DataPath({"ValidationImage"}));
  CHECK(preflightGeom.getSpacing() == oldSpacing);
  CHECK(preflightGeom.getOrigin() == oldOrigin);
  auto result = filter.execute(dataStructure, args);
  for(const auto* failurePtr : std::array<const Result<>*, 2>{&preflightActionResult, &result.result})
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(*failurePtr);
    REQUIRE(failurePtr->errors().size() == 1);
    CHECK(failurePtr->errors().front().code == expectedCode);
    const auto& message = failurePtr->errors().front().message;
    CHECK(message.find("'ValidationImage'") != std::string::npos);
    CHECK(message.find(row == 2 ? "Origin on the X axis" : "Spacing on the X axis") != std::string::npos);
    CHECK(message.find(expectedValue) != std::string::npos);
  }
  CHECK(geomPtr->getSpacing() == oldSpacing);
  CHECK(geomPtr->getOrigin() == oldOrigin);
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Execute rejects invalid parameters without mutation", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2);
  CAPTURE(row);
  DataStructure dataStructure;
  if(row == 0)
  {
    MakeValidationImage(dataStructure);
  }
  else
  {
    MakeValidationVertices(dataStructure);
  }
  auto args = ValidationTransformArgs(row == 0 ? 5 : row == 1 ? 4 : 3, row == 0);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, VectorFloat32ParameterType::ValueType{0, 1, 1});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, VectorFloat32ParameterType::ValueType{std::numeric_limits<float32>::quiet_NaN(), 0, 0});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32ParameterType::ValueType{0, 0, 0, 90});
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors()[0].code == (row == 0 ? -86004 : row == 1 ? -86002 : -86052));
  if(row != 0)
  {
    auto* geomPtr = dataStructure.getDataAs<VertexGeom>(DataPath({"ValidationVertices"}));
    const std::array<float32, 12> expected = {1, 2, 3, -1, 0, 2, 2, -3, 1, 0, 1, -2};
    for(usize index = 0; index < expected.size(); ++index)
    {
      CHECK(geomPtr->getVerticesRef()[index] == expected[index]);
    }
  }
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Large axis vertex regression", "[SimplnxCore][ApplyTransformationToGeometryFilter][NumericDomainRed]")
{
  const float32 magnitude = GENERATE(1.0f, 1e20f);
  DataStructure dataStructure;
  auto* geomPtr = MakeValidationVertices(dataStructure);
  auto args = ValidationTransformArgs(3, false);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32ParameterType::ValueType{0, 0, magnitude, 90});
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const std::array<float32, 12> expected = {-2, 1, 3, 0, -1, 2, 3, 2, 1, -1, 0, -2};
  for(usize index = 0; index < expected.size(); ++index)
  {
    CHECK(geomPtr->getVerticesRef()[index] == Approx(expected[index]).margin(1e-6));
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Execute rejects vector lengths and image rotation", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto row = GENERATE(0, 1, 2);
  DataStructure dataStructure;
  if(row == 2)
  {
    MakeValidationImage(dataStructure);
  }
  else
  {
    MakeValidationVertices(dataStructure);
  }
  auto args = ValidationTransformArgs(row == 0 ? 4 : row == 1 ? 5 : 3, row == 2);
  if(row == 2)
  {
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32ParameterType::ValueType{0, 0, 0, 90});
  }
  else
  {
    args.insertOrAssign(row == 0 ? ApplyTransformationToGeometryFilter::k_Translation_Key : ApplyTransformationToGeometryFilter::k_Scale_Key, VectorFloat32ParameterType::ValueType{1, 2});
  }
  SizeVec3Type destinationDims{};
  FloatVec3Type destinationOrigin{};
  if(row == 2)
  {
    auto validArgs = ValidationTransformArgs(3, true);
    validArgs.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32ParameterType::ValueType{0, 0, 1, 90});
    ApplyTransformationToGeometryFilter filter;
    auto pre = filter.preflight(dataStructure, validArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(pre.output_actions);
    auto applied = pre.output_actions.value().applyRegular(dataStructure, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(applied);
    auto* destinationPtr = dataStructure.getDataAs<ImageGeom>(DataPath({".ValidationImage"}));
    REQUIRE(destinationPtr != nullptr);
    destinationDims = destinationPtr->getDimensions();
    destinationOrigin = destinationPtr->getOrigin();
    auto* samplesPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({".ValidationImage", "Cells", "Samples"}));
    REQUIRE(samplesPtr != nullptr);
    samplesPtr->fill(99);
    auto invalidArgs = ValidationTransformArgs(3, true);
    for(const auto& values :
        std::vector<VectorFloat32ParameterType::ValueType>{{0, 0, 0, 90}, {std::numeric_limits<float32>::quiet_NaN(), 0, 1, 90}, {0, 0, 1, std::numeric_limits<float32>::infinity()}})
    {
      invalidArgs.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, values);
      auto rejected = filter.preflight(dataStructure, invalidArgs);
      SIMPLNX_RESULT_REQUIRE_INVALID(rejected.output_actions);
      CHECK(rejected.output_actions.errors()[0].code == (values[0] != values[0] ? -86051 : values[3] != 90 ? -86053 : -86052));
    }
  }
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == (row == 2 ? -86052 : -1));
  if(row != 2)
  {
    INFO("Malformed active parameter: " << (row == 0 ? "Translation" : "Scale"));
    CHECK(result.result.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
    auto* geomPtr = dataStructure.getDataAs<VertexGeom>(DataPath({"ValidationVertices"}));
    const std::array<float32, 12> expected = {1, 2, 3, -1, 0, 2, 2, -3, 1, 0, 1, -2};
    for(usize index = 0; index < expected.size(); ++index)
    {
      CHECK(geomPtr->getVerticesRef()[index] == expected[index]);
    }
  }
  else
  {
    CHECK(dataStructure.getDataAs<ImageGeom>(DataPath({"ValidationImage"}))->getOrigin() == FloatVec3Type{0, 0, 0});
    auto* destinationPtr = dataStructure.getDataAs<ImageGeom>(DataPath({".ValidationImage"}));
    REQUIRE(destinationPtr != nullptr);
    CHECK(destinationPtr->getDimensions() == destinationDims);
    CHECK(destinationPtr->getOrigin() == destinationOrigin);
    auto* sourceSamplesPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({"ValidationImage", "Cells", "Samples"}));
    auto* destinationSamplesPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({".ValidationImage", "Cells", "Samples"}));
    REQUIRE(sourceSamplesPtr != nullptr);
    REQUIRE(destinationSamplesPtr != nullptr);
    for(usize index = 0; index < 8; ++index)
    {
      CHECK((*sourceSamplesPtr)[index] == index + 1);
      CHECK((*destinationSamplesPtr)[index] == 99);
    }
  }
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Finite extremes and inactive vectors", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5);
  DataStructure dataStructure;
  const bool image = row < 3 || row == 5;
  ImageGeom* geomPtr = nullptr;
  VertexGeom* verticesPtr = nullptr;
  if(image)
  {
    geomPtr = MakeValidationImage(dataStructure, {1, 1, 1}, {row == 5 ? 0.25f : 1.0f, 1, 1});
  }
  else
  {
    verticesPtr = MakeValidationVertices(dataStructure);
  }
  auto args = ValidationTransformArgs(row == 2 || row == 5 ? 4 : 5, image);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32ParameterType::ValueType{0, 0, 0, 90});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, VectorFloat32ParameterType::ValueType{row == 0 ? std::numeric_limits<float32>::denorm_min() :
                                                                                                              row == 1 ? 1e20f :
                                                                                                              row == 3 ? -1.0f :
                                                                                                                         0.0f,
                                                                                                              1, 1});
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, VectorFloat32ParameterType::ValueType{row == 2 ? 2e38f : 1.0f, 0, 0});
  ApplyTransformationToGeometryFilter filter;
  auto pre = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.output_actions);
  CHECK(std::none_of(pre.output_actions.warnings().begin(), pre.output_actions.warnings().end(), [](const Warning& warning) { return warning.code <= -86000 && warning.code >= -86069; }));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(row == 0)
  {
    CHECK(geomPtr->getSpacing()[0] == std::numeric_limits<float32>::denorm_min());
  }
  if(row == 1)
  {
    CHECK(geomPtr->getSpacing()[0] == 1e20f);
  }
  if(row == 2)
  {
    CHECK(geomPtr->getOrigin()[0] == 2e38f);
  }
  if(row == 3 || row == 4)
  {
    const std::array<float32, 12> reflected = {-1, 2, 3, 1, 0, 2, -2, -3, 1, 0, 1, -2};
    const std::array<float32, 12> collapsed = {0, 2, 3, 0, 0, 2, 0, -3, 1, 0, 1, -2};
    const auto& expected = row == 3 ? reflected : collapsed;
    for(usize index = 0; index < expected.size(); ++index)
    {
      CHECK(verticesPtr->getVerticesRef()[index] == expected[index]);
    }
  }
  if(row == 5)
  {
    CHECK(geomPtr->getSpacing()[0] == 0.25f);
    CHECK(geomPtr->getOrigin()[0] == 1);
  }
}
TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Pipeline stand-in metadata", "[SimplnxCore][ApplyTransformationToGeometryFilter][SetImageGeomOriginScalingFilter]")
{
  const auto row = GENERATE(0, 1);
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope algorithmScope(scenario);
  Pipeline pipeline;
  auto first = ValidationTransformArgs(1, true);
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, DataPath({"Matrix"}));
  REQUIRE(pipeline.push_back(std::make_unique<ApplyTransformationToGeometryFilter>(), first));
  if(row == 0)
  {
    auto second = ValidationTransformArgs(5, true);
    second.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, VectorFloat32ParameterType::ValueType{3e38f, 1, 1});
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
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, VectorFloat32ParameterType::ValueType{3e38f, 1, 1});
    second.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, VectorFloat32ParameterType::ValueType{0, 0, 0});
    REQUIRE(pipeline.push_back(std::make_unique<SetImageGeomOriginScalingFilter>(), second));
  }
  for(bool execute : {false, true})
  {
    DataStructure dataStructure;
    MakeValidationImage(dataStructure, row == 0 ? SizeVec3Type{1, 1, 1} : SizeVec3Type{10, 1, 1}, row == 0 ? FloatVec3Type{2, 1, 1} : FloatVec3Type{1, 1, 1});
    auto* matrixPtr = Float32ArrayType::CreateWithStore<Float32DataStoreType>(dataStructure, "Matrix", {1}, {16});
    REQUIRE(matrixPtr != nullptr);
    const std::array<float32, 16> values =
        row == 0 ? std::array<float32, 16>{0.5f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} : std::array<float32, 16>{0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    for(usize idx = 0; idx < 16; ++idx)
    {
      (*matrixPtr)[idx] = values[idx];
    }
    if(!execute)
    {
      REQUIRE_FALSE(pipeline.preflight(dataStructure, false));
      const auto* downstreamPtr = dynamic_cast<const PipelineFilter*>(pipeline.at(1));
      REQUIRE(downstreamPtr != nullptr);
      const auto errors = downstreamPtr->getErrors();
      REQUIRE(errors.size() == 1);
      CHECK(errors.front().code == (row == 0 ? -1556 : -1557));
      CHECK(errors.front().message.find("'ValidationImage'") != std::string::npos);
    }
    else
    {
      REQUIRE(algorithmScope.execute([&] { return pipeline.execute(dataStructure, false); }));
      auto* geomPtr = dataStructure.getDataAs<ImageGeom>(DataPath({"ValidationImage"}));
      REQUIRE(geomPtr != nullptr);
      if(row == 0)
      {
        CHECK(geomPtr->getSpacing()[0] == 3e38f);
      }
      else
      {
        CHECK(geomPtr->getDimensions() == SizeVec3Type{1, 10, 1});
        CHECK(geomPtr->getOrigin()[0] == Approx(-1.5e38f));
        CHECK(geomPtr->getOrigin()[1] == -5);
        CHECK(geomPtr->getOrigin()[2] == -0.5f);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Principal rotation controls", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  MakeValidationImage(dataStructure);
  auto args = ValidationTransformArgs(3, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, VectorFloat32ParameterType::ValueType{0, 0, 1, 90});
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  auto* geomPtr = dataStructure.getDataAs<ImageGeom>(DataPath({"ValidationImage"}));
  REQUIRE(geomPtr != nullptr);
  CHECK(geomPtr->getDimensions() == SizeVec3Type{2, 2, 2});
  auto* samplesPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({"ValidationImage", "Cells", "Samples"}));
  REQUIRE(samplesPtr != nullptr);
  const std::array<int32, 8> expected = {3, 1, 4, 2, 7, 5, 8, 6};
  for(usize index = 0; index < expected.size(); ++index)
  {
    CHECK((*samplesPtr)[index] == expected[index]);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Selected vector preflight validation", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
  DataStructure dataStructure;
  MakeValidationImage(dataStructure);
  MakeValidationVertices(dataStructure);
  ApplyTransformationToGeometryInputValues input{};
  input.selected_geometry_path = DataPath({row == 6 || row == 7 ? "ValidationVertices" : "ValidationImage"});
  input.transformation_selection = row < 3 ? 4 : row < 8 ? 5 : row < 11 ? 3 : 0;
  input.translation = row == 0 ? std::vector<float32>{1, 2} : row == 1 ? std::vector<float32>{std::numeric_limits<float32>::infinity(), 0, 0} : std::vector<float32>{1, 2, 3};
  input.scale = row == 3             ? std::vector<float32>{1, 2} :
                row == 4             ? std::vector<float32>{0, 1, 1} :
                row == 5 || row == 6 ? std::vector<float32>{std::numeric_limits<float32>::quiet_NaN(), 1, 1} :
                row == 7             ? std::vector<float32>{-1, 0, 1} :
                                       std::vector<float32>{1, 2, 3};
  input.rotation = row == 8 ? std::vector<float32>{0, 0, 1} : row == 9 ? std::vector<float32>{0, 0, 0, 90} : std::vector<float32>{0, 0, 2, 90};
  if(row == 12)
  {
    input.transformation_selection = 1;
  }
  if(row == 13)
  {
    input.transformation_selection = 2;
  }
  if(row == 14)
  {
    input.transformation_selection = 5;
    input.scale = {1, 2, 3};
  }
  auto args = ValidationTransformArgs(input.transformation_selection, row != 6 && row != 7);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, input.translation);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, input.scale);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, input.rotation);
  if(row == 12)
  {
    auto* matrixPtr = Float32ArrayType::CreateWithStore<Float32DataStoreType>(dataStructure, "Matrix", {16}, {1});
    REQUIRE(matrixPtr != nullptr);
    matrixPtr->fill(0);
    for(usize index = 0; index < 16; index += 5)
    {
      (*matrixPtr)[index] = 1;
    }
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, DataPath({"Matrix"}));
  }
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(dataStructure, args).output_actions;
  const std::array<int32, 15> codes = {-1, -86002, 0, -1, -86004, -86004, -86003, 0, -1, -86052, 0, 0, 0, 0, 0};
  if(codes[row] == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == codes[row]);
    if(codes[row] == -1)
    {
      INFO("Malformed active parameter: " << (row == 0 ? "Translation" : row == 3 ? "Scale" : "Rotation Axis-Angle"));
      CHECK(result.errors()[0].message == (row == 8 ? "VectorParameter requires size 4. Received size 3." : "VectorParameter requires size 3. Received size 2."));
    }
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometry: Preflight accepts one malformed inactive vector", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5);
  CAPTURE(row);
  DataStructure dataStructure;
  MakeValidationVertices(dataStructure);
  auto args = ValidationTransformArgs(row < 2 ? 4 : row < 4 ? 5 : 3, false);
  const auto malformedKey = row == 0 || row == 5 ? ApplyTransformationToGeometryFilter::k_Scale_Key :
                            row == 1 || row == 3 ? ApplyTransformationToGeometryFilter::k_Rotation_Key :
                                                   ApplyTransformationToGeometryFilter::k_Translation_Key;
  args.insertOrAssign(malformedKey, VectorFloat32ParameterType::ValueType{});
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
}

namespace
{
const DataPath k_FeatureGeomPath({"Geometry"});
const DataPath k_FeatureAMPath({"Geometry", "Feature Data"});
const DataPath k_ElementAMPath({"Geometry", "Element Data"});
const DataPath k_TransformFeatureIdsPath({"Geometry", "Element Data", "FeatureIds"});

class FailOnRenumberReadStore : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;

  ~FailOnRenumberReadStore() noexcept override = default;
  FailOnRenumberReadStore(const FailOnRenumberReadStore&) = delete;
  FailOnRenumberReadStore(FailOnRenumberReadStore&&) noexcept = delete;
  FailOnRenumberReadStore& operator=(const FailOnRenumberReadStore&) = delete;
  FailOnRenumberReadStore& operator=(FailOnRenumberReadStore&&) noexcept = delete;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    // Let the source validation read succeed, then fail the post-transform read.
    if(++m_ReadCount == 2)
    {
      return MakeErrorResult(-95102, "Injected post-transform Feature Ids read failure.");
    }
    return DataStore<int32>::copyIntoBuffer(startIndex, buffer);
  }

private:
  mutable usize m_ReadCount = 0;
};

// Three cells or vertices carry sparse IDs 1, 3, 5. Feature data has a non-flat shape.
DataStructure CreateTransformFeatureFixture(bool node = false)
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  IGeometry* geomPtr = nullptr;
  if(node)
  {
    auto* verticesPtr = VertexGeom::Create(dataStructure, "Geometry");
    REQUIRE(verticesPtr != nullptr);
    auto* coordsPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Vertices", {3}, {3}, verticesPtr->getId());
    REQUIRE(coordsPtr != nullptr);
    coordsPtr->fill(0);
    (*coordsPtr)[3] = 1;
    (*coordsPtr)[6] = 2;
    verticesPtr->setVertices(*coordsPtr);
    geomPtr = verticesPtr;
  }
  else
  {
    auto* imagePtr = ImageGeom::Create(dataStructure, "Geometry");
    REQUIRE(imagePtr != nullptr);
    imagePtr->setDimensions({3, 1, 1});
    REQUIRE(imagePtr->setOrigin({0, 0, 0}).valid());
    REQUIRE(imagePtr->setSpacing({1, 1, 1}).valid());
    geomPtr = imagePtr;
  }
  auto* elementsPtr = AttributeMatrix::Create(dataStructure, "Element Data", node ? ShapeType{3} : ShapeType{1, 1, 3}, geomPtr->getId());
  REQUIRE(elementsPtr != nullptr);
  if(node)
  {
    dynamic_cast<VertexGeom*>(geomPtr)->setVertexAttributeMatrix(*elementsPtr);
  }
  else
  {
    dynamic_cast<ImageGeom*>(geomPtr)->setCellData(*elementsPtr);
  }
  auto* idsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "FeatureIds", elementsPtr->getShape(), {1}, elementsPtr->getId());
  REQUIRE(idsPtr != nullptr);
  (*idsPtr)[0] = 1;
  (*idsPtr)[1] = 3;
  (*idsPtr)[2] = 5;
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2, 3}, geomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  auto* valuesPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Values", featuresPtr->getShape(), {1}, featuresPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  for(usize i = 0; i < 6; ++i)
  {
    (*valuesPtr)[i] = static_cast<int32>(10 + i);
  }
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{2, 3}, {"zero", "one", "two", "three", "four", "five"}, featuresPtr->getId()) != nullptr);
  auto* neighborsPtr = NeighborList<int32>::Create(dataStructure, "Neighbors", ShapeType{2, 3}, featuresPtr->getId());
  REQUIRE(neighborsPtr != nullptr);
  neighborsPtr->setList(1, std::vector<int32>{3});
  neighborsPtr->setList(3, std::vector<int32>{1, 5});
  neighborsPtr->setList(5, std::vector<int32>{3});
  auto* ensemblePtr = AttributeMatrix::Create(dataStructure, "Ensemble", ShapeType{2}, geomPtr->getId());
  REQUIRE(ensemblePtr != nullptr);
  auto* phasesPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Phases", {2}, {1}, ensemblePtr->getId());
  REQUIRE(phasesPtr != nullptr);
  (*phasesPtr)[0] = 42;
  (*phasesPtr)[1] = 99;
  return dataStructure;
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
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{0, 0, 1, 90}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{2, 3, 4}));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{2, 3, 4}));
  return args;
}

void CheckTransformFeatureData(const DataStructure& dataStructure, bool clear, const ShapeType& shape)
{
  const auto* featuresPtr = dataStructure.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(featuresPtr != nullptr);
  CHECK(featuresPtr->getShape() == shape);
  CHECK(featuresPtr->getSize() == (clear ? 0 : 3));
  if(!clear)
  {
    const auto* valuesPtr = dataStructure.getDataAs<Int32ArrayType>(k_FeatureAMPath.createChildPath("Values"));
    REQUIRE(valuesPtr != nullptr);
    for(usize i = 0; i < 6; ++i)
    {
      CHECK((*valuesPtr)[i] == static_cast<int32>(10 + i));
    }
    const auto* namesPtr = dataStructure.getDataAs<StringArray>(k_FeatureAMPath.createChildPath("Names"));
    REQUIRE(namesPtr != nullptr);
    const std::array<std::string, 6> expectedNames = {"zero", "one", "two", "three", "four", "five"};
    for(usize i = 0; i < expectedNames.size(); ++i)
    {
      CHECK((*namesPtr)[i] == expectedNames[i]);
    }
    const auto* neighborsPtr = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath("Neighbors"));
    REQUIRE(neighborsPtr != nullptr);
    CHECK(neighborsPtr->getList(0).empty());
    CHECK(neighborsPtr->getList(1) == std::vector<int32>{3});
    CHECK(neighborsPtr->getList(2).empty());
    CHECK(neighborsPtr->getList(3) == std::vector<int32>{1, 5});
    CHECK(neighborsPtr->getList(4).empty());
    CHECK(neighborsPtr->getList(5) == std::vector<int32>{3});
  }
  const auto* phasesPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({"Geometry", "Ensemble", "Phases"}));
  REQUIRE(phasesPtr != nullptr);
  REQUIRE(phasesPtr->getNumberOfTuples() == 2);
  CHECK((*phasesPtr)[0] == 42);
  CHECK((*phasesPtr)[1] == 99);
  CHECK(dataStructure.getData(DataPath({".Geometry"})) == nullptr);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM replacement", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const bool clear = GENERATE(true, false);
  const uint64 transform = GENERATE(1, 2, 3);
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto args = MakeTransformFeatureArgs(clear, false, transform);
  // Manual and precomputed matrices rotate 90 degrees about Z, as does the axis-angle default.
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key,
                      std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{0, -1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
  auto* matricesPtr = DataGroup::Create(ds, "Matrices");
  REQUIRE(matricesPtr != nullptr);
  auto* matrixPtr = unit_test::CreateTestDataArray<float32>(ds, "Matrix", {16}, {1}, matricesPtr->getId());
  REQUIRE(matrixPtr != nullptr);
  const std::array<float32, 16> entries = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  for(usize i = 0; i < entries.size(); ++i)
  {
    (*matrixPtr)[i] = entries[i];
  }
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(DataPath({"Matrices", "Matrix"})));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, clear, {2, 3});
  const auto* idsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(idsPtr->getNumberOfTuples() == 3);
  CHECK((*idsPtr)[0] == 1);
  CHECK((*idsPtr)[1] == 3);
  CHECK((*idsPtr)[2] == 5);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM in place", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
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
  const auto* idsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(idsPtr != nullptr);
  CHECK((*idsPtr)[0] == 1);
  CHECK((*idsPtr)[1] == (clear && renumber ? 2 : 3));
  CHECK((*idsPtr)[2] == (clear && renumber ? 3 : 5));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Renumber read failure preserves in-place Feature AM", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  auto ds = CreateTransformFeatureFixture();
  auto& ids = ds.getDataRefAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  auto store = std::make_shared<FailOnRenumberReadStore>(ids.getTupleShape(), ShapeType{1}, int32{0});
  const std::array<int32, 3> inputIds = {1, 3, 5};
  const auto writeResult = store->copyFromBuffer(0, inputIds);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  const auto setStoreResult = ids.setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);
  const auto featureAmId = ds.getDataRefAs<AttributeMatrix>(k_FeatureAMPath).getId();

  // An image translation stays in place and does not dispatch a resampling algorithm.
  const ApplyTransformationToGeometryFilter filter;
  const auto result = filter.execute(ds, MakeTransformFeatureArgs(true, true, 4));

  const auto& image = ds.getDataRefAs<ImageGeom>(k_FeatureGeomPath);
  CHECK(image.getOrigin() == FloatVec3Type{2, 3, 4});
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors().front().code == -95102);
  CHECK(result.result.errors().front().message == "Injected post-transform Feature Ids read failure.");
  CHECK(ds.getDataRefAs<AttributeMatrix>(k_FeatureAMPath).getId() == featureAmId);
  CheckTransformFeatureData(ds, false, {2, 3});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM rotation drops feature", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const bool renumber = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto args = MakeTransformFeatureArgs(true, renumber);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{0, 0, 1, 45}));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  // With c=sqrt(1/2), the rotated box has origin (-c,0,0) and a 3x3x1 grid.
  // Inverse centers are ((i+j+1)*c-1/2, (j-i)*c+1/2, 1/2).
  // Only (i,j)=(0,0),(1,1) lie inside the source: x=0.207,1.621, y=0.5.
  // Thus source cell 2 (ID 5) disappears. Background stays 0; ID 3 becomes 2.
  const std::vector<int32> expected = {1, 0, 0, 0, renumber ? 2 : 3, 0, 0, 0, 0};
  const auto* idsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(idsPtr->getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    CHECK((*idsPtr)[i] == expected[i]);
  }
  CheckTransformFeatureData(ds, true, renumber ? ShapeType{3} : ShapeType{2, 3});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Preflight warns for unchanged Attribute Matrices", "[SimplnxCore][ApplyTransformationToGeometryFilter][UnchangedAttributeMatrices]")
{
  const bool node = GENERATE(false, true);
  const auto transform = node ? 4 : GENERATE(3, 4, 5);
  auto ds = CreateTransformFeatureFixture(node);
  const DataPath ensemblePath({"Geometry", "Ensemble"});
  const ApplyTransformationToGeometryFilter filter;

  SECTION("Feature clearing leaves Ensemble data")
  {
    auto result = filter.preflight(ds, MakeTransformFeatureArgs(true, false, transform, node));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    REQUIRE(result.output_actions.warnings().size() == 1);
    const auto& warning = result.output_actions.warnings().front();
    CHECK(warning.code == -4041);
    CHECK(warning.message.find(transform == 3 ? "copied to the transformed geometry" : "kept in the transformed geometry") != std::string::npos);
    CHECK(warning.message.find("\nGeometry/Ensemble\n") != std::string::npos);
    CHECK(warning.message.find("\nGeometry/Feature Data\n") == std::string::npos);
    CHECK(warning.message.find("\nGeometry/Element Data\n") == std::string::npos);
  }

  SECTION("Only the registered element data remains")
  {
    REQUIRE(ds.removeData(k_FeatureAMPath));
    REQUIRE(ds.removeData(ensemblePath));
    auto result = filter.preflight(ds, MakeTransformFeatureArgs(false, false, transform, node));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    CHECK(result.output_actions.warnings().empty());
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM preflight", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  auto ds = CreateTransformFeatureFixture();
  ApplyTransformationToGeometryFilter filter;

  SECTION("Feature AM information for replacement geometry")
  {
    const bool clear = GENERATE(true, false);
    auto result = filter.preflight(ds, MakeTransformFeatureArgs(clear, false));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    const auto it =
        std::find_if(result.output_values.begin(), result.output_values.end(), [clear](const auto& value) { return value.name == (clear ? "Cleared Feature Data" : "Feature Data Not Cleared"); });
    REQUIRE(it != result.output_values.end());
    if(clear)
    {
      CHECK(it->value.find("Geometry/Feature Data/Values") != std::string::npos);
      CHECK(it->value.find("Geometry/Feature Data/Names") != std::string::npos);
      CHECK(it->value.find("Geometry/Feature Data/Neighbors") != std::string::npos);
      CHECK(it->value.find("Ensemble") == std::string::npos);
    }
  }

  SECTION("Modified objects for in-place translation")
  {
    const bool renumber = GENERATE(true, false);
    CAPTURE(renumber);
    const auto result = filter.preflight(ds, MakeTransformFeatureArgs(true, renumber, 4));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);

    usize featureIdsModifications = 0;
    usize featureAmModifications = 0;
    for(const auto& modification : result.output_actions.value().modified_actions)
    {
      CAPTURE(modification.modified_path.toString());
      CHECK(modification.modified_path.getParent() != k_FeatureAMPath);
      if(modification.modified_path == k_TransformFeatureIdsPath)
      {
        ++featureIdsModifications;
        CHECK(modification.data_object_type == DataObject::Type::DataArray);
        CHECK(modification.modified_type == DataObjectModification::ModifiedType::Modified);
      }
      else if(modification.modified_path == k_FeatureAMPath)
      {
        ++featureAmModifications;
        CHECK(modification.data_object_type == DataObject::Type::AttributeMatrix);
        CHECK(modification.modified_type == DataObjectModification::ModifiedType::Modified);
      }
    }

    const usize expectedModifications = renumber ? 1 : 0;
    CHECK(featureIdsModifications == expectedModifications);
    CHECK(featureAmModifications == expectedModifications);
  }

  CHECK(filter.parametersVersion() == 3);
  CHECK(filter.getDefaultArguments().value<bool>(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key));
  CHECK_FALSE(filter.getDefaultArguments().value<bool>(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM errors", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
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
    auto* outsidePtr = AttributeMatrix::Create(ds, "Outside", ShapeType{3});
    REQUIRE(outsidePtr != nullptr);
    REQUIRE(unit_test::CreateTestDataArray<int32>(ds, "Ids", {3}, {1}, outsidePtr->getId()) != nullptr);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Outside", "Ids"})));
    expectedCode = -67453;
  }
  SECTION("Feature IDs tuple count")
  {
    auto* elementsPtr = ds.getDataAs<AttributeMatrix>(k_ElementAMPath);
    REQUIRE(elementsPtr != nullptr);
    auto resize = elementsPtr->resizeTuples({2});
    SIMPLNX_RESULT_REQUIRE_VALID(resize);
    expectedCode = -67454;
  }
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors().size() == 1);
  CHECK(result.output_actions.errors()[0].code == expectedCode);
  if(expectedCode == -67451)
  {
    CHECK(result.output_actions.errors()[0].message.find("Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.") != std::string::npos);
  }
  unit_test::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM shared parent", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto* groupPtr = DataGroup::Create(ds, "Alias");
  REQUIRE(groupPtr != nullptr);
  REQUIRE(ds.setAdditionalParent(ds.getData(k_FeatureAMPath)->getId(), groupPtr->getId()));
  auto args = MakeTransformFeatureArgs(true, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Alias", "Feature Data"})));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM validates dropped source ID", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const int32 invalidId = GENERATE(-1, 6);
  auto ds = CreateTransformFeatureFixture();
  auto* idsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(idsPtr != nullptr);
  (*idsPtr)[2] = invalidId;
  auto args = MakeTransformFeatureArgs(true, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{0, 0, 1, 45}));
  ApplyTransformationToGeometryFilter filter;
  // Source validation must reject even the cell that resampling drops, before dispatch.
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == (invalidId < 0 ? -5355 : -5351));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM node rotation", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  auto ds = CreateTransformFeatureFixture(true);
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.execute(ds, MakeTransformFeatureArgs(true, true, 3, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
  const auto* coordsPtr = ds.getDataAs<Float32ArrayType>(DataPath({"Geometry", "Vertices"}));
  REQUIRE(coordsPtr != nullptr);
  CHECK((*coordsPtr)[3] == Approx(0).margin(1e-6));
  CHECK((*coordsPtr)[4] == Approx(1));
  CHECK((*coordsPtr)[6] == Approx(0).margin(1e-6));
  CHECK((*coordsPtr)[7] == Approx(2));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM nested alias", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto* nestedPtr = DataGroup::Create(ds, "Misc", ds.getData(k_FeatureGeomPath)->getId());
  REQUIRE(nestedPtr != nullptr);
  auto* keepPtr = unit_test::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nestedPtr->getId());
  REQUIRE(keepPtr != nullptr);
  (*keepPtr)[0] = 123;
  REQUIRE(ds.setAdditionalParent(ds.getData(k_FeatureAMPath)->getId(), nestedPtr->getId()));
  auto* outsidePtr = DataGroup::Create(ds, "Outside");
  REQUIRE(outsidePtr != nullptr);
  REQUIRE(ds.setAdditionalParent(nestedPtr->getId(), outsidePtr->getId()));
  auto args = MakeTransformFeatureArgs(true, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "Misc", "Feature Data"})));
  ApplyTransformationToGeometryFilter filter;
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
  CHECK(ds.getData(DataPath({"Geometry", "Misc", "Feature Data"})) == nullptr);
  const auto* copiedPtr = ds.getDataAs<Int32ArrayType>(DataPath({"Geometry", "Misc", "Keep"}));
  REQUIRE(copiedPtr != nullptr);
  CHECK((*copiedPtr)[0] == 123);
  const auto* originalPtr = ds.getDataAs<AttributeMatrix>(DataPath({"Outside", "Misc", "Feature Data"}));
  REQUIRE(originalPtr != nullptr);
  CHECK(originalPtr->getShape() == ShapeType{2, 3});
  CHECK(originalPtr->getSize() == 3);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Copy actions through a linked root placement", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  auto ds = CreateTransformFeatureFixture();
  const auto* geometryPtr = ds.getData(k_FeatureGeomPath);
  auto* outsidePtr = DataGroup::Create(ds, "Outside");
  auto* nestedPtr = DataGroup::Create(ds, "Misc", geometryPtr->getId());
  REQUIRE(outsidePtr != nullptr);
  REQUIRE(nestedPtr != nullptr);
  REQUIRE(ds.setAdditionalParent(geometryPtr->getId(), outsidePtr->getId()));
  const auto featureId = ds.getData(k_FeatureAMPath)->getId();
  REQUIRE(ds.setAdditionalParent(featureId, nestedPtr->getId()));
  const auto parents = ds.getData(k_FeatureAMPath)->getParentIds();

  ApplyTransformationToGeometryFilter filter;
  const auto preflight = filter.preflight(ds, MakeTransformFeatureArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto result = preflight.output_actions.value().applyRegular(ds, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* replacementPtr = ds.getDataAs<AttributeMatrix>(DataPath({".Geometry", "Feature Data"}));
  REQUIRE(replacementPtr != nullptr);
  CHECK(replacementPtr->getId() != featureId);
  CHECK(replacementPtr->getSize() == 0);
  CHECK(ds.getData(DataPath({".Geometry", "Misc"})) != nullptr);
  CHECK(ds.getData(DataPath({".Geometry", "Misc", "Feature Data"})) == nullptr);
  const auto* sourceFeaturePtr = ds.getData(k_FeatureAMPath);
  REQUIRE(sourceFeaturePtr != nullptr);
  CHECK(sourceFeaturePtr->getParentIds() == parents);
  for(const auto& placement :
      {k_FeatureAMPath, DataPath({"Geometry", "Misc", "Feature Data"}), DataPath({"Outside", "Geometry", "Feature Data"}), DataPath({"Outside", "Geometry", "Misc", "Feature Data"})})
  {
    CHECK(ds.getId(placement) == featureId);
    const auto* valuesPtr = ds.getDataAs<Int32ArrayType>(placement.createChildPath("Values"));
    REQUIRE(valuesPtr != nullptr);
    CHECK((*valuesPtr)[5] == 15);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM shared input survives", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const bool renumber = GENERATE(false, true);
  auto ds = CreateTransformFeatureFixture(true);
  auto* featuresPtr = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(featuresPtr != nullptr);
  auto resized = featuresPtr->resizeTuples({3});
  SIMPLNX_RESULT_REQUIRE_VALID(resized);
  REQUIRE(ds.setAdditionalParent(ds.getData(k_TransformFeatureIdsPath)->getId(), featuresPtr->getId()));
  REQUIRE(ds.setAdditionalParent(ds.getData(DataPath({"Geometry", "Vertices"}))->getId(), featuresPtr->getId()));
  auto* sourceIdsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(sourceIdsPtr != nullptr);
  (*sourceIdsPtr)[0] = 0;
  (*sourceIdsPtr)[1] = 2;
  (*sourceIdsPtr)[2] = 2;
  ApplyTransformationToGeometryFilter filter;
  auto args = MakeTransformFeatureArgs(true, renumber, 3, true);
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  DataStructure staged(ds);
  auto actions = preflight.output_actions.value().applyRegular(staged, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actions);
  REQUIRE(staged.getData(k_TransformFeatureIdsPath) != nullptr);
  REQUIRE(staged.getData(DataPath({"Geometry", "Vertices"})) != nullptr);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, renumber ? ShapeType{2} : ShapeType{3});
  const auto* idsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(idsPtr != nullptr);
  CHECK((*idsPtr)[2] == (renumber ? 1 : 2));
  const auto* coordsPtr = ds.getDataAs<Float32ArrayType>(DataPath({"Geometry", "Vertices"}));
  REQUIRE(coordsPtr != nullptr);
  CHECK((*coordsPtr)[7] == Approx(2));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Feature AM precomputed input survives", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  auto ds = CreateTransformFeatureFixture(true);
  auto* featuresPtr = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(featuresPtr != nullptr);
  auto resized = featuresPtr->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resized);
  auto* matrixPtr = unit_test::CreateTestDataArray<float32>(ds, "Matrix", {4}, {4}, featuresPtr->getId());
  REQUIRE(matrixPtr != nullptr);
  const std::array<float32, 16> entries = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  for(usize i = 0; i < entries.size(); ++i)
  {
    (*matrixPtr)[i] = entries[i];
  }
  auto args = MakeTransformFeatureArgs(true, false, 1, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_FeatureAMPath.createChildPath("Matrix")));
  ApplyTransformationToGeometryFilter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  DataStructure staged(ds);
  auto actions = preflight.output_actions.value().applyRegular(staged, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actions);
  REQUIRE(staged.getData(k_FeatureAMPath.createChildPath("Matrix")) != nullptr);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckTransformFeatureData(ds, true, {4});
  const auto* coordsPtr = ds.getDataAs<Float32ArrayType>(DataPath({"Geometry", "Vertices"}));
  REQUIRE(coordsPtr != nullptr);
  CHECK((*coordsPtr)[7] == Approx(2));
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: No Transform keeps non-interpolated cell arrays", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const bool clear = GENERATE(true, false);
  auto ds = CreateTransformFeatureFixture();
  auto* maskPtr = unit_test::CreateTestDataArray<bool>(ds, "Mask", {1, 1, 3}, {1}, ds.getData(k_ElementAMPath)->getId());
  REQUIRE(maskPtr != nullptr);
  (*maskPtr)[1] = true;
  auto args = MakeTransformFeatureArgs(clear, false, 0);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  ApplyTransformationToGeometryFilter filter;
  // No Transform keeps the geometry in place, so Linear interpolation drops no cell array.
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  for(const auto& warning : preflight.output_actions.warnings())
  {
    CHECK(warning.code != 82010);
  }
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* keptMaskPtr = ds.getDataAs<BoolArrayType>(k_ElementAMPath.createChildPath("Mask"));
  REQUIRE(keptMaskPtr != nullptr);
  CHECK((*keptMaskPtr)[1]);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: StrictFeatureIds preflight", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM][StrictFeatureIds]")
{
  unit_test::LoadPlugins();
  auto ds = CreateTransformFeatureFixture(true);
  // A direct child AM with one tuple per vertex that is not the registered Vertex AM.
  auto* otherDataPtr = AttributeMatrix::Create(ds, "Other Data", ShapeType{3}, ds.getData(k_FeatureGeomPath)->getId());
  REQUIRE(otherDataPtr != nullptr);
  auto* misplacedIdsPtr = unit_test::CreateTestDataArray<int32>(ds, "MisplacedFeatureIds", {3}, {1}, otherDataPtr->getId());
  REQUIRE(misplacedIdsPtr != nullptr);
  misplacedIdsPtr->fill(1);

  ApplyTransformationToGeometryFilter filter;
  auto args = MakeTransformFeatureArgs(true, true, 4, true);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "Other Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    CHECK(result.output_actions.errors().front().code == -67453);
    CHECK(result.output_actions.errors().front().message ==
          "**Feature Ids** DataArray 'Geometry/Other Data/MisplacedFeatureIds' must be in a Vertex, Edge, Face, or Polyhedron Attribute Matrix of the selected geometry 'Geometry'. "
          "Select an array from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Vertex Attribute Matrix")
  {
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_TransformFeatureIdsPath));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  unit_test::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Linear interpolation preserves source labels", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  auto ds = CreateTransformFeatureFixture();
  auto* elementsPtr = ds.getDataAs<AttributeMatrix>(k_ElementAMPath);
  REQUIRE(elementsPtr != nullptr);
  auto* signalPtr = unit_test::CreateTestDataArray<float32>(ds, "Signal", {1, 1, 3}, {1}, elementsPtr->getId());
  REQUIRE(signalPtr != nullptr);
  (*signalPtr)[0] = 0;
  (*signalPtr)[1] = 20;
  (*signalPtr)[2] = 40;
  auto* sourceIdsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(sourceIdsPtr != nullptr);
  (*sourceIdsPtr)[2] = 3;
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
  const auto* idsPtr = ds.getDataAs<Int32ArrayType>(k_TransformFeatureIdsPath);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(idsPtr->getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    CHECK((*idsPtr)[i] == expected[i]);
  }
  const auto* outputSignalPtr = ds.getDataAs<Float32ArrayType>(k_ElementAMPath.createChildPath("Signal"));
  REQUIRE(outputSignalPtr != nullptr);
  CHECK((*outputSignalPtr)[1] == Approx(10.625));
  CHECK((*outputSignalPtr)[2] == Approx(30.625));
  CheckTransformFeatureData(ds, true, {3});
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Reject nonnumeric Feature Ids", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const bool strings = GENERATE(true, false);
  auto ds = CreateTransformFeatureFixture();
  const auto idsPath = k_TransformFeatureIdsPath;
  const auto* originalPtr = ds.getDataAs<Int32ArrayType>(idsPath);
  REQUIRE(originalPtr != nullptr);
  const auto shape = originalPtr->getTupleShape();
  const auto tuples = originalPtr->getNumberOfTuples();
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
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors().size() == 1);
  CHECK(result.output_actions.errors()[0].code == filter_parameter::constants::k_Validate_ArrayType_Error);
  CHECK(result.output_actions.errors()[0].message.find(idsPath.toString()) != std::string::npos);
  CHECK(result.output_actions.errors()[0].message.find("Select an array of one of these kinds: DataArray.") != std::string::npos);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Reject matrix output in cleared Feature AM", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  const bool alias = GENERATE(false, true);
  auto ds = CreateTransformFeatureFixture();
  auto* featuresPtr = ds.getDataAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(featuresPtr != nullptr);
  auto resizeResult = featuresPtr->resizeTuples({16});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  DataPath parentPath = k_FeatureAMPath;
  if(alias)
  {
    auto* otherPtr = DataGroup::Create(ds, "Alias");
    REQUIRE(otherPtr != nullptr);
    REQUIRE(ds.setAdditionalParent(featuresPtr->getId(), otherPtr->getId()));
    parentPath = DataPath({"Alias", "Feature Data"});
  }
  auto args = MakeTransformFeatureArgs(true, false, 4);
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SaveTransformMatrix_Key, std::make_any<bool>(true));
  const auto outputPath = parentPath.createChildPath("Matrix");
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformMatrixOutputPath_Key, std::make_any<DataPath>(outputPath));
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors().size() == 1);
  CHECK(result.output_actions.errors()[0].code == -67456);
  CHECK(result.output_actions.errors()[0].message.find(outputPath.toString()) != std::string::npos);
  CHECK(ds.getData(outputPath) == nullptr);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter: Reject another geometry element Feature AM", "[SimplnxCore][ApplyTransformationToGeometryFilter][ClearFeatureAM]")
{
  unit_test::LoadPlugins();
  auto ds = CreateTransformFeatureFixture();
  const auto featurePath = k_FeatureAMPath;
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(otherPtr != nullptr);
  auto* verticesPtr = unit_test::CreateTestDataArray<float32>(ds, "Vertices", {featuresPtr->getNumberOfTuples()}, {3}, otherPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  otherPtr->setVertices(*verticesPtr);
  REQUIRE(ds.setAdditionalParent(featuresPtr->getId(), otherPtr->getId()));
  otherPtr->setVertexAttributeMatrix(*featuresPtr);
  ApplyTransformationToGeometryFilter filter;
  auto result = filter.preflight(ds, MakeTransformFeatureArgs(true, false, 4));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors().size() == 1);
  CHECK(result.output_actions.errors()[0].code == -67460);
  CHECK(result.output_actions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.output_actions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(otherPtr->getVertexAttributeMatrix() == featuresPtr);
  CHECK(featuresPtr->getSize() > 0);
}
