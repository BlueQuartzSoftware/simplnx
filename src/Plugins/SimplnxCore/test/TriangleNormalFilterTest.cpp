#include "SimplnxCore/Filters/TriangleNormalFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/TriangleNormal.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
constexpr float64 k_max_difference = 0.0001;
}

TEST_CASE("SimplnxCore::TriangleNormalFilter", "[SimplnxCore][TriangleNormalFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Face Normals";

  DataStructure dataStructure;

  {
    Arguments args;
    ReadStlFileFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_SimplnxTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    TriangleNormalFilter filter;
    Arguments args;
    std::string triangleNormalsName = "Face Normals (computed)";

    DataPath geometryPath = DataPath({triangleGeometryName});
    const DataPath verticesPath = geometryPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);
    const DataPath facesPath = geometryPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(verticesPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(facesPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(verticesPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(facesPath));

    // Create default Parameters for the filter.
    args.insertOrAssign(TriangleNormalFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayName_Key, std::make_any<std::string>(triangleNormalsName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    nx::core::UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/TriangleNormals.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath triangleNormalsDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleNormalsName);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(triangleNormalsDataPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(triangleNormalsDataPath));

    // Let's compare the normals.
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});
    auto& officialNormals = dataStructure.getDataRefAs<Float64Array>(normalsDataPath);
    Float64Array& calculatedNormals = dataStructure.getDataRefAs<Float64Array>(triangleNormalsDataPath);
    std::vector<double> offNorms, calcNorms;
    for(int64 i = 0; i < officialNormals.getSize(); i++)
    {
      auto result = fabs(officialNormals[i] - calculatedNormals[i]);
      REQUIRE(result < ::k_max_difference);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::TriangleNormalFilter: SIMPL Backwards Compatibility", "[SimplnxCore][TriangleNormalFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "TriangleNormalFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "TriangleNormalFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<TriangleNormalFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(TriangleNormalFilter::k_TriGeometryDataPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayName_Key) == "TestArray");
    }
  }
}

namespace
{
constexpr usize k_NormalBlockFaces = 262144;
constexpr usize k_NormalBufferFaces = 4096;
constexpr float64 k_NormalSentinel = -37.0;
constexpr int32 k_NormalTransferError = -98761;
const DataPath k_NormalGeometryPath({"NormalMesh"});
const DataPath k_NormalVerticesPath = k_NormalGeometryPath.createChildPath("Vertices");
const DataPath k_NormalFacesPath = k_NormalGeometryPath.createChildPath("Faces");
const DataPath k_NormalOutputPath = k_NormalGeometryPath.createChildPath("FaceData").createChildPath("Normals");
constexpr std::array<float32, 18> k_NormalVertices = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 3, 0, 1, 0, 2, 1};
constexpr std::array<std::array<IGeometry::MeshIndexType, 3>, 5> k_NormalFaces = {{{0, 1, 2}, {0, 2, 1}, {0, 2, 3}, {0, 4, 5}, {0, 3, 2}}};
// The sloped face has cross product (-2, -3, 6) and length 7. These literals require double arithmetic.
constexpr std::array<std::array<float64, 3>, 5> k_ExpectedNormals = {
    {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-0.2857142857142857142857142857, -0.4285714285714285714285714286, 0.8571428571428571428571428571}, {-1, 0, 0}}};

usize NormalFixtureRow(usize faceIndex)
{
  return faceIndex == k_NormalBlockFaces ? 4 : faceIndex % 4;
}

enum class NormalStoreRole : usize
{
  Vertices,
  Faces,
  Normals
};

struct NormalTransfer
{
  NormalStoreRole role;
  bool write;
  usize start;
  usize count;
};

struct NormalTransferState
{
  bool armed = false;
  NormalStoreRole failingRole = NormalStoreRole::Normals;
  bool failingWrite = false;
  usize failingCall = 0;
  bool cancelAfterFirstWrite = false;
  std::atomic_bool shouldCancel{false};
  std::array<usize, 3> reads{};
  std::array<usize, 3> writes{};
  std::vector<NormalTransfer> calls;
};

const char* NormalFailureMessage(NormalStoreRole role)
{
  switch(role)
  {
  case NormalStoreRole::Vertices:
    return "Injected triangle normal vertex read failure.";
  case NormalStoreRole::Faces:
    return "Injected triangle normal connectivity read failure.";
  case NormalStoreRole::Normals:
    return "Injected triangle normal output write failure.";
  }
  return "Unknown triangle normal transfer failure.";
}

/**
 * @class NormalReportingStore
 * @brief Records sequential bulk transfers and rejects selected calls before touching resident values.
 * @tparam T Stored scalar type.
 * @note The borrowed state outlives the store. This fixture does not model partial backend writes.
 */
template <typename T>
class NormalReportingStore final : public DataStore<T>
{
public:
  /**
   * @brief Creates real resident storage with a disarmed, longer-lived trace.
   * @param tuples Number of three-component tuples.
   * @param state Records target transfers only.
   * @param role Identifies the array in each transfer.
   */
  NormalReportingStore(usize tuples, NormalTransferState& state, NormalStoreRole role)
  : DataStore<T>(ShapeType{tuples}, ShapeType{3}, T{})
  , m_State(state)
  , m_Role(role)
  {
  }

  ~NormalReportingStore() override = default;
  NormalReportingStore(const NormalReportingStore&) = default;
  NormalReportingStore(NormalReportingStore&&) = default;
  NormalReportingStore& operator=(const NormalReportingStore&) = delete;
  NormalReportingStore& operator=(NormalReportingStore&&) = delete;

  /**
   * @brief Records a read and refuses the selected call before changing the buffer.
   * @param startIndex First flat value index.
   * @param buffer Receives the resident values on success.
   * @return Injected error or the resident read result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    if(m_State.armed)
    {
      const usize call = ++m_State.reads[static_cast<usize>(m_Role)];
      m_State.calls.push_back({m_Role, false, startIndex, buffer.size()});
      if(m_State.failingRole == m_Role && !m_State.failingWrite && m_State.failingCall == call)
      {
        return MakeErrorResult(k_NormalTransferError, NormalFailureMessage(m_Role));
      }
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  /**
   * @brief Records writes and optionally cancels after the first completed output block.
   * @param startIndex First flat value index.
   * @param buffer Supplies values to publish unless the selected call fails.
   * @return Injected error or the resident write result.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    if(!m_State.armed)
    {
      return DataStore<T>::copyFromBuffer(startIndex, buffer);
    }
    const usize call = ++m_State.writes[static_cast<usize>(m_Role)];
    m_State.calls.push_back({m_Role, true, startIndex, buffer.size()});
    if(m_State.failingRole == m_Role && m_State.failingWrite && m_State.failingCall == call)
    {
      return MakeErrorResult(k_NormalTransferError, NormalFailureMessage(m_Role));
    }
    auto result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && m_Role == NormalStoreRole::Normals && call == 1 && m_State.cancelAfterFirstWrite)
    {
      m_State.shouldCancel = true;
    }
    return result;
  }

private:
  NormalTransferState& m_State;
  const NormalStoreRole m_Role;
};

/**
 * @struct NormalTraceGuard
 * @brief Limits recording to the target call and disarms on assertion failure.
 */
struct NormalTraceGuard
{
  /**
   * @brief Clears setup activity and starts the target transfer trace.
   * @param state Outlives this guard and its reporting stores.
   */
  explicit NormalTraceGuard(NormalTransferState& state)
  : stateRef(state)
  {
    stateRef.calls.clear();
    stateRef.reads.fill(0);
    stateRef.writes.fill(0);
    stateRef.armed = true;
  }

  /**
   * @brief Stops recording even when the target or a test assertion throws.
   */
  ~NormalTraceGuard()
  {
    stateRef.armed = false;
  }

  NormalTraceGuard(const NormalTraceGuard&) = delete;
  NormalTraceGuard(NormalTraceGuard&&) = delete;
  NormalTraceGuard& operator=(const NormalTraceGuard&) = delete;
  NormalTraceGuard& operator=(NormalTraceGuard&&) = delete;

  NormalTransferState& stateRef;
};

/**
 * @struct NormalFixture
 * @brief Supplies six shared vertices and repeated triangles with a distinct final block tail.
 */
struct NormalFixture
{
  DataStructure dataStructure;
  TriangleGeom* geometryPtr = nullptr;
  Float32Array* verticesPtr = nullptr;
  DataArray<IGeometry::MeshIndexType>* facesPtr = nullptr;
  AttributeMatrix* faceDataPtr = nullptr;
  AttributeMatrix* vertexDataPtr = nullptr;

  /**
   * @brief Creates accepted filter inputs with no output array.
   * @param faceCount Number of faces; zero retains a valid empty tuple shape.
   * @param state Optional longer-lived state selects real resident reporting stores.
   */
  explicit NormalFixture(usize faceCount, NormalTransferState* state = nullptr)
  {
    geometryPtr = TriangleGeom::Create(dataStructure, k_NormalGeometryPath.getTargetName());
    REQUIRE(geometryPtr != nullptr);
    faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {faceCount}, geometryPtr->getId());
    vertexDataPtr = AttributeMatrix::Create(dataStructure, "VertexData", {6}, geometryPtr->getId());
    REQUIRE(faceDataPtr != nullptr);
    REQUIRE(vertexDataPtr != nullptr);
    geometryPtr->setFaceAttributeMatrix(*faceDataPtr);
    geometryPtr->setVertexAttributeMatrix(*vertexDataPtr);
    std::shared_ptr<AbstractDataStore<float32>> verticesStore;
    std::shared_ptr<AbstractDataStore<IGeometry::MeshIndexType>> facesStore;
    if(state != nullptr)
    {
      verticesStore = std::make_shared<NormalReportingStore<float32>>(6, *state, NormalStoreRole::Vertices);
      facesStore = std::make_shared<NormalReportingStore<IGeometry::MeshIndexType>>(faceCount, *state, NormalStoreRole::Faces);
    }
    else
    {
      verticesStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_NormalVerticesPath, {6}, {3});
      facesStore = DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, k_NormalFacesPath, {faceCount}, {3});
    }
    REQUIRE(verticesStore != nullptr);
    REQUIRE(facesStore != nullptr);
    verticesPtr = Float32Array::Create(dataStructure, "Vertices", verticesStore, geometryPtr->getId());
    facesPtr = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", facesStore, geometryPtr->getId());
    REQUIRE(verticesPtr != nullptr);
    REQUIRE(facesPtr != nullptr);
    geometryPtr->setVertices(*verticesPtr);
    geometryPtr->setFaceList(*facesPtr);
    auto verticesResult = verticesStore->copyFromBuffer(0, nonstd::span<const float32>(k_NormalVertices.data(), k_NormalVertices.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(verticesResult);
    std::vector<IGeometry::MeshIndexType> buffer(k_NormalBufferFaces * 3);
    for(usize firstFace = 0; firstFace < faceCount; firstFace += k_NormalBufferFaces)
    {
      const usize count = std::min(k_NormalBufferFaces, faceCount - firstFace);
      for(usize faceIndex = 0; faceIndex < count; ++faceIndex)
      {
        const auto& face = k_NormalFaces[NormalFixtureRow(firstFace + faceIndex)];
        std::copy(face.begin(), face.end(), buffer.begin() + faceIndex * 3);
      }
      auto result = facesStore->copyFromBuffer(firstFace * 3, nonstd::span<const IGeometry::MeshIndexType>(buffer.data(), count * 3));
      SIMPLNX_RESULT_REQUIRE_VALID(result);
    }
  }

  ~NormalFixture() = default;
  NormalFixture(const NormalFixture&) = default;
  NormalFixture(NormalFixture&&) = default;
  NormalFixture& operator=(const NormalFixture&) = default;
  NormalFixture& operator=(NormalFixture&&) = default;

  Arguments arguments() const
  {
    Arguments args;
    args.insertOrAssign(TriangleNormalFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(k_NormalGeometryPath));
    args.insertOrAssign(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayName_Key, std::make_any<std::string>("Normals"));
    return args;
  }

  Float64Array& output()
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(k_NormalOutputPath));
    return dataStructure.getDataRefAs<Float64Array>(k_NormalOutputPath);
  }
};

/**
 * @brief Checks every input and output value with bounded buffers, including an unpublished suffix.
 * @param fixture Owns the input arrays and output.
 * @param completedFaces Number of leading faces with computed normals; all remaining values retain the sentinel.
 */
void CheckNormalFixture(NormalFixture& fixture, usize completedFaces)
{
  auto& normals = fixture.output();
  const usize faceCount = fixture.facesPtr->getNumberOfTuples();
  REQUIRE(completedFaces <= faceCount);
  REQUIRE(fixture.geometryPtr->getVertices() == fixture.verticesPtr);
  REQUIRE(fixture.geometryPtr->getFaces() == fixture.facesPtr);
  REQUIRE(fixture.geometryPtr->getFaceAttributeMatrix() == fixture.faceDataPtr);
  REQUIRE(fixture.geometryPtr->getVertexAttributeMatrix() == fixture.vertexDataPtr);
  REQUIRE(fixture.vertexDataPtr->getShape() == ShapeType{6});
  REQUIRE(fixture.faceDataPtr->getShape() == ShapeType{faceCount});
  REQUIRE(fixture.verticesPtr->getTupleShape() == ShapeType{6});
  REQUIRE(fixture.verticesPtr->getComponentShape() == ShapeType{3});
  REQUIRE(fixture.facesPtr->getTupleShape() == ShapeType{faceCount});
  REQUIRE(fixture.facesPtr->getComponentShape() == ShapeType{3});
  REQUIRE(normals.getTupleShape() == ShapeType{faceCount});
  REQUIRE(normals.getComponentShape() == ShapeType{3});
  REQUIRE(normals.getSize() == faceCount * 3);
  std::array<float32, 18> vertices{};
  auto verticesResult = fixture.verticesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(vertices.data(), vertices.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesResult);
  REQUIRE(vertices == k_NormalVertices);
  std::vector<IGeometry::MeshIndexType> faces(k_NormalBufferFaces * 3);
  std::vector<float64> values(k_NormalBufferFaces * 3);
  for(usize firstFace = 0; firstFace < faceCount; firstFace += k_NormalBufferFaces)
  {
    const usize count = std::min(k_NormalBufferFaces, faceCount - firstFace);
    auto facesResult = fixture.facesPtr->getDataStoreRef().copyIntoBuffer(firstFace * 3, nonstd::span<IGeometry::MeshIndexType>(faces.data(), count * 3));
    auto normalsResult = normals.getDataStoreRef().copyIntoBuffer(firstFace * 3, nonstd::span<float64>(values.data(), count * 3));
    SIMPLNX_RESULT_REQUIRE_VALID(facesResult);
    SIMPLNX_RESULT_REQUIRE_VALID(normalsResult);
    for(usize valueIndex = 0; valueIndex < count * 3; ++valueIndex)
    {
      const usize faceIndex = firstFace + valueIndex / 3;
      const usize component = valueIndex % 3;
      const usize row = NormalFixtureRow(faceIndex);
      const float64 expected = faceIndex < completedFaces ? k_ExpectedNormals[row][component] : k_NormalSentinel;
      if(faces[valueIndex] != k_NormalFaces[row][component] || !(std::abs(values[valueIndex] - expected) <= 1.0e-14))
      {
        CAPTURE(faceIndex, component, faces[valueIndex], k_NormalFaces[row][component], values[valueIndex], expected);
        FAIL("Triangle normal values or input connectivity differ from the independent fixture.");
      }
    }
  }
  // These separate reads make the first face, block end, and one-face tail explicit witnesses.
  for(const usize faceIndex : {usize{0}, k_NormalBlockFaces - 1, k_NormalBlockFaces})
  {
    if(faceIndex >= faceCount)
    {
      continue;
    }
    std::array<float64, 3> normal{};
    auto result = normals.getDataStoreRef().copyIntoBuffer(faceIndex * 3, nonstd::span<float64>(normal.data(), normal.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(usize component = 0; component < 3; ++component)
    {
      const float64 expected = faceIndex < completedFaces ? k_ExpectedNormals[NormalFixtureRow(faceIndex)][component] : k_NormalSentinel;
      CAPTURE(faceIndex, component);
      CHECK(std::abs(normal[component] - expected) <= 1.0e-14);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

/**
 * @brief Validates filter inputs before supplying an output for the direct algorithm contract.
 * @param fixture Owns the accepted geometry and receives the sentinel output.
 * @param state Outlives the fixture and records the direct algorithm's transfers.
 * @return The created resident output array.
 */
Float64Array* AddNormalReportingOutput(NormalFixture& fixture, NormalTransferState& state)
{
  TriangleNormalFilter filter;
  REQUIRE(fixture.dataStructure.getData(k_NormalOutputPath) == nullptr);
  const auto preflight = filter.preflight(fixture.dataStructure, fixture.arguments());
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto store = std::make_shared<NormalReportingStore<float64>>(fixture.facesPtr->getNumberOfTuples(), state, NormalStoreRole::Normals);
  store->fill(k_NormalSentinel);
  auto* outputPtr = Float64Array::Create(fixture.dataStructure, "Normals", store, fixture.faceDataPtr->getId());
  REQUIRE(outputPtr != nullptr);
  return outputPtr;
}
} // namespace

TEST_CASE("SimplnxCore::TriangleNormalFilter: computation block tail", "[SimplnxCore][TriangleNormalFilter][N2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  NormalFixture fixture(k_NormalBlockFaces + 1);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  REQUIRE(fixture.dataStructure.getData(k_NormalOutputPath) == nullptr);
  TriangleNormalFilter filter;
  const auto args = fixture.arguments();
  const auto preflight = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  scope.requireExpectedStore(fixture.output());
  CheckNormalFixture(fixture, k_NormalBlockFaces + 1);
}

#if SIMPLNX_TEST_ALGORITHM_PATH != 2
TEST_CASE("SimplnxCore::TriangleNormal: block transfer contracts", "[SimplnxCore][TriangleNormalFilter][N2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenarios({UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore})));
  const usize caseIndex = GENERATE(usize{0}, usize{1}, usize{2}, usize{3}, usize{4}, usize{5}, usize{6});
  struct ContractCase
  {
    const char* name;
    NormalStoreRole failingRole;
    bool failingWrite;
    usize failingCall;
    usize traceLength;
    usize completedFaces;
    int32 contextCode;
    const char* contextMessage;
  };
  const std::array<ContractCase, 7> cases = {
      {{"complete", NormalStoreRole::Normals, false, 0, 5, k_NormalBlockFaces + 1, 0, ""},
       {"vertex read", NormalStoreRole::Vertices, false, 1, 1, 0, -65810, "Error reading vertex coordinates for triangle normal calculation."},
       {"first face read", NormalStoreRole::Faces, false, 1, 2, 0, -65811, "Error reading connectivity for triangle normal calculation at triangle 0 (262144 triangles)."},
       {"tail face read", NormalStoreRole::Faces, false, 2, 4, k_NormalBlockFaces, -65811, "Error reading connectivity for triangle normal calculation at triangle 262144 (1 triangles)."},
       {"first output write", NormalStoreRole::Normals, true, 1, 3, 0, -65812, "Error writing normals at triangle 0 (262144 triangles)."},
       {"tail output write", NormalStoreRole::Normals, true, 2, 5, k_NormalBlockFaces, -65812, "Error writing normals at triangle 262144 (1 triangles)."},
       {"cancel after first block", NormalStoreRole::Normals, false, 0, 3, k_NormalBlockFaces, 0, ""}}};
  const auto& contract = cases[caseIndex];
  CAPTURE(scenario, contract.name);
  UnitTest::AlgorithmTestScope scope(scenario);
  NormalTransferState state;
  NormalFixture fixture(k_NormalBlockFaces + 1, &state);
  auto* outputPtr = AddNormalReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  scope.requireExpectedStore(*outputPtr);
  state.failingRole = contract.failingRole;
  state.failingWrite = contract.failingWrite;
  state.failingCall = contract.failingCall;
  state.cancelAfterFirstWrite = caseIndex == 6;
  TriangleNormalInputValues inputValues{k_NormalGeometryPath, "Normals"};
  const IFilter::MessageHandler messages{};
  NormalTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleNormal(fixture.dataStructure, messages, state.shouldCancel, &inputValues)(); });
  const auto calls = state.calls;
  state.armed = false;
  REQUIRE(result.warnings().empty());
  if(contract.failingCall == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 2);
    CHECK(result.errors()[0].code == k_NormalTransferError);
    CHECK(result.errors()[0].message == NormalFailureMessage(contract.failingRole));
    CHECK(result.errors()[1].code == contract.contextCode);
    CHECK(result.errors()[1].message == contract.contextMessage);
  }
  CHECK(state.shouldCancel.load() == state.cancelAfterFirstWrite);
  const std::array<NormalTransfer, 5> expected = {{{NormalStoreRole::Vertices, false, 0, 18},
                                                   {NormalStoreRole::Faces, false, 0, 786432},
                                                   {NormalStoreRole::Normals, true, 0, 786432},
                                                   {NormalStoreRole::Faces, false, 786432, 3},
                                                   {NormalStoreRole::Normals, true, 786432, 3}}};
  REQUIRE(calls.size() == contract.traceLength);
  for(usize callIndex = 0; callIndex < calls.size(); ++callIndex)
  {
    CAPTURE(callIndex);
    CHECK(calls[callIndex].role == expected[callIndex].role);
    CHECK(calls[callIndex].write == expected[callIndex].write);
    CHECK(calls[callIndex].start == expected[callIndex].start);
    CHECK(calls[callIndex].count == expected[callIndex].count);
  }
  CheckNormalFixture(fixture, contract.completedFaces);
}
#endif

TEST_CASE("SimplnxCore::TriangleNormal: early exit controls", "[SimplnxCore][TriangleNormalFilter][N2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool preCancelled = GENERATE(false, true);
  CAPTURE(scenario, preCancelled);
  UnitTest::AlgorithmTestScope scope(scenario);
  NormalTransferState state;
  NormalFixture fixture(preCancelled ? 4 : 0, &state);
  auto* outputPtr = AddNormalReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  scope.requireExpectedStore(*outputPtr);
  state.shouldCancel = preCancelled;
  TriangleNormalInputValues inputValues{k_NormalGeometryPath, "Normals"};
  const IFilter::MessageHandler messages{};
  NormalTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleNormal(fixture.dataStructure, messages, state.shouldCancel, &inputValues)(); });
  const auto calls = state.calls;
  state.armed = false;
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.warnings().empty());
  CHECK(calls.empty());
  CHECK(state.shouldCancel.load() == preCancelled);
  CheckNormalFixture(fixture, 0);
}

TEST_CASE("SimplnxCore::TriangleNormalFilter: empty geometry", "[SimplnxCore][TriangleNormalFilter][N2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  NormalFixture fixture(0);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  REQUIRE(fixture.dataStructure.getData(k_NormalOutputPath) == nullptr);
  TriangleNormalFilter filter;
  const auto args = fixture.arguments();
  const auto preflight = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  scope.requireExpectedStore(fixture.output());
  CheckNormalFixture(fixture, 0);
}

#if SIMPLNX_TEST_ALGORITHM_PATH != 2
TEST_CASE("SimplnxCore::TriangleNormal: buffered invalid connectivity", "[SimplnxCore][TriangleNormalFilter][N2][InvalidConnectivity]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenarios({UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore})));
  const auto invalidVertex = GENERATE(IGeometry::MeshIndexType{3}, std::numeric_limits<IGeometry::MeshIndexType>::max());
  const usize invalidFace = GENERATE(usize{0}, usize{1});
  const usize invalidCorner = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(scenario, invalidVertex, invalidFace, invalidCorner);
  UnitTest::AlgorithmTestScope scope(scenario);
  NormalTransferState state;
  NormalFixture fixture(2, &state);
  auto resizeVertices = fixture.geometryPtr->resizeVertexList(3);
  auto resizeVertexData = fixture.vertexDataPtr->resizeTuples({3});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeVertices);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeVertexData);
  // Three vertices admit only IDs 0, 1, and 2. Every corner must fail before offset arithmetic.
  std::array<IGeometry::MeshIndexType, 6> faces = {0, 1, 2, 0, 1, 2};
  faces.at((invalidFace * 3) + invalidCorner) = invalidVertex;
  auto facesWrite = fixture.facesPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const IGeometry::MeshIndexType>(faces.data(), faces.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(facesWrite);
  auto* outputPtr = AddNormalReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  scope.requireExpectedStore(*outputPtr);
  TriangleNormalInputValues inputValues{.InputTriangleGeometryPath = k_NormalGeometryPath, .OutputNormalsArrayName = "Normals"};
  const IFilter::MessageHandler messages{};
  const NormalTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleNormal(fixture.dataStructure, messages, state.shouldCancel, &inputValues)(); });
  state.armed = false;
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -65813);
  CHECK(result.errors()[0].message == fmt::format("Triangle {} references vertex index {}, but the vertex tuple count is 3. Valid vertex indices are less than 3.", invalidFace, invalidVertex));
  CHECK(result.warnings().empty());
  CHECK(state.writes[static_cast<usize>(NormalStoreRole::Normals)] == 0);
  std::array<float64, 6> actual{};
  auto outputRead = outputPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(outputRead);
  for(const float64 value : actual)
  {
    CHECK(value == k_NormalSentinel);
  }
  std::array<IGeometry::MeshIndexType, 6> actualFaces{};
  auto facesRead = fixture.facesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<IGeometry::MeshIndexType>(actualFaces.data(), actualFaces.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(facesRead);
  CHECK(actualFaces == faces);
  std::array<float32, 9> actualVertices{};
  auto verticesRead = fixture.verticesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actualVertices.data(), actualVertices.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesRead);
  CHECK(std::equal(actualVertices.begin(), actualVertices.end(), k_NormalVertices.begin()));
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}
#endif