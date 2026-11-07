#include "SimplnxCore/Filters/Algorithms/LabelTriangleGeometry.hpp"
#include "SimplnxCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
fs::path k_ExemplarDataFilePath = fs::path(fmt::format("{}/label_triangle_geometry_test_v2/label_triangle_geometry_test.dream3d", nx::core::unit_test::k_TestFilesDir));
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/label_triangle_geometry_test_v2/data_to_generate_test/combined_stls.dream3d", nx::core::unit_test::k_TestFilesDir));

static constexpr StringLiteral k_CreatedAMName = "Cell Feature AM";
static constexpr StringLiteral k_NumTrianglesName = "NumTriangles";
static constexpr StringLiteral k_RegionIdsName = "Region Ids";

const DataPath k_TriangleGeomPath({"TriangleGeometry"});
const DataPath k_CreatedRegionIdsPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(k_RegionIdsName);
const DataPath k_CellFeatureAMPath = k_TriangleGeomPath.createChildPath(k_CreatedAMName);
const DataPath k_NumTrianglesPath = k_CellFeatureAMPath.createChildPath(k_NumTrianglesName);
} // namespace

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "label_triangle_geometry_test_v2.tar.gz", "label_triangle_geometry_test_v2", true, true);

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    LabelTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(::k_CreatedRegionIdsPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>(::k_CreatedAMName));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>(::k_NumTrianglesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // This is in here because the exemplar face attribute matrix is not sized correctly. This will
    // correct that value allowing the test to proceed normally.
    auto& exemplarContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    auto vertexAttributeMatrixResizeResult = exemplarContourTriGeom.getVertexAttributeMatrix()->resizeTuples({144});
    SIMPLNX_RESULT_REQUIRE_VALID(vertexAttributeMatrixResizeResult);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(k_ExemplarDataFilePath);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, ::k_CellFeatureAMPath, ::k_TriangleGeomPath.toString());
  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, ::k_TriangleGeomPath.createChildPath(Constants::k_Face_Data), ::k_TriangleGeomPath.toString());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][LabelTriangleGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "LabelTriangleGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<LabelTriangleGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key) == DataPath({"DataContainer", "FaceData", "RegionIds"}));
      CHECK(args.value<std::string>(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key) == "TriangleData");
      CHECK(args.value<std::string>(LabelTriangleGeometryFilter::k_NumTrianglesName_Key) == "NumTriangles");
    }
  }
}

namespace
{
constexpr int32 k_InjectedError = -56991;
const DataPath k_MeshGeomPath({"Triangles"});
const DataPath k_MeshFacePath = k_MeshGeomPath.createChildPath("FaceData");
using SupportListType = TriangleGeom::ElementDynamicList;
using SupportIndexType = IGeometry::MeshIndexType;

// These stores inject ordinary Result failures only. They do not simulate OOC or allocation failure.
template <typename T>
class InjectedFailureStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;
  ~InjectedFailureStore() noexcept override = default;
  InjectedFailureStore(const InjectedFailureStore&) = delete;
  InjectedFailureStore(InjectedFailureStore&&) = delete;
  InjectedFailureStore& operator=(const InjectedFailureStore&) = delete;
  InjectedFailureStore& operator=(InjectedFailureStore&&) = delete;

  bool RejectResize = false;
  usize FailingRead = 0;
  mutable std::vector<std::pair<usize, usize>> Reads;

  Result<> resizeTuples(const ShapeType& shape) override
  {
    return RejectResize ? MakeErrorResult(k_InjectedError, "injected resize failure") : DataStore<T>::resizeTuples(shape);
  }

  Result<> copyIntoBuffer(usize start, nonstd::span<T> buffer) const override
  {
    Reads.emplace_back(start, buffer.size());
    return Reads.size() == FailingRead ? MakeErrorResult(k_InjectedError, "injected face read failure") : DataStore<T>::copyIntoBuffer(start, buffer);
  }
};

struct SupportSnapshot
{
  SupportSnapshot() = default;
  ~SupportSnapshot() noexcept = default;
  SupportSnapshot(const SupportSnapshot&) = default;
  SupportSnapshot(SupportSnapshot&&) noexcept = default;
  SupportSnapshot& operator=(const SupportSnapshot&) = default;
  SupportSnapshot& operator=(SupportSnapshot&&) noexcept = default;

  std::optional<DataObject::IdType> Id;
  std::optional<DataObject::IdType> Association;
  const SupportListType* Object = nullptr;
  std::vector<DataPath> Paths;
  std::vector<std::vector<SupportIndexType>> Lists;
};

DataPath SupportDataPath(bool neighbors)
{
  return k_MeshGeomPath.createChildPath(neighbors ? TriangleGeom::k_EltNeighbors : TriangleGeom::k_EltsContainingVert);
}

SupportSnapshot CaptureSupportSnapshot(const DataStructure& dataStructure, bool neighbors)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
  const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
  SupportSnapshot snapshot;
  snapshot.Association = neighbors ? geometry.getElementNeighborsId() : geometry.getElementContainingVertId();
  snapshot.Object = dataStructure.getDataAs<SupportListType>(SupportDataPath(neighbors));
  if(snapshot.Object != nullptr)
  {
    snapshot.Id = snapshot.Object->getId();
    snapshot.Paths = snapshot.Object->getDataPaths();
    for(usize i = 0; i < snapshot.Object->size(); ++i)
    {
      std::vector<SupportIndexType> list;
      for(usize j = 0; j < snapshot.Object->getNumberOfElements(i); ++j)
      {
        list.push_back(snapshot.Object->getElementListPointer(i)[j]);
      }
      snapshot.Lists.push_back(std::move(list));
    }
  }
  return snapshot;
}

void RequireSupportSnapshot(const DataStructure& dataStructure, bool neighbors, const SupportSnapshot& expected)
{
  const auto actual = CaptureSupportSnapshot(dataStructure, neighbors);
  CHECK(actual.Association == expected.Association);
  REQUIRE(actual.Id == expected.Id);
  CHECK(actual.Object == expected.Object);
  CHECK(actual.Paths == expected.Paths);
  CHECK(actual.Lists == expected.Lists);
}

void CreateSupportData(DataStructure& dataStructure, bool neighbors, const std::vector<std::vector<SupportIndexType>>& lists, bool associate = true)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
  auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
  auto* support = SupportListType::Create(dataStructure, neighbors ? TriangleGeom::k_EltNeighbors : TriangleGeom::k_EltsContainingVert, geometry.getId());
  REQUIRE(support != nullptr);
  support->allocateLists(std::vector<uint16>(lists.size(), 0));
  for(usize i = 0; i < lists.size(); ++i)
  {
    auto list = lists[i];
    if(!list.empty())
    {
      REQUIRE(support->setElementList(i, static_cast<uint16>(list.size()), list.data()));
    }
  }
  if(associate)
  {
    if(neighbors)
    {
      geometry.setElementNeighborsId(support->getId());
    }
    else
    {
      geometry.setElementContainingVertId(support->getId());
    }
  }
  REQUIRE(CaptureSupportSnapshot(dataStructure, neighbors).Lists == lists);
}

template <typename T>
DataArray<T>& CreateSupportFixtureArray(DataStructure& dataStructure, const DataPath& path, usize tuples, usize components, DataObject::IdType parent)
{
  auto store = DataStoreUtilities::CreateDataStore<T>(dataStructure, path, {tuples}, {components});
  REQUIRE(store != nullptr);
  store->fill(T{});
  auto* array = DataArray<T>::Create(dataStructure, path.getTargetName(), std::move(store), parent);
  REQUIRE(array != nullptr);
  return *array;
}

void ApplySupportFixtureActions(DataStructure& dataStructure, const IFilter& filter, const Arguments& args)
{
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto applied = preflight.outputActions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
}

void RequireInjectedFailure(const Result<>& result, const std::string& message)
{
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == k_InjectedError);
  CHECK(result.errors()[0].message == message);
}

void RequireSupportFixtureOocStores(const DataStructure& dataStructure, const std::vector<DataPath>& paths)
{
  for(const auto& path : paths)
  {
    CAPTURE(path);
    const auto* array = dataStructure.getDataAs<IDataArray>(path);
    REQUIRE(array != nullptr);
    REQUIRE(array->getDataFormat() == "HDF5-OOC");
    REQUIRE(array->getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  }
}

// Fresh destinations stay separate from backing stores and partial failed saves.
class SupportSaveFiles
{
public:
  explicit SupportSaveFiles(const std::string& label)
  {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for(usize i = 0; i < Paths.size(); ++i)
    {
      Paths[i] = fs::temp_directory_path() / fmt::format("support-ownership-{}-{}-{}.dream3d", label, nonce, i);
      REQUIRE_FALSE(fs::exists(Paths[i]));
    }
  }
  ~SupportSaveFiles() noexcept
  {
    for(const auto& path : Paths)
    {
      std::error_code error;
      fs::remove(path, error);
    }
  }
  SupportSaveFiles(const SupportSaveFiles&) = delete;
  SupportSaveFiles(SupportSaveFiles&&) = delete;
  SupportSaveFiles& operator=(const SupportSaveFiles&) = delete;
  SupportSaveFiles& operator=(SupportSaveFiles&&) = delete;
  std::array<fs::path, 2> Paths;
};

void DiscardSuppliedSupport(DataStructure& dataStructure, bool neighbors, const SupportSnapshot& snapshot)
{
  if(snapshot.Id.has_value())
  {
    RequireSupportSnapshot(dataStructure, neighbors, snapshot);
    REQUIRE(dataStructure.removeData(*snapshot.Id));
    REQUIRE(dataStructure.getData(*snapshot.Id) == nullptr);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
    auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
    if(neighbors)
    {
      geometry.setElementNeighborsId(std::nullopt);
    }
    else
    {
      geometry.setElementContainingVertId(std::nullopt);
    }
  }
  RequireSupportSnapshot(dataStructure, neighbors, {});
}
} // namespace

namespace
{
const DataPath k_RegionIdsPath = k_MeshFacePath.createChildPath("RegionIds");
const DataPath k_MeshFeaturePath = k_MeshGeomPath.createChildPath("Features");
const DataPath k_TriangleCountPath = k_MeshFeaturePath.createChildPath("NumTriangles");

Arguments CreateLabelFixture(DataStructure& dataStructure, bool incidence, bool neighbors)
{
  auto* geometry = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geometry != nullptr);
  auto* faceData = AttributeMatrix::Create(dataStructure, "FaceData", {1}, geometry->getId());
  REQUIRE(faceData != nullptr);
  geometry->setFaceAttributeMatrix(*faceData);
  auto& vertices = CreateSupportFixtureArray<float32>(dataStructure, k_MeshGeomPath.createChildPath("Vertices"), 3, 3, geometry->getId());
  auto& faces = CreateSupportFixtureArray<SupportIndexType>(dataStructure, k_MeshGeomPath.createChildPath("Faces"), 1, 3, geometry->getId());
  vertices[3] = 1.0F;
  vertices[7] = 1.0F;
  faces[1] = 1;
  faces[2] = 2;
  geometry->setVertices(vertices);
  geometry->setFaceList(faces);
  if(incidence)
  {
    CreateSupportData(dataStructure, false, {{0}, {0}, {0}});
  }
  if(neighbors)
  {
    CreateSupportData(dataStructure, true, {{}});
  }
  Arguments args;
  args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(k_MeshGeomPath));
  args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(k_RegionIdsPath));
  args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Features"));
  args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>("NumTriangles"));
  return args;
}

Result<> RunLabel(DataStructure& dataStructure, bool cancelled)
{
  LabelTriangleGeometryInputValues inputs{k_MeshGeomPath, k_RegionIdsPath, k_MeshFeaturePath, k_TriangleCountPath};
  const std::atomic_bool cancel = cancelled;
  return LabelTriangleGeometry(dataStructure, {}, cancel, &inputs)();
}

void CheckLabelInputs(const DataStructure& dataStructure)
{
  const auto* vertices = dataStructure.getDataAs<Float32Array>(k_MeshGeomPath.createChildPath("Vertices"));
  const auto* faces = dataStructure.getDataAs<DataArray<SupportIndexType>>(k_MeshGeomPath.createChildPath("Faces"));
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  REQUIRE(vertices->getNumberOfTuples() == 3);
  REQUIRE(faces->getNumberOfTuples() == 1);
  const std::array<float32, 9> expectedVertices = {0, 0, 0, 1, 0, 0, 0, 1, 0};
  for(usize i = 0; i < expectedVertices.size(); ++i)
  {
    CHECK((*vertices)[i] == expectedVertices[i]);
  }
  for(usize i = 0; i < 3; ++i)
  {
    CHECK((*faces)[i] == i);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
  const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
  CHECK(geometry.getVertices() == vertices);
  CHECK(geometry.getFaces() == faces);
}

void CheckLabelOutputs(const DataStructure& dataStructure)
{
  const auto* regions = dataStructure.getDataAs<Int32Array>(k_RegionIdsPath);
  const auto* counts = dataStructure.getDataAs<UInt64Array>(k_TriangleCountPath);
  REQUIRE(regions != nullptr);
  REQUIRE(counts != nullptr);
  REQUIRE(regions->getNumberOfTuples() == 1);
  CHECK((*regions)[0] == 1);
  REQUIRE(counts->getTupleShape() == ShapeType{3});
  const std::array<uint64, 3> expected = {0, 1, 0};
  for(usize i = 0; i < expected.size(); ++i)
  {
    CHECK((*counts)[i] == expected[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_MeshFeaturePath));
  CHECK(dataStructure.getDataRefAs<AttributeMatrix>(k_MeshFeaturePath).getShape() == ShapeType{3});
}

void CheckLabelSave(DataStructure& dataStructure, const SupportSnapshot& incidence, const SupportSnapshot& neighbors)
{
  SupportSaveFiles files("label");
  const auto save = HDF5::DataStructureWriter::WriteFile(dataStructure, files.Paths[0]);
  if(incidence.Id.has_value() || neighbors.Id.has_value())
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(save);
    REQUIRE(save.errors().size() == 1);
    CHECK(save.errors()[0].code == -5);
    CHECK(save.errors()[0].message == "Could not find IO factory for datatype: DynamicListArray");
    RequireSupportSnapshot(dataStructure, false, incidence);
    RequireSupportSnapshot(dataStructure, true, neighbors);
    CheckLabelInputs(dataStructure);
    CheckLabelOutputs(dataStructure);
    // Explicit caller recovery, after every preservation assertion. All owners here are this fixture.
    DiscardSuppliedSupport(dataStructure, true, neighbors);
    DiscardSuppliedSupport(dataStructure, false, incidence);
    const auto recovery = HDF5::DataStructureWriter::WriteFile(dataStructure, files.Paths[1]);
    SIMPLNX_RESULT_REQUIRE_VALID(recovery);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(save);
  }
  const auto read = HDF5::DataStructureReader::ReadFile(files.Paths[(incidence.Id.has_value() || neighbors.Id.has_value()) ? 1 : 0]);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  CheckLabelInputs(read.value());
  CheckLabelOutputs(read.value());
  RequireSupportSnapshot(read.value(), false, {});
  RequireSupportSnapshot(read.value(), true, {});
  UnitTest::CheckArraysInheritTupleDims(read.value());
}

void CheckLabelSuccess(bool incidence, bool neighbors, bool cancelled, bool save, bool ooc)
{
  DataStructure dataStructure;
  const auto args = CreateLabelFixture(dataStructure, incidence, neighbors);
  const auto incidenceBefore = CaptureSupportSnapshot(dataStructure, false);
  const auto neighborsBefore = CaptureSupportSnapshot(dataStructure, true);
  LabelTriangleGeometryFilter filter;
  if(cancelled)
  {
    ApplySupportFixtureActions(dataStructure, filter, args);
    const auto result = RunLabel(dataStructure, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }
  CheckLabelInputs(dataStructure);
  CheckLabelOutputs(dataStructure);
  if(ooc)
  {
    RequireSupportFixtureOocStores(dataStructure, {k_MeshGeomPath.createChildPath("Vertices"), k_MeshGeomPath.createChildPath("Faces"), k_RegionIdsPath, k_TriangleCountPath});
  }
  if(save)
  {
    CheckLabelSave(dataStructure, incidenceBefore, neighborsBefore);
  }
  else
  {
    RequireSupportSnapshot(dataStructure, false, incidenceBefore);
    RequireSupportSnapshot(dataStructure, true, neighborsBefore);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: support ownership", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool incidence = GENERATE(false, true);
  const bool neighbors = GENERATE(false, true);
  const bool cancelled = GENERATE(false, true);
  CAPTURE(incidence, neighbors, cancelled);
  CheckLabelSuccess(incidence, neighbors, cancelled, false, false);
}

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: cleanup on resize failure", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool incidence = GENERATE(false, true);
  const bool neighbors = GENERATE(false, true);
  CAPTURE(incidence, neighbors);
  DataStructure dataStructure;
  const auto args = CreateLabelFixture(dataStructure, incidence, neighbors);
  const auto incidenceBefore = CaptureSupportSnapshot(dataStructure, false);
  const auto neighborsBefore = CaptureSupportSnapshot(dataStructure, true);
  ApplySupportFixtureActions(dataStructure, LabelTriangleGeometryFilter{}, args);
  auto store = std::make_shared<InjectedFailureStore<uint64>>(ShapeType{1}, ShapeType{1}, 73);
  store->RejectResize = true;
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(k_TriangleCountPath));
  const auto replacement = dataStructure.getDataRefAs<UInt64Array>(k_TriangleCountPath).setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replacement);
  const auto result = RunLabel(dataStructure, false);
  RequireInjectedFailure(result, "AttributeMatrix 'Features' failed to resize child array 'Triangles/Features/NumTriangles' to shape [3]: injected resize failure");
  RequireSupportSnapshot(dataStructure, false, incidenceBefore);
  RequireSupportSnapshot(dataStructure, true, neighborsBefore);
  CheckLabelInputs(dataStructure);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RegionIdsPath));
  CHECK(dataStructure.getDataRefAs<Int32Array>(k_RegionIdsPath)[0] == 1);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_MeshFeaturePath));
  CHECK(dataStructure.getDataRefAs<AttributeMatrix>(k_MeshFeaturePath).getShape() == ShapeType{3});
  CHECK(store->getTupleShape() == ShapeType{1});
  CHECK((*store)[0] == 73);
  UnitTest::CheckArraysInheritTupleDims(dataStructure, {k_TriangleCountPath});
}

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: support creation failure", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  // 0: incidence count; 1: third read/orphan neighbor; 2: reused incidence/first neighbor read.
  const usize mode = GENERATE(0, 1, 2);
  CAPTURE(mode);
  DataStructure dataStructure;
  const auto args = CreateLabelFixture(dataStructure, mode == 2, false);
  const auto incidenceBefore = CaptureSupportSnapshot(dataStructure, false);
  ApplySupportFixtureActions(dataStructure, LabelTriangleGeometryFilter{}, args);
  auto store = std::make_shared<InjectedFailureStore<SupportIndexType>>(ShapeType{1}, ShapeType{3}, 0);
  (*store)[1] = 1;
  (*store)[2] = 2;
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<SupportIndexType>>(k_MeshGeomPath.createChildPath("Faces")));
  const auto replacement = dataStructure.getDataRefAs<DataArray<SupportIndexType>>(k_MeshGeomPath.createChildPath("Faces")).setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replacement);
  store->Reads.clear();
  store->FailingRead = mode == 1 ? 3 : 1;
  const auto result = RunLabel(dataStructure, false);
  RequireInjectedFailure(result, "injected face read failure");
  const std::vector<std::pair<usize, usize>> expectedReads(store->FailingRead, {0, 3});
  REQUIRE(store->Reads == expectedReads);
  RequireSupportSnapshot(dataStructure, false, incidenceBefore);
  RequireSupportSnapshot(dataStructure, true, {});
  CheckLabelInputs(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: unassociated named child", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool neighbors = GENERATE(false, true);
  CAPTURE(neighbors);
  DataStructure dataStructure;
  const auto args = CreateLabelFixture(dataStructure, false, false);
  CreateSupportData(dataStructure, neighbors, {{42, 7}}, false);
  const auto incidenceBefore = CaptureSupportSnapshot(dataStructure, false);
  const auto neighborsBefore = CaptureSupportSnapshot(dataStructure, true);
  ApplySupportFixtureActions(dataStructure, LabelTriangleGeometryFilter{}, args);
  const auto result = RunLabel(dataStructure, false);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == (neighbors ? -2232 : -2231));
  RequireSupportSnapshot(dataStructure, false, incidenceBefore);
  RequireSupportSnapshot(dataStructure, true, neighborsBefore);
  CheckLabelInputs(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: retained support save compatibility", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool incidence = GENERATE(false, true);
  const bool neighbors = GENERATE(false, true);
  CAPTURE(incidence, neighbors);
  CheckLabelSuccess(incidence, neighbors, false, true, false);
}

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: real store support ownership", "[SimplnxCore][.OocStoreContract]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  REQUIRE(Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  const bool incidence = GENERATE(false, true);
  const bool neighbors = GENERATE(false, true);
  CAPTURE(incidence, neighbors);
  CheckLabelSuccess(incidence, neighbors, false, true, true);
  std::cout << fmt::format("Label ownership/save/recovery complete incidence={} neighbors={} vertices=HDF5-OOC faces=HDF5-OOC regions=HDF5-OOC counts=HDF5-OOC\n", incidence, neighbors);
}
