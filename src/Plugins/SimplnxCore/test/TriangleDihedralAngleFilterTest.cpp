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
constexpr float64 k_MaxDifference = 0.0001;
constexpr usize k_VertexTupleCount = 11;
constexpr usize k_VertexCompCount = 3;
constexpr usize k_FaceTupleCount = 4;
constexpr usize k_FaceCompCount = 3;

const IGeometry::SharedVertexList* CreateVertexList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto* dataStructurePtr = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_VertexTupleCount}, std::vector<usize>{k_VertexCompCount}, 0.0f);
  auto* dataArrPtr = IGeometry::SharedVertexList::Create(*dataStructurePtr, "Vertices", std::move(dataStore), parentId);
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<IGeometry::SharedVertexList*>(dataArrPtr);
}

const IGeometry::SharedFaceList* CreateFaceList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto* dataStructurePtr = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{k_FaceTupleCount}, std::vector<usize>{k_FaceCompCount}, 0);
  auto* dataArrPtr = IGeometry::SharedFaceList::Create(*dataStructurePtr, "Faces", std::move(dataStore), parentId);
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<IGeometry::SharedFaceList*>(dataArrPtr);
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
  AttributeMatrix* faceDataPtr = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceAttributeMatrixName, {k_FaceTupleCount}, acuteTriangle.getId());
  acuteTriangle.setFaceAttributeMatrix(*faceDataPtr);
  AttributeMatrix* vertDataPtr = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, {k_VertexTupleCount}, acuteTriangle.getId());
  acuteTriangle.setVertexAttributeMatrix(*vertDataPtr);
  const auto* vertexListPtr = CreateVertexList(acuteTriangle, vertDataPtr->getId());
  const auto* facesListPtr = CreateFaceList(acuteTriangle, faceDataPtr->getId());
  auto underlyingStoreV = vertexListPtr->getDataStorePtr().lock();
  auto underlyingStoreF = facesListPtr->getDataStorePtr().lock();

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
  acuteTriangle.setFaceList(*facesListPtr);
  acuteTriangle.setVertices(*vertexListPtr);

  TriangleDihedralAngleFilter filter;
  Arguments args;
  std::string triangleDihedralAnglesName = "Triangle Dihedral Angles";

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];
  DataPath triangleDihedralAnglesDataPath = geometryPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName).createChildPath(triangleDihedralAnglesName);
  const auto* verticesArrayPtr = acuteTriangle.getVertices();
  const auto* facesArrayPtr = acuteTriangle.getFaces();
  REQUIRE(verticesArrayPtr != nullptr);
  REQUIRE(facesArrayPtr != nullptr);
  scope.requireExpectedStore(*verticesArrayPtr);
  scope.requireExpectedStore(*facesArrayPtr);

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
  REQUIRE(result1 < ::k_MaxDifference);
  auto result2 = fabs(officialDihedralAngleMinimums[1] - calculatedDihedralAngleMinimums[1]);
  REQUIRE(result2 < ::k_MaxDifference);
  REQUIRE(std::isnan(calculatedDihedralAngleMinimums[2]));
  auto result4 = fabs(officialDihedralAngleMinimums[3] - calculatedDihedralAngleMinimums[3]);
  REQUIRE(result4 < ::k_MaxDifference);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][TriangleDihedralAngleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "TriangleDihedralAngleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "TriangleDihedralAngleFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<TriangleDihedralAngleFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  DihedralStoreRole failing_role = DihedralStoreRole::Angles;
  bool failing_write = false;
  usize failing_call = 0;
  bool cancel_after_first_write = false;
  std::atomic_bool should_cancel{false};
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

  ~DihedralReportingStore() override = default;
  DihedralReportingStore(const DihedralReportingStore&) = default;
  DihedralReportingStore(DihedralReportingStore&&) = default;
  DihedralReportingStore& operator=(const DihedralReportingStore&) = delete;
  DihedralReportingStore& operator=(DihedralReportingStore&&) = delete;

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
      if(m_State.failing_role == m_Role && !m_State.failing_write && m_State.failing_call == call)
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
    if(m_State.failing_role == m_Role && m_State.failing_write && m_State.failing_call == call)
    {
      return MakeErrorResult(k_DihedralTransferError, DihedralFailureMessage(m_Role));
    }
    auto result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && m_Role == DihedralStoreRole::Angles && call == 1 && m_State.cancel_after_first_write)
    {
      m_State.should_cancel = true;
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
  : state_ref(state)
  {
    state_ref.calls.clear();
    state_ref.reads.fill(0);
    state_ref.writes.fill(0);
    state_ref.armed = true;
  }

  /**
   * @brief Stops recording even when the target or a test assertion throws.
   */
  ~DihedralTraceGuard()
  {
    state_ref.armed = false;
  }

  DihedralTraceGuard(const DihedralTraceGuard&) = delete;
  DihedralTraceGuard(DihedralTraceGuard&&) = delete;
  DihedralTraceGuard& operator=(const DihedralTraceGuard&) = delete;
  DihedralTraceGuard& operator=(DihedralTraceGuard&&) = delete;

  DihedralTransferState& state_ref;
};

/**
 * @struct DihedralFixture
 * @brief Supplies eleven shared vertices and repeated triangles with a distinct final block tail.
 */
struct DihedralFixture
{
  DataStructure data_structure;
  TriangleGeom* geometry_ptr = nullptr;
  Float32Array* vertices_ptr = nullptr;
  DataArray<IGeometry::MeshIndexType>* faces_ptr = nullptr;
  AttributeMatrix* face_data_ptr = nullptr;
  AttributeMatrix* vertex_data_ptr = nullptr;

  /**
   * @brief Creates accepted filter inputs with no output array.
   * @param faceCount Number of faces; zero retains a valid empty tuple shape.
   * @param state Optional longer-lived state selects real resident reporting stores.
   */
  explicit DihedralFixture(usize faceCount, DihedralTransferState* state = nullptr)
  {
    geometry_ptr = TriangleGeom::Create(data_structure, k_DihedralGeometryPath.getTargetName());
    REQUIRE(geometry_ptr != nullptr);
    face_data_ptr = AttributeMatrix::Create(data_structure, "FaceData", {faceCount}, geometry_ptr->getId());
    vertex_data_ptr = AttributeMatrix::Create(data_structure, "VertexData", {11}, geometry_ptr->getId());
    REQUIRE(face_data_ptr != nullptr);
    REQUIRE(vertex_data_ptr != nullptr);
    geometry_ptr->setFaceAttributeMatrix(*face_data_ptr);
    geometry_ptr->setVertexAttributeMatrix(*vertex_data_ptr);
    std::shared_ptr<AbstractDataStore<float32>> verticesStore;
    std::shared_ptr<AbstractDataStore<IGeometry::MeshIndexType>> facesStore;
    if(state != nullptr)
    {
      verticesStore = std::make_shared<DihedralReportingStore<float32>>(11, *state, DihedralStoreRole::Vertices);
      facesStore = std::make_shared<DihedralReportingStore<IGeometry::MeshIndexType>>(faceCount, *state, DihedralStoreRole::Faces);
    }
    else
    {
      verticesStore = DataStoreUtilities::CreateDataStore<float32>(data_structure, k_DihedralVerticesPath, {11}, {3});
      facesStore = DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(data_structure, k_DihedralFacesPath, {faceCount}, {3});
    }
    REQUIRE(verticesStore != nullptr);
    REQUIRE(facesStore != nullptr);
    vertices_ptr = Float32Array::Create(data_structure, "Vertices", verticesStore, geometry_ptr->getId());
    faces_ptr = DataArray<IGeometry::MeshIndexType>::Create(data_structure, "Faces", facesStore, geometry_ptr->getId());
    REQUIRE(vertices_ptr != nullptr);
    REQUIRE(faces_ptr != nullptr);
    geometry_ptr->setVertices(*vertices_ptr);
    geometry_ptr->setFaceList(*faces_ptr);
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

  ~DihedralFixture() = default;
  DihedralFixture(const DihedralFixture&) = default;
  DihedralFixture(DihedralFixture&&) = default;
  DihedralFixture& operator=(const DihedralFixture&) = default;
  DihedralFixture& operator=(DihedralFixture&&) = default;

  static Arguments arguments()
  {
    Arguments args;
    args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(k_DihedralGeometryPath));
    args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>("Angles"));
    return args;
  }

  Float64Array& output()
  {
    REQUIRE_NOTHROW(data_structure.getDataRefAs<Float64Array>(k_DihedralOutputPath));
    return data_structure.getDataRefAs<Float64Array>(k_DihedralOutputPath);
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
  const usize faceCount = fixture.faces_ptr->getNumberOfTuples();
  REQUIRE(completedFaces <= faceCount);
  REQUIRE(fixture.geometry_ptr->getVertices() == fixture.vertices_ptr);
  REQUIRE(fixture.geometry_ptr->getFaces() == fixture.faces_ptr);
  REQUIRE(fixture.geometry_ptr->getFaceAttributeMatrix() == fixture.face_data_ptr);
  REQUIRE(fixture.geometry_ptr->getVertexAttributeMatrix() == fixture.vertex_data_ptr);
  REQUIRE(fixture.vertex_data_ptr->getShape() == ShapeType{11});
  REQUIRE(fixture.face_data_ptr->getShape() == ShapeType{faceCount});
  REQUIRE(fixture.vertices_ptr->getTupleShape() == ShapeType{11});
  REQUIRE(fixture.vertices_ptr->getComponentShape() == ShapeType{3});
  REQUIRE(fixture.faces_ptr->getTupleShape() == ShapeType{faceCount});
  REQUIRE(fixture.faces_ptr->getComponentShape() == ShapeType{3});
  REQUIRE(angles.getTupleShape() == ShapeType{faceCount});
  REQUIRE(angles.getComponentShape() == ShapeType{1});
  REQUIRE(angles.getSize() == faceCount);
  std::array<float32, 33> vertices{};
  auto verticesResult = fixture.vertices_ptr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(vertices.data(), vertices.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesResult);
  REQUIRE(vertices == k_DihedralVertices);
  std::vector<IGeometry::MeshIndexType> faces(k_DihedralBufferFaces * 3);
  std::vector<float64> values(k_DihedralBufferFaces);
  for(usize firstFace = 0; firstFace < faceCount; firstFace += k_DihedralBufferFaces)
  {
    const usize count = std::min(k_DihedralBufferFaces, faceCount - firstFace);
    auto facesResult = fixture.faces_ptr->getDataStoreRef().copyIntoBuffer(firstFace * 3, nonstd::span<IGeometry::MeshIndexType>(faces.data(), count * 3));
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
  UnitTest::CheckArraysInheritTupleDims(fixture.data_structure);
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
  REQUIRE(fixture.data_structure.getData(k_DihedralOutputPath) == nullptr);
  const auto preflight = filter.preflight(fixture.data_structure, fixture.arguments());
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto store = std::make_shared<DihedralReportingStore<float64>>(fixture.faces_ptr->getNumberOfTuples(), state, DihedralStoreRole::Angles);
  store->fill(k_DihedralSentinel);
  auto* outputPtr = Float64Array::Create(fixture.data_structure, "Angles", store, fixture.face_data_ptr->getId());
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
  scope.requireExpectedStore(*fixture.vertices_ptr);
  scope.requireExpectedStore(*fixture.faces_ptr);
  REQUIRE(fixture.data_structure.getData(k_DihedralOutputPath) == nullptr);
  TriangleDihedralAngleFilter filter;
  const auto args = fixture.arguments();
  const auto preflight = filter.preflight(fixture.data_structure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, fixture.data_structure, args);
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
    DihedralStoreRole failing_role;
    bool failing_write;
    usize failing_call;
    usize trace_length;
    usize completed_faces;
    int32 context_code;
    const char* context_message;
  };
  const std::array<ContractCase, 7> cases = {{{.name = "complete",
                                               .failing_role = DihedralStoreRole::Angles,
                                               .failing_write = false,
                                               .failing_call = 0,
                                               .trace_length = 5,
                                               .completed_faces = k_DihedralBlockFaces + 1,
                                               .context_code = 0,
                                               .context_message = ""},
                                              {.name = "vertex read",
                                               .failing_role = DihedralStoreRole::Vertices,
                                               .failing_write = false,
                                               .failing_call = 1,
                                               .trace_length = 1,
                                               .completed_faces = 0,
                                               .context_code = -65820,
                                               .context_message = "Error reading vertex coordinates for triangle dihedral angle calculation."},
                                              {.name = "first face read",
                                               .failing_role = DihedralStoreRole::Faces,
                                               .failing_write = false,
                                               .failing_call = 1,
                                               .trace_length = 2,
                                               .completed_faces = 0,
                                               .context_code = -65821,
                                               .context_message = "Error reading connectivity for triangle dihedral angle calculation at triangle 0 (262144 triangles)."},
                                              {.name = "tail face read",
                                               .failing_role = DihedralStoreRole::Faces,
                                               .failing_write = false,
                                               .failing_call = 2,
                                               .trace_length = 4,
                                               .completed_faces = k_DihedralBlockFaces,
                                               .context_code = -65821,
                                               .context_message = "Error reading connectivity for triangle dihedral angle calculation at triangle 262144 (1 triangles)."},
                                              {.name = "first output write",
                                               .failing_role = DihedralStoreRole::Angles,
                                               .failing_write = true,
                                               .failing_call = 1,
                                               .trace_length = 3,
                                               .completed_faces = 0,
                                               .context_code = -65822,
                                               .context_message = "Error writing dihedral angles at triangle 0 (262144 triangles)."},
                                              {.name = "tail output write",
                                               .failing_role = DihedralStoreRole::Angles,
                                               .failing_write = true,
                                               .failing_call = 2,
                                               .trace_length = 5,
                                               .completed_faces = k_DihedralBlockFaces,
                                               .context_code = -65822,
                                               .context_message = "Error writing dihedral angles at triangle 262144 (1 triangles)."},
                                              {.name = "cancel after first block",
                                               .failing_role = DihedralStoreRole::Angles,
                                               .failing_write = false,
                                               .failing_call = 0,
                                               .trace_length = 3,
                                               .completed_faces = k_DihedralBlockFaces,
                                               .context_code = 0,
                                               .context_message = ""}}};
  const auto& contract = cases[caseIndex];
  CAPTURE(scenario, contract.name);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralTransferState state;
  DihedralFixture fixture(k_DihedralBlockFaces + 1, &state);
  auto* outputPtr = AddDihedralReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.vertices_ptr);
  scope.requireExpectedStore(*fixture.faces_ptr);
  scope.requireExpectedStore(*outputPtr);
  state.failing_role = contract.failing_role;
  state.failing_write = contract.failing_write;
  state.failing_call = contract.failing_call;
  state.cancel_after_first_write = caseIndex == 6;
  TriangleDihedralAngleInputValues inputValues{.InputTriangleGeometryPath = k_DihedralGeometryPath, .SurfaceMeshTriangleDihedralAnglesArrayName = "Angles"};
  const IFilter::MessageHandler messages{};
  DihedralTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleDihedralAngle(fixture.data_structure, messages, state.should_cancel, &inputValues)(); });
  const auto calls = state.calls;
  state.armed = false;
  REQUIRE(result.warnings().empty());
  if(contract.failing_call == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 2);
    CHECK(result.errors()[0].code == k_DihedralTransferError);
    CHECK(result.errors()[0].message == DihedralFailureMessage(contract.failing_role));
    CHECK(result.errors()[1].code == contract.context_code);
    CHECK(result.errors()[1].message == contract.context_message);
  }
  CHECK(state.should_cancel.load() == state.cancel_after_first_write);
  const std::array<DihedralTransfer, 5> expected = {{{.role = DihedralStoreRole::Vertices, .write = false, .start = 0, .count = 33},
                                                     {.role = DihedralStoreRole::Faces, .write = false, .start = 0, .count = 786432},
                                                     {.role = DihedralStoreRole::Angles, .write = true, .start = 0, .count = 262144},
                                                     {.role = DihedralStoreRole::Faces, .write = false, .start = 786432, .count = 3},
                                                     {.role = DihedralStoreRole::Angles, .write = true, .start = 262144, .count = 1}}};
  REQUIRE(calls.size() == contract.trace_length);
  for(usize callIndex = 0; callIndex < calls.size(); ++callIndex)
  {
    CAPTURE(callIndex);
    CHECK(calls[callIndex].role == expected[callIndex].role);
    CHECK(calls[callIndex].write == expected[callIndex].write);
    CHECK(calls[callIndex].start == expected[callIndex].start);
    CHECK(calls[callIndex].count == expected[callIndex].count);
  }
  CheckDihedralFixture(fixture, contract.completed_faces);
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
  scope.requireExpectedStore(*fixture.vertices_ptr);
  scope.requireExpectedStore(*fixture.faces_ptr);
  scope.requireExpectedStore(*outputPtr);
  state.should_cancel = preCancelled;
  TriangleDihedralAngleInputValues inputValues{.InputTriangleGeometryPath = k_DihedralGeometryPath, .SurfaceMeshTriangleDihedralAnglesArrayName = "Angles"};
  const IFilter::MessageHandler messages{};
  DihedralTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleDihedralAngle(fixture.data_structure, messages, state.should_cancel, &inputValues)(); });
  const auto calls = state.calls;
  state.armed = false;
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.warnings().empty());
  CHECK(calls.empty());
  CHECK(state.should_cancel.load() == preCancelled);
  CheckDihedralFixture(fixture, 0);
}

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: empty geometry", "[SimplnxCore][TriangleDihedralAngleFilter][D2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DihedralFixture fixture(0);
  scope.requireExpectedStore(*fixture.vertices_ptr);
  scope.requireExpectedStore(*fixture.faces_ptr);
  REQUIRE(fixture.data_structure.getData(k_DihedralOutputPath) == nullptr);
  TriangleDihedralAngleFilter filter;
  const auto args = fixture.arguments();
  const auto preflight = filter.preflight(fixture.data_structure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, fixture.data_structure, args);
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
  auto resizeVertices = fixture.geometry_ptr->resizeVertexList(3);
  auto resizeVertexData = fixture.vertex_data_ptr->resizeTuples({3});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeVertices);
  SIMPLNX_RESULT_REQUIRE_VALID(resizeVertexData);
  // Three vertices admit only IDs 0, 1, and 2. Every corner must fail before offset arithmetic.
  std::array<IGeometry::MeshIndexType, 6> faces = {0, 1, 2, 0, 1, 2};
  faces.at((invalidFace * 3) + invalidCorner) = invalidVertex;
  auto facesWrite = fixture.faces_ptr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const IGeometry::MeshIndexType>(faces.data(), faces.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(facesWrite);
  auto* outputPtr = AddDihedralReportingOutput(fixture, state);
  scope.requireExpectedStore(*fixture.vertices_ptr);
  scope.requireExpectedStore(*fixture.faces_ptr);
  scope.requireExpectedStore(*outputPtr);
  TriangleDihedralAngleInputValues inputValues{.InputTriangleGeometryPath = k_DihedralGeometryPath, .SurfaceMeshTriangleDihedralAnglesArrayName = "Angles"};
  const IFilter::MessageHandler messages{};
  const DihedralTraceGuard traceGuard(state);
  const auto result = scope.execute([&] { return TriangleDihedralAngle(fixture.data_structure, messages, state.should_cancel, &inputValues)(); });
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
  auto facesRead = fixture.faces_ptr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<IGeometry::MeshIndexType>(actualFaces.data(), actualFaces.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(facesRead);
  CHECK(actualFaces == faces);
  std::array<float32, 9> actualVertices{};
  auto verticesRead = fixture.vertices_ptr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actualVertices.data(), actualVertices.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesRead);
  CHECK(std::equal(actualVertices.begin(), actualVertices.end(), k_DihedralVertices.begin()));
  UnitTest::CheckArraysInheritTupleDims(fixture.data_structure);
}
#endif

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: Permutation invariant minimum", "[SimplnxCore][TriangleDihedralAngleFilter]")
{
  UnitTest::LoadPlugins();

  // Issue #1790 (70-30-80 degrees); a right triangle with atan(1/10) at B; an obtuse triangle (135 degrees at A);
  // a collinear face (C = -2B exactly in float32) whose normalized dot products round past +/-1 without the clamp.
  const std::array<std::array<float32, 9>, 4> fixtures = {
      {{{0, 0, 0, 1, 0, 0, 0.1736481785774231F, 0.477094441652298F, 0}}, {{0, 0, 0, 10, 0, 0, 0, 1, 0}}, {{0, 0, 0, 1, 0, 0, -0.5F, 0.5F, 0}}, {{0, 0, 0, 0.7F, 3.0F, 0.1F, -1.4F, -6.0F, -0.2F}}}};
  // Closed-form minimum interior angle of each fixture, in degrees.
  const float64 radToDeg = 180.0 / std::acos(-1.0);
  const std::array<float64, 4> closedFormMinimums = {{30.0, std::atan(0.1) * radToDeg, std::atan(1.0 / 3.0) * radToDeg, 0.0}};
  // ABC, BCA, CAB, followed by their reversed windings.
  const std::array<usize, 18> orders = {{0, 1, 2, 1, 2, 0, 2, 0, 1, 0, 2, 1, 2, 1, 0, 1, 0, 2}};
  for(usize fixture = 0; fixture < fixtures.size(); ++fixture)
  {
    CAPTURE(fixture);
    DataStructure dataStructure;
    auto* geomPtr = TriangleGeom::Create(dataStructure, "Triangles");
    REQUIRE(geomPtr != nullptr);
    auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "Face Data", std::vector<usize>{6}, geomPtr->getId());
    auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", std::vector<usize>{3}, geomPtr->getId());
    REQUIRE(faceDataPtr != nullptr);
    REQUIRE(vertexDataPtr != nullptr);
    geomPtr->setFaceAttributeMatrix(*faceDataPtr);
    geomPtr->setVertexAttributeMatrix(*vertexDataPtr);
    auto* verticesPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Vertices", std::vector<usize>{3}, std::vector<usize>{3}, geomPtr->getId());
    auto* facesPtr = DataArray<IGeometry::MeshIndexType>::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "Faces", std::vector<usize>{6}, std::vector<usize>{3}, geomPtr->getId());
    REQUIRE(verticesPtr != nullptr);
    REQUIRE(facesPtr != nullptr);
    for(usize i = 0; i < 9; ++i)
    {
      (*verticesPtr)[i] = fixtures[fixture][i];
    }
    for(usize i = 0; i < orders.size(); ++i)
    {
      (*facesPtr)[i] = orders[i];
    }
    geomPtr->setVertices(*verticesPtr);
    geomPtr->setFaceList(*facesPtr);

    // Independent reference: atan2 of each corner's cross magnitude and dot.
    float64 expected = std::numeric_limits<float64>::infinity();
    for(usize corner = 0; corner < 3; ++corner)
    {
      std::array<float64, 3> toNext{};
      std::array<float64, 3> toPrev{};
      for(usize axis = 0; axis < 3; ++axis)
      {
        toNext[axis] = static_cast<float64>((*verticesPtr)[3 * ((corner + 1) % 3) + axis]) - (*verticesPtr)[3 * corner + axis];
        toPrev[axis] = static_cast<float64>((*verticesPtr)[3 * ((corner + 2) % 3) + axis]) - (*verticesPtr)[3 * corner + axis];
      }
      const std::array<float64, 3> cross = {{toNext[1] * toPrev[2] - toNext[2] * toPrev[1], toNext[2] * toPrev[0] - toNext[0] * toPrev[2], toNext[0] * toPrev[1] - toNext[1] * toPrev[0]}};
      const float64 dot = toNext[0] * toPrev[0] + toNext[1] * toPrev[1] + toNext[2] * toPrev[2];
      const float64 magnitude = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
      expected = std::min(expected, std::atan2(magnitude, dot) * radToDeg);
    }
    // The computed oracle must agree with the hand-derived closed form.
    REQUIRE(std::abs(expected - closedFormMinimums[fixture]) < 1.0e-5);
    TriangleDihedralAngleFilter filter;
    Arguments args;
    args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
    args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>("Minimum"));
    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(DataPath({"Triangles", "Face Data", "Minimum"})));
    const auto& output = dataStructure.getDataRefAs<Float64Array>(DataPath({"Triangles", "Face Data", "Minimum"}));
    for(usize face = 0; face < 6; ++face)
    {
      CAPTURE(face, expected, output[face]);
      CHECK(std::isfinite(output[face]));
      // Allow float32 edge-subtraction rounding; the oracle subtracts in float64.
      CHECK(std::abs(output[face] - expected) < 1.0e-5);
      CHECK(output[face] <= 60.0 + 1.0e-5);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
