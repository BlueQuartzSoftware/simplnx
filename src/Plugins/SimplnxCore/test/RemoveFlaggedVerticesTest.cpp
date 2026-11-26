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
fs::path KBaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_elements_data/remove_flagged_vertices_data.dream3d", nx::core::unit_test::k_TestFilesDir));

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
  auto* vertexGeomPtr = VertexGeom::Create(dataStructure, "Vertex");
  REQUIRE(vertexGeomPtr != nullptr);
  auto* verticesPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, VertexGeom::k_SharedVertexListName, {8}, {3}, vertexGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  vertexGeomPtr->setVertices(*verticesPtr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{8}, vertexGeomPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  vertexGeomPtr->setVertexAttributeMatrix(*vertexDataPtr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {8}, {1}, vertexDataPtr->getId());
  auto* maskPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", {8}, {1}, vertexDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  for(usize i = 0; i < 8; ++i)
  {
    (*featureIdsPtr)[i] = static_cast<int32>(i % 4 + 1);
    (*maskPtr)[i] = i % 4 < 2;
    vertexGeomPtr->setVertexCoordinate(i, {static_cast<float32>(i), 0.0F, 0.0F});
  }

  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Feature Data", ShapeType{5}, vertexGeomPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* numElementsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureDataPtr->getId());
  REQUIRE(numElementsPtr != nullptr);
  numElementsPtr->fill(2);
  (*numElementsPtr)[0] = 0;
  auto* neighborsPtr = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureDataPtr->getId());
  REQUIRE(neighborsPtr != nullptr);
  neighborsPtr->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureDataPtr->getId()) != nullptr);

  auto* ensembleDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Ensemble Data", ShapeType{2}, vertexGeomPtr->getId());
  REQUIRE(ensembleDataPtr != nullptr);
  auto* crystalStructuresPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleDataPtr->getId());
  REQUIRE(crystalStructuresPtr != nullptr);
  (*crystalStructuresPtr)[0] = 999;
  (*crystalStructuresPtr)[1] = 1;
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

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Preflight warns for unchanged Attribute Matrices", "[SimplnxCore][RemoveFlaggedVerticesFilter][UnchangedAttributeMatrices]")
{
  auto ds = CreateFeatureRemovalFixture();
  const DataPath featurePath({"Vertex", "Vertex Feature Data"});
  const DataPath ensemblePath({"Vertex", "Vertex Ensemble Data"});
  const RemoveFlaggedVerticesFilter filter;

  SECTION("Feature clearing leaves Ensemble data")
  {
    auto result = filter.preflight(ds, MakeFeatureRemovalArgs(true, false));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    REQUIRE(result.outputActions.warnings().size() == 1);
    const auto& warning = result.outputActions.warnings().front();
    CHECK(warning.code == -4041);
    CHECK(warning.message.find("\n" + ensemblePath.toString() + "\n") != std::string::npos);
    CHECK(warning.message.find("\n" + featurePath.toString() + "\n") == std::string::npos);
  }

  SECTION("Only the registered element data remains")
  {
    REQUIRE(ds.removeData(featurePath));
    REQUIRE(ds.removeData(ensemblePath));
    auto result = filter.preflight(ds, MakeFeatureRemovalArgs(false, false));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    CHECK(result.outputActions.warnings().empty());
  }
}

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

  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, Constants::k_SmallIN100);

  std::vector<usize> vertexTupleDims = {100};
  std::vector<usize> vertexCompDims = {3};

  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeomPtr = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry, topLevelGroupPtr->getId());
  Float32Array* coordsPtr = UnitTest::CreateTestDataArray<float>(dataStructure, "coords", vertexTupleDims, vertexCompDims, vertexGeomPtr->getId());
  vertexGeomPtr->setVertices(*coordsPtr); // Add the vertices to the VertexGeom object

  auto* vertexAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, Constants::k_VertexDataGroupName, vertexTupleDims, vertexGeomPtr->getId());
  vertexGeomPtr->setVertexAttributeMatrix(*vertexAttributeMatrixPtr);

  Int32Array* slipVectorPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_SlipVector, vertexTupleDims, vertexCompDims, vertexAttributeMatrixPtr->getId());
  Int32Array* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, vertexTupleDims, {1}, vertexAttributeMatrixPtr->getId());

  BoolArray* conditionalArrayPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, Constants::k_ConditionalArray, vertexTupleDims, {1}, vertexAttributeMatrixPtr->getId());
  conditionalArrayPtr->fill(false);
  // initialize the coords just to have something other than 0.0
  // Set the first 25 values of the conditional array to true, thus keeping 75 vertices
  for(size_t i = 0; i < vertexTupleDims[0]; i++)
  {
    coordsPtr->initializeTuple(i, static_cast<float>(i));
    slipVectorPtr->initializeTuple(i, static_cast<int32>(i));
    featureIdsPtr->initializeTuple(i, static_cast<int32>(i));
    if(i < 25)
    {
      conditionalArrayPtr->initializeTuple(i, true);
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

  auto* reducedVertexGeomPtr = dataStructure.getDataAs<VertexGeom>(reducedVertexPath);

  size_t reducedTupleCount = reducedVertexGeomPtr->getNumberOfVertices();
  REQUIRE(reducedTupleCount == 75);

  auto& reducedFeatureIds = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_FeatureIds));
  REQUIRE((reducedFeatureIds.getNumberOfTuples() == 75));

  auto& reducedSlipVectors = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_SlipVector));
  REQUIRE((reducedSlipVectors.getNumberOfTuples() == 75));

  for(size_t i = 0; i < reducedTupleCount; i++)
  {
    Point3D<float> coord = reducedVertexGeomPtr->getVertexCoordinate(i);
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
  DataStructure dataStructure = UnitTest::LoadDataStructure(KBaseDataFilePath);

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
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RemoveFlaggedVerticesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RemoveFlaggedVerticesFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<RemoveFlaggedVerticesFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
      auto* vertexGeomPtr = VertexGeom::Create(dataStructure, "DataContainer");
      REQUIRE(vertexGeomPtr != nullptr);
      auto* verticesPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, VertexGeom::k_SharedVertexListName, {3}, {3}, vertexGeomPtr->getId());
      REQUIRE(verticesPtr != nullptr);
      vertexGeomPtr->setVertices(*verticesPtr);
      auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{3}, vertexGeomPtr->getId());
      REQUIRE(vertexDataPtr != nullptr);
      vertexGeomPtr->setVertexAttributeMatrix(*vertexDataPtr);
      REQUIRE(UnitTest::CreateTestDataArray<bool>(dataStructure, "TestArray", {3}, {1}, vertexDataPtr->getId()) != nullptr);
      auto preflightResult = filterPtr->preflight(dataStructure, args);
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
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  const bool renumber = GENERATE(false, true);
  auto result = filter.preflight(dataStructure, MakeFeatureRemovalArgs(true, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE(result.outputActions.warnings().size() == 1);
  CHECK(result.outputActions.warnings().front().code == -4041);
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
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.preflight(dataStructure, MakeFeatureRemovalArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE(result.outputActions.warnings().size() == 1);
  CHECK(result.outputActions.warnings().front().code == -4041);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  CHECK(std::any_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear OFF ignores stored renumber", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  Arguments args = MakeFeatureRemovalArgs(false, true);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data", "DoesNotExist"})));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(result.outputActions.warnings().size() == 1);
  CHECK(result.outputActions.warnings().front().code == -4041);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: StrictFeatureIds preflight", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  const auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(DataPath({"Vertex"}));
  REQUIRE(vertexGeomPtr != nullptr);
  const auto* vertexDataPtr = vertexGeomPtr->getVertexAttributeMatrix();
  REQUIRE(vertexDataPtr != nullptr);
  auto* otherDataPtr = AttributeMatrix::Create(dataStructure, "Other Vertex Data", vertexDataPtr->getShape(), vertexGeomPtr->getId());
  REQUIRE(otherDataPtr != nullptr);
  auto* misplacedIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "MisplacedFeatureIds", otherDataPtr->getShape(), {1}, otherDataPtr->getId());
  REQUIRE(misplacedIdsPtr != nullptr);
  misplacedIdsPtr->fill(1);

  RemoveFlaggedVerticesFilter filter;
  Arguments args = MakeFeatureRemovalArgs(true, true);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Other Vertex Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(dataStructure, args);
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
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts another parent path to the same Vertex Attribute Matrix")
  {
    auto* aliasPtr = DataGroup::Create(dataStructure, "Alias");
    REQUIRE(aliasPtr != nullptr);
    REQUIRE(dataStructure.setAdditionalParent(vertexDataPtr->getId(), aliasPtr->getId()));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Alias", "Vertex Data", "FeatureIds"})));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
    const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
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
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON Feature AM errors", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  // An Attribute Matrix that is not a child of "Vertex".
  REQUIRE(AttributeMatrix::Create(dataStructure, "Elsewhere", std::vector<usize>{5}) != nullptr);
  RemoveFlaggedVerticesFilter filter;

  Arguments args = MakeFeatureRemovalArgs(true, false);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.outputActions);
  REQUIRE(notChild.outputActions.errors().size() == 1);
  CHECK(notChild.outputActions.errors().front().code == -67421);
  CHECK(notChild.outputActions.errors().front().message.find("Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.") != std::string::npos);

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Vertex Data"})));
  auto isVertexAm = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isVertexAm.outputActions);
  REQUIRE(isVertexAm.outputActions.errors().size() == 1);
  CHECK(isVertexAm.outputActions.errors().front().code == -67422);

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Vertex", "Missing"})));
  auto missing = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.outputActions);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON accepts shared parent paths", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  auto* aliasesPtr = DataGroup::Create(dataStructure, "Aliases");
  REQUIRE(aliasesPtr != nullptr);
  const auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(DataPath({"Vertex"}));
  REQUIRE(vertexGeomPtr != nullptr);
  const auto* featureAmPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Vertex", "Vertex Feature Data"}));
  REQUIRE(featureAmPtr != nullptr);
  // The Feature AM is still a direct child of "Vertex"; it is only selected through a second parent.
  REQUIRE(dataStructure.setAdditionalParent(vertexGeomPtr->getVertexAttributeMatrix()->getId(), aliasesPtr->getId()));
  REQUIRE(dataStructure.setAdditionalParent(featureAmPtr->getId(), aliasesPtr->getId()));
  Arguments args = MakeFeatureRemovalArgs(true, true);
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_VertexFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Vertex Data", "FeatureIds"})));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Vertex Feature Data"})));
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getNumberOfTuples() == 3); // features 3 and 4 remain, plus row 0
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON, renumber OFF execute", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureRemovalArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // Retained vertices keep their original Feature Ids (3 and 4).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {3, 4, 3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Vertex", "Vertex Feature Data"})));
  CHECK(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Vertex", "Vertex Feature Data"})).getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON through a linked root placement", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  auto ds = CreateFeatureRemovalFixture();
  const auto* vertexPtr = ds.getData(DataPath({"Vertex"}));
  auto* linkedPtr = DataGroup::Create(ds, "Linked");
  auto* nestedPtr = DataGroup::Create(ds, "Nested", vertexPtr->getId());
  REQUIRE(linkedPtr != nullptr);
  REQUIRE(nestedPtr != nullptr);
  REQUIRE(ds.setAdditionalParent(vertexPtr->getId(), linkedPtr->getId()));
  const DataPath featurePath({"Vertex", "Vertex Feature Data"});
  const auto featureId = ds.getData(featurePath)->getId();
  REQUIRE(ds.setAdditionalParent(featureId, nestedPtr->getId()));
  const auto parents = ds.getData(featurePath)->getParentIds();

  RemoveFlaggedVerticesFilter filter;
  const auto args = MakeFeatureRemovalArgs(true, false);
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const DataPath reducedFeaturePath({"Reduced", "Vertex Feature Data"});
  const auto* replacementPtr = ds.getDataAs<AttributeMatrix>(reducedFeaturePath);
  REQUIRE(replacementPtr != nullptr);
  CHECK(replacementPtr->getId() != featureId);
  CHECK(replacementPtr->getSize() == 0);
  CHECK(ds.getData(DataPath({"Reduced", "Nested", "Vertex Feature Data"})) == nullptr);
  const auto* sourceFeaturePtr = ds.getData(featurePath);
  REQUIRE(sourceFeaturePtr != nullptr);
  CHECK(sourceFeaturePtr->getParentIds() == parents);

  for(const auto& placement :
      {featurePath, DataPath({"Vertex", "Nested", "Vertex Feature Data"}), DataPath({"Linked", "Vertex", "Vertex Feature Data"}), DataPath({"Linked", "Vertex", "Nested", "Vertex Feature Data"})})
  {
    CHECK(ds.getId(placement) == featureId);
    const auto* countsPtr = ds.getDataAs<Int32Array>(placement.createChildPath("NumElements"));
    const auto* namesPtr = ds.getDataAs<StringArray>(placement.createChildPath("Names"));
    const auto* neighborsPtr = ds.getDataAs<NeighborList<int32>>(placement.createChildPath("NeighborList"));
    REQUIRE(countsPtr != nullptr);
    REQUIRE(namesPtr != nullptr);
    REQUIRE(neighborsPtr != nullptr);
    const std::array<int32, 5> expectedCounts = {0, 2, 2, 2, 2};
    const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
    const std::array<std::vector<int32>, 5> expectedNeighbors = {{{}, {2}, {1, 3}, {2, 4}, {3}}};
    for(usize i = 0; i < expectedCounts.size(); ++i)
    {
      CHECK((*countsPtr)[i] == expectedCounts[i]);
      CHECK((*namesPtr)[i] == expectedNames[i]);
      CHECK(neighborsPtr->getList(i) == expectedNeighbors[i]);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON, renumber ON execute", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{3}); // features 3 and 4 remain, plus row 0
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  RemoveFlaggedVerticesFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(dataStructure, MakeFeatureRemovalArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Feature Data", "NumElements"})));
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 2, 2, 2, 2};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Reduced", "Vertex Feature Data", "NeighborList"})));
  const auto& neighbors = dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Reduced", "Vertex Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(DataPath({"Reduced", "Vertex Feature Data", "Names"})));
  const auto& names = dataStructure.getDataRefAs<StringArray>(DataPath({"Reduced", "Vertex Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 4);
  const std::array<int32, 4> expectedIds = {3, 4, 3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Reduced", "Vertex Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON Feature Id tuple count", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(DataPath({"Vertex"}));
  REQUIRE(vertexGeomPtr != nullptr);
  // Keep the Vertex AM internally consistent, but give the geometry one extra vertex.
  auto resizeResult = vertexGeomPtr->resizeVertexList(9);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.preflight(dataStructure, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors().front().code == -67424);
  CHECK(result.outputActions.errors().front().message ==
        "**Vertex Feature Ids** DataArray 'Vertex/Vertex Data/FeatureIds' has 8 tuples, but the selected Vertex Geometry 'Vertex' has 9 vertices. Select a DataArray with one tuple per vertex.");
  resizeResult = vertexGeomPtr->resizeVertexList(8);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON validates source Feature Id range", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  const int32 invalidId = GENERATE(int32{-1}, int32{5});
  const usize vertexIndex = GENERATE(usize{0}, usize{2});
  CAPTURE(invalidId, vertexIndex);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"})));
  auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"}));
  // Index 0 is removed; index 2 is retained. Both must be checked against the source AM.
  ids[vertexIndex] = invalidId;
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors().front().code == (invalidId < 0 ? -5355 : -5351));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Clear ON renumber preserves background", "[SimplnxCore][RemoveFlaggedVerticesFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureRemovalFixture();
  const bool removeAll = GENERATE(false, true);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"})));
  auto& sourceIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"Vertex", "Vertex Data", "FeatureIds"}));
  sourceIds[2] = 0;
  if(removeAll)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(DataPath({"Vertex", "Vertex Data", "Mask"})));
    dataStructure.getDataRefAs<BoolArray>(DataPath({"Vertex", "Vertex Data", "Mask"})).fill(true);
  }
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureRemovalArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"})));
  const auto& featureAm = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Reduced", "Vertex Feature Data"}));
  CHECK(featureAm.getNumberOfTuples() == (removeAll ? 1 : 3));
  CHECK(featureAm.getSize() == 0);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Reduced", "Vertex Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == (removeAll ? 0 : 4));
  if(!removeAll)
  {
    const std::array<int32, 4> expectedIds = {0, 2, 1, 2};
    for(usize i = 0; i < expectedIds.size(); ++i)
    {
      CHECK(ids[i] == expectedIds[i]);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  const auto featureId = featuresPtr->getId();
  const auto shape = featuresPtr->getShape();
  const auto countsPath = featurePath.createChildPath("NumElements");
  const auto& counts = ds.getDataRefAs<Int32Array>(countsPath);
  std::vector<int32> originalCounts(counts.getSize());
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    originalCounts[i] = counts[i];
  }
  auto* nestedPtr = DataGroup::Create(ds, "Misc", ds.getData(sourcePath)->getId());
  REQUIRE(nestedPtr != nullptr);
  auto* keepPtr = UnitTest::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nestedPtr->getId());
  REQUIRE(keepPtr != nullptr);
  (*keepPtr)[0] = 123;
  REQUIRE(ds.setAdditionalParent(featureId, nestedPtr->getId()));
  auto* outsidePtr = DataGroup::Create(ds, "Outside");
  REQUIRE(outsidePtr != nullptr);
  REQUIRE(ds.setAdditionalParent(nestedPtr->getId(), outsidePtr->getId()));
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
  const auto* outputFeaturesPtr = ds.getDataAs<AttributeMatrix>(outputPath.createChildPath("Vertex Feature Data"));
  REQUIRE(outputFeaturesPtr != nullptr);
  CHECK(outputFeaturesPtr->getSize() == (clear ? 0 : 3));
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
  const auto* originalPtr = ds.getDataAs<Int32Array>(idsPath);
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
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(otherPtr != nullptr);
  auto* verticesPtr = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {featuresPtr->getNumberOfTuples()}, {3}, otherPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  otherPtr->setVertices(*verticesPtr);
  REQUIRE(ds.setAdditionalParent(featuresPtr->getId(), otherPtr->getId()));
  otherPtr->setVertexAttributeMatrix(*featuresPtr);
  RemoveFlaggedVerticesFilter filter;
  auto result = filter.preflight(ds, MakeFeatureRemovalArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67460);
  CHECK(result.outputActions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(otherPtr->getVertexAttributeMatrix() == featuresPtr);
  CHECK(featuresPtr->getSize() > 0);
}
