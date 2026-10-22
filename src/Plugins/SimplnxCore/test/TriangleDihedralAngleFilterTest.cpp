#include "SimplnxCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/TriangleDihedralAngle.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

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
constexpr usize k_VertexTupleCount = 11;
constexpr usize k_VertexCompCount = 3;
constexpr usize k_FaceTupleCount = 4;
constexpr usize k_FaceCompCount = 3;

static const IGeometry::SharedVertexList* createVertexList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto dataStructure = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_VertexTupleCount}, std::vector<usize>{k_VertexCompCount}, 0.0f);
  auto* dataArr = IGeometry::SharedVertexList::Create(*dataStructure, "Vertices", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedVertexList*>(dataArr);
}

static const IGeometry::SharedFaceList* createFaceList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto dataStructure = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{k_FaceTupleCount}, std::vector<usize>{k_FaceCompCount}, 0);
  auto* dataArr = IGeometry::SharedFaceList::Create(*dataStructure, "Faces", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedFaceList*>(dataArr);
}

} // namespace

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter[valid results]", "[SimplnxCore][TriangleDihedralAngleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "DihedralAngles";
  DataStructure dataStructure;
  TriangleGeom& acuteTriangle = *TriangleGeom::Create(dataStructure, triangleGeometryName);
  AttributeMatrix* faceData = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceAttributeMatrixName, {k_FaceTupleCount}, acuteTriangle.getId());
  acuteTriangle.setFaceAttributeMatrix(*faceData);
  AttributeMatrix* vertData = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, {k_VertexTupleCount}, acuteTriangle.getId());
  acuteTriangle.setVertexAttributeMatrix(*vertData);
  auto vertexList = createVertexList(acuteTriangle, vertData->getId());
  auto facesList = createFaceList(acuteTriangle, faceData->getId());
  auto underlyingStoreV = vertexList->getDataStorePtr().lock();
  auto underlyingStoreF = facesList->getDataStorePtr().lock();

  // load Vertex list
  std::vector<float32> vertexes = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 3.0f, 0.0f,  0.0f, 4.0f, 1.0f,
                                   0.0f, 6.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 9.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f};
  auto count = 0;
  for(auto element : vertexes)
  {
    underlyingStoreV->setValue(count++, element);
  }
  // load face list
  std::vector<IGeometry::MeshIndexType> faces = {0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10};
  count = 0;
  for(auto element : faces)
  {
    underlyingStoreF->setValue(count++, element);
  }
  acuteTriangle.setFaceList(*facesList);
  acuteTriangle.setVertices(*vertexList);

  TriangleDihedralAngleFilter filter;
  Arguments args;
  std::string triangleDihedralAnglesName = "Triangle Dihedral Angles";

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];
  DataPath triangleDihedralAnglesDataPath = geometryPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName).createChildPath(triangleDihedralAnglesName);
  const auto* verticesArray = acuteTriangle.getVertices();
  const auto* facesArray = acuteTriangle.getFaces();
  REQUIRE(verticesArray != nullptr);
  REQUIRE(facesArray != nullptr);
  scope.requireExpectedStore(*verticesArray);
  scope.requireExpectedStore(*facesArray);

  // Create default Parameters for the filter.
  args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>(triangleDihedralAnglesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(triangleDihedralAnglesDataPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(triangleDihedralAnglesDataPath));

  std::vector<float64> officialDihedralAngleMinimums = {53.1301002502441, 18.4349555969238, 0.0, 0.0};
  Float64Array& calculatedDihedralAngleMinimums = dataStructure.getDataRefAs<Float64Array>(triangleDihedralAnglesDataPath);

  auto debug1 = calculatedDihedralAngleMinimums[0];
  auto debug2 = calculatedDihedralAngleMinimums[1];
  auto debug3 = calculatedDihedralAngleMinimums[2];
  auto debug4 = calculatedDihedralAngleMinimums[3];

  auto result1 = fabs(officialDihedralAngleMinimums[0] - calculatedDihedralAngleMinimums[0]);
  REQUIRE(result1 < ::k_max_difference);
  auto result2 = fabs(officialDihedralAngleMinimums[1] - calculatedDihedralAngleMinimums[1]);
  REQUIRE(result2 < ::k_max_difference);
  REQUIRE(std::isnan(calculatedDihedralAngleMinimums[2]));
  auto result4 = fabs(officialDihedralAngleMinimums[3] - calculatedDihedralAngleMinimums[3]);
  REQUIRE(result4 < ::k_max_difference);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][TriangleDihedralAngleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "TriangleDihedralAngleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "TriangleDihedralAngleFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<TriangleDihedralAngleFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key) == "TestArray");
      CHECK(args.value<DataPath>(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key) == DataPath({"DataContainer"}));
    }
  }
}

namespace
{
constexpr usize k_DihedralBlockFaces = 262144;
constexpr usize k_DihedralBufferFaces = 4096;
constexpr float64 k_DihedralSentinel = -37.0;
constexpr int32 k_DihedralTransferError = -98761;
const DataPath k_DihedralGeometryPath({"DihedralMesh"});
const DataPath k_DihedralVerticesPath = k_DihedralGeometryPath.createChildPath("Vertices");
const DataPath k_DihedralFacesPath = k_DihedralGeometryPath.createChildPath("Faces");
const DataPath k_DihedralOutputPath = k_DihedralGeometryPath.createChildPath("FaceData").createChildPath("Angles");
constexpr std::array<float32, 33> k_DihedralVertices = {0, 0, 0, 1, 0, 0, 0.5f, 1, 0, 2, 0, 0, 3, 0, 0, 4, 1, 0, 6, 0, 0, 7, 0, 0, 8, 0, 0, 9, 0, 0, 10, 0, 0};
constexpr std::array<std::array<IGeometry::MeshIndexType, 3>, 4> k_DihedralFaces = {{{0, 1, 2}, {3, 4, 5}, {6, 7, 7}, {8, 9, 10}}};
// The finite angles in degrees are 2*atan(1/2), atan(1/3), and zero for the collinear face.
// The repeated-vertex face must produce NaN, which has a separate comparison below.
constexpr std::array<float64, 4> k_ExpectedAngles = {53.13010235415598, 18.43494882292201, 0.0, 0.0};

usize DihedralFixtureRow(usize faceIndex)
{
  return faceIndex == k_DihedralBlockFaces ? 1 : faceIndex % 4;
}

/**
 * @brief Compares one angle without accepting NaN as a finite or unpublished value.
 * @param actual Stored angle, in degrees.
 * @param row Identifies the independent triangle fixture.
 * @param completed Selects a computed angle or the unchanged output sentinel.
 * @return True if the scalar has the required value or NaN classification.
 */
bool MatchesDihedralAngle(float64 actual, usize row, bool completed)
{
  if(!completed)
  {
    return actual == k_DihedralSentinel;
  }
  if(row == 2)
  {
    return std::isnan(actual);
  }
  if(row == 3)
  {
    return actual == 0.0;
  }
  return std::isfinite(actual) && std::abs(actual - k_ExpectedAngles[row]) <= 1.0e-10;
}

enum class DihedralStoreRole : usize
{
  Vertices,
  Faces,
  Angles
};

struct DihedralTransfer
{
  DihedralStoreRole role;
  bool write;
  usize start;
  usize count;
};

struct DihedralTransferState
{
  bool armed = false;
  DihedralStoreRole failingRole = DihedralStoreRole::Angles;
  bool failingWrite = false;
  usize failingCall = 0;
  bool cancelAfterFirstWrite = false;
  std::atomic_bool shouldCancel{false};
  std::array<usize, 3> reads{};
  std::array<usize, 3> writes{};
  std::vector<DihedralTransfer> calls;
};

const char* DihedralFailureMessage(DihedralStoreRole role)
{
  switch(role)
  {
  case DihedralStoreRole::Vertices:
    return "Injected triangle dihedral angle vertex read failure.";
  case DihedralStoreRole::Faces:
    return "Injected triangle dihedral angle connectivity read failure.";
  case DihedralStoreRole::Angles:
    return "Injected triangle dihedral angle output write failure.";
  }
  return "Unknown triangle dihedral angle transfer failure.";
}

/**
 * @class DihedralReportingStore
 * @brief Records sequential bulk transfers and rejects selected calls before touching resident values.
 * @tparam T Stored scalar type.
 * @note The borrowed state outlives the store. This fixture does not model partial backend writes.
 */
template <typename T>
class DihedralReportingStore final : public DataStore<T>
{
public:
  /**
   * @brief Creates real resident storage with a disarmed, longer-lived trace.
   * @param tuples Number of tuples; output tuples have one component.
   * @param state Records target transfers only.
   * @param role Identifies the array in each transfer.
   */
  DihedralReportingStore(usize tuples, DihedralTransferState& state, DihedralStoreRole role)
  : DataStore<T>(ShapeType{tuples}, ShapeType{role == DihedralStoreRole::Angles ? usize{1} : usize{3}}, T{})
  , m_State(state)
  , m_Role(role)
  {
  }

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
        return MakeErrorResult(k_DihedralTransferError, DihedralFailureMessage(m_Role));
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
      return MakeErrorResult(k_DihedralTransferError, DihedralFailureMessage(m_Role));
    }
    auto result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && m_Role == DihedralStoreRole::Angles && call == 1 && m_State.cancelAfterFirstWrite)
    {
      m_State.shouldCancel = true;
    }
    return result;
  }

private:
  DihedralTransferState& m_State;
  const DihedralStoreRole m_Role;
};

/**
 * @struct DihedralTraceGuard
 * @brief Limits recording to the target call and disarms on assertion failure.
 */
struct DihedralTraceGuard
{
  /**
   * @brief Clears setup activity and starts the target transfer trace.
   * @param state Outlives this guard and its reporting stores.
   */
  explicit DihedralTraceGuard(DihedralTransferState& state)
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
  ~DihedralTraceGuard()
  {
    stateRef.armed = false;
  }

  DihedralTraceGuard(const DihedralTraceGuard&) = delete;
  DihedralTraceGuard(DihedralTraceGuard&&) = delete;
  DihedralTraceGuard& operator=(const DihedralTraceGuard&) = delete;
  DihedralTraceGuard& operator=(DihedralTraceGuard&&) = delete;

  DihedralTransferState& stateRef;
};

/**
 * @struct DihedralFixture
 * @brief Supplies eleven shared vertices and repeated triangles with a distinct final block tail.
 */
struct DihedralFixture
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
  explicit DihedralFixture(usize faceCount, DihedralTransferState* state = nullptr)
  {
    geometryPtr = TriangleGeom::Create(dataStructure, k_DihedralGeometryPath.getTargetName());
    REQUIRE(geometryPtr != nullptr);
    faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {faceCount}, geometryPtr->getId());
    vertexDataPtr = AttributeMatrix::Create(dataStructure, "VertexData", {11}, geometryPtr->getId());
    REQUIRE(faceDataPtr != nullptr);
    REQUIRE(vertexDataPtr != nullptr);
    geometryPtr->setFaceAttributeMatrix(*faceDataPtr);
    geometryPtr->setVertexAttributeMatrix(*vertexDataPtr);
    std::shared_ptr<AbstractDataStore<float32>> verticesStore;
    std::shared_ptr<AbstractDataStore<IGeometry::MeshIndexType>> facesStore;
    if(state != nullptr)
    {
      verticesStore = std::make_shared<DihedralReportingStore<float32>>(11, *state, DihedralStoreRole::Vertices);
      facesStore = std::make_shared<DihedralReportingStore<IGeometry::MeshIndexType>>(faceCount, *state, DihedralStoreRole::Faces);
    }
    else
    {
      verticesStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_DihedralVerticesPath, {11}, {3});
      facesStore = DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, k_DihedralFacesPath, {faceCount}, {3});
    }
    REQUIRE(verticesStore != nullptr);
    REQUIRE(facesStore != nullptr);
    verticesPtr = Float32Array::Create(dataStructure, "Vertices", verticesStore, geometryPtr->getId());
    facesPtr = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", facesStore, geometryPtr->getId());
    REQUIRE(verticesPtr != nullptr);
    REQUIRE(facesPtr != nullptr);
    geometryPtr->setVertices(*verticesPtr);
    geometryPtr->setFaceList(*facesPtr);
    auto verticesResult = verticesStore->copyFromBuffer(0, nonstd::span<const float32>(k_DihedralVertices.data(), k_DihedralVertices.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(verticesResult);
    std::vector<IGeometry::MeshIndexType> buffer(k_DihedralBufferFaces * 3);
    for(usize firstFace = 0; firstFace < faceCount; firstFace += k_DihedralBufferFaces)
    {
      const usize count = std::min(k_DihedralBufferFaces, faceCount - firstFace);
      for(usize faceIndex = 0; faceIndex < count; ++faceIndex)
      {
        const auto& face = k_DihedralFaces[DihedralFixtureRow(firstFace + faceIndex)];
        std::copy(face.begin(), face.end(), buffer.begin() + faceIndex * 3);
      }
      auto result = facesStore->copyFromBuffer(firstFace * 3, nonstd::span<const IGeometry::MeshIndexType>(buffer.data(), count * 3));
      SIMPLNX_RESULT_REQUIRE_VALID(result);
    }
  }

  Arguments arguments() const
  {
    Arguments args;
    args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(k_DihedralGeometryPath));
    args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>("Angles"));
    return args;
  }

  Float64Array& output()
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(k_DihedralOutputPath));
    return dataStructure.getDataRefAs<Float64Array>(k_DihedralOutputPath);
  }
};

/**
 * @brief Checks all input values and angles in bounded buffers, including an unpublished suffix.
 * @param fixture Owns the input arrays and output.
 * @param completedFaces Number of leading faces with computed angles; all remaining values retain the sentinel.
 */
void CheckDihedralFixture(DihedralFixture& fixture, usize completedFaces)
{
  auto& angles = fixture.output();
  const usize faceCount = fixture.facesPtr->getNumberOfTuples();
  REQUIRE(completedFaces <= faceCount);
  REQUIRE(fixture.geometryPtr->getVertices() == fixture.verticesPtr);
  REQUIRE(fixture.geometryPtr->getFaces() == fixture.facesPtr);
  REQUIRE(fixture.geometryPtr->getFaceAttributeMatrix() == fixture.faceDataPtr);
  REQUIRE(fixture.geometryPtr->getVertexAttributeMatrix() == fixture.vertexDataPtr);
  REQUIRE(fixture.vertexDataPtr->getShape() == ShapeType{11});
  REQUIRE(fixture.faceDataPtr->getShape() == ShapeType{faceCount});
  REQUIRE(fixture.verticesPtr->getTupleShape() == ShapeType{11});
  REQUIRE(fixture.verticesPtr->getComponentShape() == ShapeType{3});
  REQUIRE(fixture.facesPtr->getTupleShape() == ShapeType{faceCount});
  REQUIRE(fixture.facesPtr->getComponentShape() == ShapeType{3});
  REQUIRE(angles.getTupleShape() == ShapeType{faceCount});
  REQUIRE(angles.getComponentShape() == ShapeType{1});
  REQUIRE(angles.getSize() == faceCount);
  std::array<float32, 33> vertices{};
  auto verticesResult = fixture.verticesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(vertices.data(), vertices.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesResult);
  REQUIRE(vertices == k_DihedralVertices);
  std::vector<IGeometry::MeshIndexType> faces(k_DihedralBufferFaces * 3);
  std::vector<float64> values(k_DihedralBufferFaces);
  for(usize firstFace = 0; firstFace < faceCount; firstFace += k_DihedralBufferFaces)
  {
    const usize count = std::min(k_DihedralBufferFaces, faceCount - firstFace);
    auto facesResult = fixture.facesPtr->getDataStoreRef().copyIntoBuffer(firstFace * 3, nonstd::span<IGeometry::MeshIndexType>(faces.data(), count * 3));
    auto anglesResult = angles.getDataStoreRef().copyIntoBuffer(firstFace, nonstd::span<float64>(values.data(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(facesResult);
    SIMPLNX_RESULT_REQUIRE_VALID(anglesResult);
    for(usize localFace = 0; localFace < count; ++localFace)
    {
      const usize faceIndex = firstFace + localFace;
      const usize row = DihedralFixtureRow(faceIndex);
      for(usize component = 0; component < 3; ++component)
      {
        if(faces[localFace * 3 + component] != k_DihedralFaces[row][component])
        {
          CAPTURE(faceIndex, component, faces[localFace * 3 + component], k_DihedralFaces[row][component]);
          FAIL("Triangle dihedral input connectivity changed.");
        }
      }
      if(!MatchesDihedralAngle(values[localFace], row, faceIndex < completedFaces))
      {
        CAPTURE(faceIndex, row, values[localFace], completedFaces);
        FAIL("Triangle dihedral angle differs from the independent fixture or unpublished sentinel.");
      }
    }
  }
  // Separate reads check the first face, block end, and distinct one-face tail.
  for(const usize faceIndex : {usize{0}, k_DihedralBlockFaces - 1, k_DihedralBlockFaces})
  {
    if(faceIndex >= faceCount)
    {
      continue;
    }
    float64 angle = k_DihedralSentinel;
    auto result = angles.getDataStoreRef().copyIntoBuffer(faceIndex, nonstd::span<float64>(&angle, 1));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CAPTURE(faceIndex, angle, completedFaces);
    CHECK(MatchesDihedralAngle(angle, DihedralFixtureRow(faceIndex), faceIndex < completedFaces));
  }
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

/**
 * @brief Validates filter inputs before supplying an output for the direct algorithm contract.
 * @param fixture Owns the accepted geometry and receives the sentinel output.
 * @param state Outlives the fixture and records the direct algorithm's transfers.
 * @return The created resident output array.
 */
Float64Array* AddDihedralReportingOutput(DihedralFixture& fixture, DihedralTransferState& state)
{
  TriangleDihedralAngleFilter filter;
  REQUIRE(fixture.dataStructure.getData(k_DihedralOutputPath) == nullptr);
  const auto preflight = filter.preflight(fixture.dataStructure, fixture.arguments());
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto store = std::make_shared<DihedralReportingStore<float64>>(fixture.facesPtr->getNumberOfTuples(), state, DihedralStoreRole::Angles);
  store->fill(k_DihedralSentinel);
  auto* outputPtr = Float64Array::Create(fixture.dataStructure, "Angles", store, fixture.faceDataPtr->getId());
  REQUIRE(outputPtr != nullptr);
  return outputPtr;
}
} // namespace

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: computation block tail", "[SimplnxCore][TriangleDihedralAngleFilter][D2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralFixture fixture(k_DihedralBlockFaces + 1);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  REQUIRE(fixture.dataStructure.getData(k_DihedralOutputPath) == nullptr);
  TriangleDihedralAngleFilter filter;
  const auto args = fixture.arguments();
  const auto preflight = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  scope.requireExpectedStore(fixture.output());
  CheckDihedralFixture(fixture, k_DihedralBlockFaces + 1);
}

#if SIMPLNX_TEST_ALGORITHM_PATH != 2
TEST_CASE("SimplnxCore::TriangleDihedralAngle: block transfer contracts", "[SimplnxCore][TriangleDihedralAngleFilter][D2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenarios({UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore})));
  const usize caseIndex = GENERATE(usize{0}, usize{1}, usize{2}, usize{3}, usize{4}, usize{5}, usize{6});
  struct ContractCase
  {
    const char* name;
    DihedralStoreRole failingRole;
    bool failingWrite;
    usize failingCall;
    usize traceLength;
    usize completedFaces;
    int32 contextCode;
    const char* contextMessage;
  };
  const std::array<ContractCase, 7> cases = {
      {{"complete", DihedralStoreRole::Angles, false, 0, 5, k_DihedralBlockFaces + 1, 0, ""},
       {"vertex read", DihedralStoreRole::Vertices, false, 1, 1, 0, -65820, "Error reading vertex coordinates for triangle dihedral angle calculation."},
       {"first face read", DihedralStoreRole::Faces, false, 1, 2, 0, -65821, "Error reading connectivity for triangle dihedral angle calculation at triangle 0 (262144 triangles)."},
       {"tail face read", DihedralStoreRole::Faces, false, 2, 4, k_DihedralBlockFaces, -65821, "Error reading connectivity for triangle dihedral angle calculation at triangle 262144 (1 triangles)."},
       {"first output write", DihedralStoreRole::Angles, true, 1, 3, 0, -65822, "Error writing dihedral angles at triangle 0 (262144 triangles)."},
       {"tail output write", DihedralStoreRole::Angles, true, 2, 5, k_DihedralBlockFaces, -65822, "Error writing dihedral angles at triangle 262144 (1 triangles)."},
       {"cancel after first block", DihedralStoreRole::Angles, false, 0, 3, k_DihedralBlockFaces, 0, ""}}};
  const auto& contract = cases[caseIndex];
  CAPTURE(scenario, contract.name);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralTransferState state;
  DihedralFixture fixture(k_DihedralBlockFaces + 1, &state);
  auto* outputPtr = AddDihedralReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  scope.requireExpectedStore(*outputPtr);
  state.failingRole = contract.failingRole;
  state.failingWrite = contract.failingWrite;
  state.failingCall = contract.failingCall;
  state.cancelAfterFirstWrite = caseIndex == 6;
  TriangleDihedralAngleInputValues inputValues{k_DihedralGeometryPath, "Angles"};
  const IFilter::MessageHandler messages{};
  DihedralTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleDihedralAngle(fixture.dataStructure, messages, state.shouldCancel, &inputValues)(); });
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
    CHECK(result.errors()[0].code == k_DihedralTransferError);
    CHECK(result.errors()[0].message == DihedralFailureMessage(contract.failingRole));
    CHECK(result.errors()[1].code == contract.contextCode);
    CHECK(result.errors()[1].message == contract.contextMessage);
  }
  CHECK(state.shouldCancel.load() == state.cancelAfterFirstWrite);
  const std::array<DihedralTransfer, 5> expected = {{{DihedralStoreRole::Vertices, false, 0, 33},
                                                     {DihedralStoreRole::Faces, false, 0, 786432},
                                                     {DihedralStoreRole::Angles, true, 0, 262144},
                                                     {DihedralStoreRole::Faces, false, 786432, 3},
                                                     {DihedralStoreRole::Angles, true, 262144, 1}}};
  REQUIRE(calls.size() == contract.traceLength);
  for(usize callIndex = 0; callIndex < calls.size(); ++callIndex)
  {
    CAPTURE(callIndex);
    CHECK(calls[callIndex].role == expected[callIndex].role);
    CHECK(calls[callIndex].write == expected[callIndex].write);
    CHECK(calls[callIndex].start == expected[callIndex].start);
    CHECK(calls[callIndex].count == expected[callIndex].count);
  }
  CheckDihedralFixture(fixture, contract.completedFaces);
}
#endif

TEST_CASE("SimplnxCore::TriangleDihedralAngle: early exit controls", "[SimplnxCore][TriangleDihedralAngleFilter][D2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool preCancelled = GENERATE(false, true);
  CAPTURE(scenario, preCancelled);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralTransferState state;
  DihedralFixture fixture(preCancelled ? 4 : 0, &state);
  auto* outputPtr = AddDihedralReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  scope.requireExpectedStore(*outputPtr);
  state.shouldCancel = preCancelled;
  TriangleDihedralAngleInputValues inputValues{k_DihedralGeometryPath, "Angles"};
  const IFilter::MessageHandler messages{};
  DihedralTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleDihedralAngle(fixture.dataStructure, messages, state.shouldCancel, &inputValues)(); });
  const auto calls = state.calls;
  state.armed = false;
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.warnings().empty());
  CHECK(calls.empty());
  CHECK(state.shouldCancel.load() == preCancelled);
  CheckDihedralFixture(fixture, 0);
}

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: empty geometry", "[SimplnxCore][TriangleDihedralAngleFilter][D2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralFixture fixture(0);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  REQUIRE(fixture.dataStructure.getData(k_DihedralOutputPath) == nullptr);
  TriangleDihedralAngleFilter filter;
  const auto args = fixture.arguments();
  const auto preflight = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  scope.requireExpectedStore(fixture.output());
  CheckDihedralFixture(fixture, 0);
}

#if SIMPLNX_TEST_ALGORITHM_PATH != 2
TEST_CASE("SimplnxCore::TriangleDihedralAngle: buffered invalid connectivity", "[SimplnxCore][TriangleDihedralAngleFilter][D2][InvalidConnectivity]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenarios({UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore})));
  const auto invalidVertex = GENERATE(IGeometry::MeshIndexType{3}, std::numeric_limits<IGeometry::MeshIndexType>::max());
  const usize invalidFace = GENERATE(usize{0}, usize{1});
  const usize invalidCorner = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(scenario, invalidVertex, invalidFace, invalidCorner);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralTransferState state;
  DihedralFixture fixture(2, &state);
  auto resizeVertices = fixture.geometryPtr->resizeVertexList(3);
  auto resizeVertexData = fixture.vertexDataPtr->resizeTuples({3});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeVertices);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeVertexData);
  // Three vertices admit only IDs 0, 1, and 2. Every corner must fail before offset arithmetic.
  std::array<IGeometry::MeshIndexType, 6> faces = {0, 1, 2, 0, 1, 2};
  faces.at((invalidFace * 3) + invalidCorner) = invalidVertex;
  auto facesWrite = fixture.facesPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const IGeometry::MeshIndexType>(faces.data(), faces.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(facesWrite);
  auto* outputPtr = AddDihedralReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.verticesPtr);
  scope.requireExpectedStore(*fixture.facesPtr);
  scope.requireExpectedStore(*outputPtr);
  TriangleDihedralAngleInputValues inputValues{.InputTriangleGeometryPath = k_DihedralGeometryPath, .SurfaceMeshTriangleDihedralAnglesArrayName = "Angles"};
  const IFilter::MessageHandler messages{};
  const DihedralTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleDihedralAngle(fixture.dataStructure, messages, state.shouldCancel, &inputValues)(); });
  state.armed = false;
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -65823);
  CHECK(result.errors()[0].message == fmt::format("Triangle {} references vertex index {}, but the vertex tuple count is 3. Valid vertex indices are less than 3.", invalidFace, invalidVertex));
  CHECK(result.warnings().empty());
  CHECK(state.writes[static_cast<usize>(DihedralStoreRole::Angles)] == 0);
  std::array<float64, 2> actual{};
  auto outputRead = outputPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(outputRead);
  for(const float64 value : actual)
  {
    CHECK(value == k_DihedralSentinel);
  }
  std::array<IGeometry::MeshIndexType, 6> actualFaces{};
  auto facesRead = fixture.facesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<IGeometry::MeshIndexType>(actualFaces.data(), actualFaces.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(facesRead);
  CHECK(actualFaces == faces);
  std::array<float32, 9> actualVertices{};
  auto verticesRead = fixture.verticesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actualVertices.data(), actualVertices.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesRead);
  CHECK(std::equal(actualVertices.begin(), actualVertices.end(), k_DihedralVertices.begin()));
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}
#endif