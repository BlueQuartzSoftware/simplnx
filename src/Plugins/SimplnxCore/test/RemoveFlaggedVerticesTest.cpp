#include "SimplnxCore/Filters/RemoveFlaggedVerticesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_elements_data/remove_flagged_vertices_data.dream3d", nx::core::unit_test::k_TestFilesDir));

constexpr StringLiteral k_CopyTestName = "copy_test";
constexpr StringLiteral k_DataName = "data";
constexpr StringLiteral k_VertexListName = "SharedVertexList";

const DataPath k_VertexGeomPath({"VertexGeometry"});
const DataPath k_MaskPath = k_VertexGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(Constants::k_Mask);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});
const DataPath k_ExemplarReducedGeomPath({"ExemplarReducedGeometry"});

/**
 * @brief Creates four features with two vertices each and removes features 1 and 2.
 * @return Vertex geometry with independent Feature and Ensemble data.
 */
DataStructure CreateFeatureRemovalFixture()
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* vertexGeom = VertexGeom::Create(dataStructure, "Vertex");
  REQUIRE(vertexGeom != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, VertexGeom::k_SharedVertexListName, {8}, {3}, vertexGeom->getId());
  REQUIRE(vertices != nullptr);
  vertexGeom->setVertices(*vertices);
  auto* vertexData = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{8}, vertexGeom->getId());
  REQUIRE(vertexData != nullptr);
  vertexGeom->setVertexAttributeMatrix(*vertexData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {8}, {1}, vertexData->getId());
  auto* mask = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", {8}, {1}, vertexData->getId());
  REQUIRE(featureIds != nullptr);
  REQUIRE(mask != nullptr);
  for(usize i = 0; i < 8; ++i)
  {
    (*featureIds)[i] = static_cast<int32>(i % 4 + 1);
    (*mask)[i] = i % 4 < 2;
    vertexGeom->setVertexCoordinate(i, {static_cast<float32>(i), 0.0F, 0.0F});
  }

  auto* featureData = AttributeMatrix::Create(dataStructure, "Vertex Feature Data", ShapeType{5}, vertexGeom->getId());
  REQUIRE(featureData != nullptr);
  auto* numElements = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureData->getId());
  REQUIRE(numElements != nullptr);
  numElements->fill(2);
  (*numElements)[0] = 0;
  auto* neighbors = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureData->getId());
  REQUIRE(neighbors != nullptr);
  neighbors->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureData->getId()) != nullptr);

  auto* ensembleData = AttributeMatrix::Create(dataStructure, "Vertex Ensemble Data", ShapeType{2}, vertexGeom->getId());
  REQUIRE(ensembleData != nullptr);
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleData->getId());
  REQUIRE(crystalStructures != nullptr);
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;
  return dataStructure;
}

/**
 * @brief Supplies every argument for the feature-removal fixture.
 * @param clear Whether to clear the destination Feature Attribute Matrix.
 * @param renumber Stored renumber setting, effective only when clear is true.
 * @return Arguments that preserve the source and create the Reduced geometry.
 */
Arguments MakeFeatureRemovalArgs(bool clear, bool renumber)
{
  Arguments args;
  args.insert(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(DataPath({"Vertex"})));
  args.insert(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data", "Mask"})));
  args.insert(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(DataPath({"Reduced"})));
  args.insert(RemoveFlaggedVerticesFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(clear));
  args.insert(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key, std::make_any<bool>(renumber));
  args.insert(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Feature Data"})));
  args.insert(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data", "FeatureIds"})));
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Instantiate", "[SimplnxCore][RemoveFlaggedVerticesFilter]")
{
  UnitTest::LoadPlugins();

  RemoveFlaggedVerticesFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>());

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>());

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: From Scratch", "[SimplnxCore][RemoveFlaggedVerticesFilter]")
{
  UnitTest::LoadPlugins();

  RemoveFlaggedVerticesFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>());

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);

  std::vector<usize> vertexTupleDims = {100};
  std::vector<usize> vertexCompDims = {3};

  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry, topLevelGroup->getId());
  Float32Array* coords = UnitTest::CreateTestDataArray<float>(dataStructure, "coords", vertexTupleDims, vertexCompDims, vertexGeom->getId());
  vertexGeom->setVertices(*coords); // Add the vertices to the VertexGeom object

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, Constants::k_VertexDataGroupName, vertexTupleDims, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

  Int32Array* slipVector = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_SlipVector, vertexTupleDims, vertexCompDims, vertexAttributeMatrix->getId());
  Int32Array* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, vertexTupleDims, {1}, vertexAttributeMatrix->getId());

  BoolArray* conditionalArray = UnitTest::CreateTestDataArray<bool>(dataStructure, Constants::k_ConditionalArray, vertexTupleDims, {1}, vertexAttributeMatrix->getId());
  conditionalArray->fill(false);
  // initialize the coords just to have something other than 0.0
  // Set the first 25 values of the conditional array to true, thus keeping 75 vertices
  for(size_t i = 0; i < vertexTupleDims[0]; i++)
  {
    coords->initializeTuple(i, static_cast<float>(i));
    slipVector->initializeTuple(i, static_cast<int32>(i));
    featureIds->initializeTuple(i, static_cast<int32>(i));
    if(i < 25)
    {
      conditionalArray->initializeTuple(i, true);
    }
  }

  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_VertexGeometry});
  DataPath vertexAMPath = vertexGeomPath.createChildPath(Constants::k_VertexDataGroupName);
  DataPath maskPath = vertexAMPath.createChildPath(Constants::k_ConditionalArray);
  DataPath reducedVertexPath({Constants::k_SmallIN100, Constants::k_ReducedGeometry});
  DataPath reducedVertexAMPath = reducedVertexPath.createChildPath(Constants::k_VertexDataGroupName);

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(reducedVertexPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  std::vector<DataPath> expectedArrayPaths;
  for([[maybe_unused]] const auto& [id, child] : dataStructure.getDataRefAs<AttributeMatrix>(vertexAMPath))
  {
    const DataPath sourcePath = vertexAMPath.createChildPath(child->getName());
    if(sourcePath.getTargetName() != VertexGeom::k_SharedVertexListName)
    {
      expectedArrayPaths.push_back(reducedVertexAMPath.createChildPath(sourcePath.getTargetName()));
    }
  }
  UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, expectedArrayPaths.size());
  UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, expectedArrayPaths);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* reducedVertexGeom = dataStructure.getDataAs<VertexGeom>(reducedVertexPath);

  size_t reducedTupleCount = reducedVertexGeom->getNumberOfVertices();
  REQUIRE(reducedTupleCount == 75);

  auto& reducedFeatureIds = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_FeatureIds));
  REQUIRE((reducedFeatureIds.getNumberOfTuples() == 75));

  auto& reducedSlipVectors = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_SlipVector));
  REQUIRE((reducedSlipVectors.getNumberOfTuples() == 75));

  for(size_t i = 0; i < reducedTupleCount; i++)
  {
    Point3D<float> coord = reducedVertexGeom->getVertexCoordinate(i);
    float compValue = static_cast<float>(i) + 25.0F;
    REQUIRE(((coord[0] == compValue) && (coord[1] == compValue) && (coord[2] == compValue)));

    REQUIRE(reducedFeatureIds[i] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3 + 1] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3 + 2] == i + 25);
  }

  // Write out the DataStructure for later viewing/debugging
  std::string filePath = fmt::format("{}/RemoveFlaggedVerticesFilter.dream3d", unit_test::k_BinaryTestOutputDir);
  // std::cout << "Writing file to: " << filePath << std::endl;
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);
  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Test Algorithm", "[SimplnxCore][RemoveFlaggedVerticesFilter]")
{
  UnitTest::LoadPlugins();

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "remove_flagged_elements_data.tar.gz", "remove_flagged_elements_data");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedVerticesFilter filter;
    Arguments args;
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>());
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>());

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(::k_VertexGeomPath));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(::k_DataName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(::k_DataName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(::k_CopyTestName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(::k_CopyTestName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(VertexGeom::k_SharedVertexListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_VertexListName);

    UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RemoveFlaggedVerticesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RemoveFlaggedVerticesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RemoveFlaggedVerticesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RemoveFlaggedVerticesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // The legacy filter copied every non-Vertex Attribute Matrix unchanged, so conversion turns Clear off.
      CHECK(args.value<bool>(RemoveFlaggedVerticesFilter::k_ClearFeatureAttributeMatrix_Key) == false);
      CHECK(args.value<bool>(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key) == false);
      CHECK(args.value<DataPath>(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key) == DataPath({"Vertex Data", "FeatureIds"}));
      CHECK(args.value<DataPath>(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key) == DataPath({"Vertex Feature Data"}));
      CHECK(args.value<DataPath>(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (StringToDataPathFilterParameterConverter) - verified by successful pipeline loading

      // The converted arguments preflight on a matching geometry that has no Feature Attribute Matrix.
      DataStructure dataStructure;
      auto* vertexGeom = VertexGeom::Create(dataStructure, "DataContainer");
      REQUIRE(vertexGeom != nullptr);
      auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, VertexGeom::k_SharedVertexListName, {3}, {3}, vertexGeom->getId());
      REQUIRE(vertices != nullptr);
      vertexGeom->setVertices(*vertices);
      auto* vertexData = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{3}, vertexGeom->getId());
      REQUIRE(vertexData != nullptr);
      vertexGeom->setVertexAttributeMatrix(*vertexData);
      REQUIRE(UnitTest::CreateTestDataArray<bool>(dataStructure, "TestArray", {3}, {1}, vertexData->getId()) != nullptr);
      auto preflightResult = filter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      CHECK(std::any_of(preflightResult.outputValues.begin(), preflightResult.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
    }
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedVertices: Child names containing the geometry name are kept", "[SimplnxCore][RemoveFlaggedVertices]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  const DataPath destGeomPath({"Geom Out"});
  auto* geomPtr = VertexGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  auto* verticesPtr = Float32Array::Create(dataStructure, VertexGeom::k_SharedVertexListName,
                                           DataStoreUtilities::CreateDataStore<float32>(dataStructure, geomPtr->getDataPaths().front().createChildPath(VertexGeom::k_SharedVertexListName), {3}, {3}),
                                           geomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  verticesPtr->fill(0.0F);
  geomPtr->setVertices(*verticesPtr);
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {3}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setVertexAttributeMatrix(*elementAmPtr);
  auto* maskPtr =
      BoolArray::Create(dataStructure, "Mask", DataStoreUtilities::CreateDataStore<bool>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Mask"), {3}, {1}), elementAmPtr->getId());
  REQUIRE(maskPtr != nullptr);
  maskPtr->fill(false);
  (*maskPtr)[0] = true;
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
  RemoveFlaggedVerticesFilter filter;
  Arguments args;
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(destGeomPath));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>(srcGeomPath.createChildPath("Vertex Data").createChildPath("Mask")));
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
    REQUIRE(dataStructure.getDataAs<Int32Array>(destGeomPath.createChildPath("Vertex Data").createChildPath("Data")) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON preflight", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  const bool renumber = GENERATE(false, true);
  auto result = filter.preflight(ds, MakeFeatureRemovalArgs(true, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  const auto it = std::find_if(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; });
  REQUIRE(it != result.outputValues.end());
  CHECK(it->value.find("Vertex/Vertex Feature Data/NumElements") != std::string::npos);
  CHECK(it->value.find("Vertex/Vertex Feature Data/NeighborList") != std::string::npos);
  CHECK(it->value.find("Vertex/Vertex Feature Data/Names") != std::string::npos);
  CHECK(it->value.find("Ensemble") == std::string::npos);
  CHECK((it->value.find("Feature Ids are renumbered.") != std::string::npos) == renumber);
  CHECK((it->value.find("The Attribute Matrix is resized to the number of features remaining after removing vertices.") != std::string::npos) == renumber);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Invalidated NeighborLists" || v.name == "Feature Data Not Cleared"; }));
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear OFF preflight", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRemovalArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  CHECK(std::any_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear OFF ignores stored renumber", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  Arguments args = MakeFeatureRemovalArgs(false, true);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data", "DoesNotExist"})));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: StrictFeatureIds preflight", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  const auto* vertexGeom = ds.getDataAs<VertexGeom>(DataPath({"Vertex"}));
  REQUIRE(vertexGeom != nullptr);
  const auto* vertexData = vertexGeom->getVertexAttributeMatrix();
  REQUIRE(vertexData != nullptr);
  auto* otherData = AttributeMatrix::Create(ds, "Other Vertex Data", vertexData->getShape(), vertexGeom->getId());
  REQUIRE(otherData != nullptr);
  auto* misplacedIds = UnitTest::CreateTestDataArray<int32>(ds, "MisplacedFeatureIds", otherData->getShape(), {1}, otherData->getId());
  REQUIRE(misplacedIds != nullptr);
  misplacedIds->fill(1);

  RemoveFlaggedVerticesFilter filter;
  Arguments args = MakeFeatureRemovalArgs(true, true);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Other Vertex Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == -67423);
    CHECK(result.outputActions.errors().front().message ==
          "**Vertex Feature Ids** DataArray 'Vertex/Other Vertex Data/MisplacedFeatureIds' must be in the Vertex Attribute Matrix 'Vertex/Vertex Data' of the selected Vertex Geometry 'Vertex'. "
          "Select the DataArray from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts another parent path to the same Vertex Attribute Matrix")
  {
    auto* alias = DataGroup::Create(ds, "Alias");
    REQUIRE(alias != nullptr);
    REQUIRE(ds.setAdditionalParent(vertexData->getId(), alias->getId()));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Alias", "Vertex Data", "FeatureIds"})));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
    const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
    REQUIRE(ids.getNumberOfTuples() == 4);
    const std::array<int32, 4> expectedIds = {1, 2, 1, 2};
    for(usize i = 0; i < expectedIds.size(); ++i)
    {
      CHECK(ids[i] == expectedIds[i]);
    }
  }
  SECTION("Renumber ON accepts Feature Ids in the Vertex Attribute Matrix")
  {
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data", "FeatureIds"})));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON Feature AM errors", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  // An Attribute Matrix that is not a child of "Vertex".
  REQUIRE(AttributeMatrix::Create(ds, "Elsewhere", std::vector<usize>{5}) != nullptr);
  RemoveFlaggedVerticesFilter filter;

  Arguments args = MakeFeatureRemovalArgs(true, false);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.outputActions);
  REQUIRE(notChild.outputActions.errors().size() == 1);
  CHECK(notChild.outputActions.errors().front().code == -67421);
  CHECK(notChild.outputActions.errors().front().message.find("Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.") != std::string::npos);

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data"})));
  auto isVertexAm = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isVertexAm.outputActions);
  REQUIRE(isVertexAm.outputActions.errors().size() == 1);
  CHECK(isVertexAm.outputActions.errors().front().code == -67422);

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Missing"})));
  auto missing = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.outputActions);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON accepts shared parent paths", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  auto* aliases = DataGroup::Create(ds, "Aliases");
  REQUIRE(aliases != nullptr);
  const auto* vertexGeom = ds.getDataAs<VertexGeom>(DataPath({"Vertex"}));
  REQUIRE(vertexGeom != nullptr);
  const auto* featureAm = ds.getDataAs<AttributeMatrix>(DataPath({"Vertex", "Vertex Feature Data"}));
  REQUIRE(featureAm != nullptr);
  // The Feature AM is still a direct child of "Vertex"; it is only selected through a second parent.
  REQUIRE(ds.setAdditionalParent(vertexGeom->getVertexAttributeMatrix()->getId(), aliases->getId()));
  REQUIRE(ds.setAdditionalParent(featureAm->getId(), aliases->getId()));
  Arguments args = MakeFeatureRemovalArgs(true, true);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Vertex Data", "FeatureIds"})));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Vertex Feature Data"})));
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getNumberOfTuples() == 3); // features 3 and 4 remain, plus row 0
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON, renumber OFF execute", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(ds, MakeFeatureRemovalArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // Retained vertices keep their original Feature Ids (3 and 4).
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {3, 4, 3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Vertex", "Vertex Feature Data"})));
  CHECK(ds.getDataRefAs<AttributeMatrix>(DataPath({"Vertex", "Vertex Feature Data"})).getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON, renumber ON execute", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(ds, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{3}); // features 3 and 4 remain, plus row 0
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(ds, MakeFeatureRemovalArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Feature Data", "NumElements"})));
  const auto& numElements = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 2, 2, 2, 2};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(ds.getDataRefAs<NeighborList<int32>>(DataPath({"Reduced", "Vertex Feature Data", "NeighborList"})));
  const auto& neighbors = ds.getDataRefAs<NeighborList<int32>>(DataPath({"Reduced", "Vertex Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<StringArray>(DataPath({"Reduced", "Vertex Feature Data", "Names"})));
  const auto& names = ds.getDataRefAs<StringArray>(DataPath({"Reduced", "Vertex Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {3, 4, 3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON Feature Id tuple count", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  auto* vertexGeom = ds.getDataAs<VertexGeom>(DataPath({"Vertex"}));
  REQUIRE(vertexGeom != nullptr);
  // Keep the Vertex AM internally consistent, but give the geometry one extra vertex.
  auto resizeResult = vertexGeom->resizeVertexList(9);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors().front().code == -67424);
  CHECK(result.outputActions.errors().front().message ==
        "**Vertex Feature Ids** DataArray 'Vertex/Vertex Data/FeatureIds' has 8 tuples, but the selected Vertex Geometry 'Vertex' has 9 vertices. Select a DataArray with one tuple per vertex.");
  resizeResult = vertexGeom->resizeVertexList(8);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON validates source Feature Id range", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  const int32 invalidId = GENERATE(int32{-1}, int32{5});
  const usize vertexIndex = GENERATE(usize{0}, usize{2});
  CAPTURE(invalidId, vertexIndex);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"})));
  auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"}));
  // Index 0 is removed; index 2 is retained. Both must be checked against the source AM.
  ids[vertexIndex] = invalidId;
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(ds, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors().front().code == (invalidId < 0 ? -5355 : -5351));
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON renumber preserves background", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureRemovalFixture();
  const bool removeAll = GENERATE(false, true);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"})));
  auto& sourceIds = ds.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"}));
  sourceIds[2] = 0;
  if(removeAll)
  {
    REQUIRE_NOTHROW(ds.getDataRefAs<BoolArray>(DataPath({"Vertex", "Vertex Data", "Mask"})));
    ds.getDataRefAs<BoolArray>(DataPath({"Vertex", "Vertex Data", "Mask"})).fill(true);
  }
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(ds, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& featureAm = ds.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(featureAm.getNumberOfTuples() == (removeAll ? 1 : 3));
  CHECK(featureAm.getSize() == 0);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == (removeAll ? 0 : 4));
  if(!removeAll)
  {
    const std::array<int32, 4> expectedIds = {0, 2, 1, 2};
    for(usize i = 0; i < expectedIds.size(); ++i)
    {
      CHECK(ids[i] == expectedIds[i]);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Feature AM nested alias regression", "[SimplnxCore][RemoveFlaggedVerticesFilter][NestedFeatureAlias]")
{
  UnitTest::LoadPlugins();
  const bool clear = GENERATE(true, false);
  const bool renumber = GENERATE(false, true);
  const bool selectNested = GENERATE(false, true);
  auto ds = CreateFeatureRemovalFixture();
  const DataPath sourcePath({"Vertex"});
  const DataPath outputPath({"Reduced"});
  const auto featurePath = sourcePath.createChildPath("Vertex Feature Data");
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
  auto args = MakeFeatureRemovalArgs(clear, renumber);
  if(selectNested)
  {
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(sourcePath.createChildPath("Misc").createChildPath("Vertex Feature Data")));
  }
  RemoveFlaggedVerticesFilter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool foundNestedCopy = false;
  const auto nestedOutputPath = outputPath.createChildPath("Misc").createChildPath("Vertex Feature Data");
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
  const auto* outputFeatures = ds.getDataAs<AttributeMatrix>(outputPath.createChildPath("Vertex Feature Data"));
  REQUIRE(outputFeatures != nullptr);
  CHECK(outputFeatures->getSize() == (clear ? 0 : 3));
  CHECK((ds.getData(nestedOutputPath) == nullptr) == clear);
  CHECK(ds.getDataRefAs<Int32Array>(outputPath.createChildPath("Misc").createChildPath("Keep"))[0] == 123);
  CHECK(ds.getData(featurePath)->getId() == featureId);
  CHECK(ds.getData(featurePath)->getParentIds() == parents);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getShape() == shape);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getSize() == 3);
  CHECK(ds.getData(DataPath({"Outside", "Misc", "Vertex Feature Data"}))->getId() == featureId);
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    CHECK(ds.getDataRefAs<Int32Array>(countsPath)[i] == originalCounts[i]);
    if(!clear)
    {
      CHECK(ds.getDataRefAs<Int32Array>(nestedOutputPath.createChildPath("NumElements"))[i] == originalCounts[i]);
    }
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Reject nonnumeric Feature Ids", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  const bool strings = GENERATE(true, false);
  auto ds = CreateFeatureRemovalFixture();
  const auto idsPath = DataPath({"Vertex", "Vertex Data", "FeatureIds"});
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
  RemoveFlaggedVerticesFilter filter;
  auto args = MakeFeatureRemovalArgs(true, true);
  REQUIRE_NOTHROW(filter.preflight(ds, args));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67425);
  CHECK(result.outputActions.errors()[0].message.find(idsPath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("single-component int32 DataArray") != std::string::npos);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Reject another geometry element Feature AM", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureRemovalFixture();
  const auto featurePath = DataPath({"Vertex", "Vertex Feature Data"});
  const auto* features = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(features != nullptr);
  auto* other = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(other != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {features->getNumberOfTuples()}, {3}, other->getId());
  REQUIRE(vertices != nullptr);
  other->setVertices(*vertices);
  REQUIRE(ds.setAdditionalParent(features->getId(), other->getId()));
  other->setVertexAttributeMatrix(*features);
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRemovalArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67460);
  CHECK(result.outputActions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(other->getVertexAttributeMatrix() == features);
  CHECK(features->getSize() > 0);
}
