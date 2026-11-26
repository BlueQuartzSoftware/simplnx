#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nx::core
{
class Arguments;
class IFilter;

namespace TriangleGeometrySupportTest
{
inline const DataPath k_MeshGeomPath({"Triangles"});
inline const DataPath k_MeshFacePath = k_MeshGeomPath.createChildPath("FaceData");
using SupportListType = TriangleGeom::ElementDynamicList;
using SupportIndexType = IGeometry::MeshIndexType;

enum class SupportKind
{
  VertexIncidence,
  FaceNeighbors
};

inline constexpr int32 k_InjectedError = -56991;
// A selected bulk read or resize fails after the test installs this store.
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
  std::optional<DataObject::IdType> Id;
  std::optional<DataObject::IdType> Association;
  std::weak_ptr<const SupportListType> Object;
  std::vector<DataPath> Paths;
  std::vector<std::vector<SupportIndexType>> Lists;
};

/** @brief Records the geometry association, object identity, paths and list values without extending the object's lifetime. */
SupportSnapshot CaptureSupportSnapshot(const DataStructure& dataStructure, SupportKind kind);
void RequireSupportSnapshot(const DataStructure& dataStructure, SupportKind kind, const SupportSnapshot& expected);
void CreateSupportData(DataStructure& dataStructure, SupportKind kind, const std::vector<std::vector<SupportIndexType>>& lists, bool associate = true);

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

void ApplySupportFixtureActions(DataStructure& dataStructure, const IFilter& filter, const Arguments& args);
void RequireInjectedFailure(const Result<>& result, const std::string& message);
void RequireSupportFixtureOocStores(const DataStructure& dataStructure, const std::vector<DataPath>& paths);

/** @brief Keeps the saved DREAM3D file separate from the backing stores and removes it after the test. */
class SupportSaveFile
{
public:
  explicit SupportSaveFile(const std::string& label);
  ~SupportSaveFile() noexcept;
  SupportSaveFile(const SupportSaveFile&) = delete;
  SupportSaveFile(SupportSaveFile&&) = delete;
  SupportSaveFile& operator=(const SupportSaveFile&) = delete;
  SupportSaveFile& operator=(SupportSaveFile&&) = delete;

  const std::filesystem::path path;
};
} // namespace TriangleGeometrySupportTest
} // namespace nx::core
