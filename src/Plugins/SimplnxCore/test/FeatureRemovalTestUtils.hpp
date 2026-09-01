#pragma once

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <type_traits>
#include <vector>

namespace nx::core::FeatureRemovalTest
{
constexpr usize k_Dim = 200;
constexpr usize k_SliceTuples = k_Dim * k_Dim;
constexpr usize k_TotalTuples = k_SliceTuples * k_Dim;
constexpr usize k_ValueComponents = 3;

inline const std::string k_GeomName = "Feature Removal Benchmark";
inline const std::string k_CellDataName = "Cell Data";
inline const std::string k_FeatureDataName = "Feature Data";
inline const DataPath k_GeomPath({k_GeomName});
inline const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
inline const DataPath k_FeatureDataPath = k_GeomPath.createChildPath(k_FeatureDataName);
inline const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
inline const DataPath k_CellValuesPath = k_CellDataPath.createChildPath("CellValues");
inline const DataPath k_NumCellsPath = k_FeatureDataPath.createChildPath("NumCells");
inline const DataPath k_NumNeighborsPath = k_FeatureDataPath.createChildPath("NumNeighbors");
inline const DataPath k_FeatureMarkerPath = k_FeatureDataPath.createChildPath("FeatureMarker");

template <typename T>
DataArray<T>& CreateArray(DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const ShapeType& componentShape, DataObject::IdType parentId)
{
  auto store = DataStoreUtilities::CreateDataStore<T>(dataStructure, path, tupleShape, componentShape, IDataAction::Mode::Execute);
  return *DataArray<T>::Create(dataStructure, path.getTargetName(), std::move(store), parentId);
}

inline void BuildInput(DataStructure& dataStructure)
{
  const ShapeType cellShape = {k_Dim, k_Dim, k_Dim};

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({k_Dim, k_Dim, k_Dim});
  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataName, cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  auto* featureData = AttributeMatrix::Create(dataStructure, k_FeatureDataName, {3}, imageGeom->getId());

  auto& featureIds = CreateArray<int32>(dataStructure, k_FeatureIdsPath, cellShape, {1}, cellData->getId());
  auto& cellValues = CreateArray<float32>(dataStructure, k_CellValuesPath, cellShape, {k_ValueComponents}, cellData->getId());
  auto& numCells = CreateArray<int32>(dataStructure, k_NumCellsPath, {3}, {1}, featureData->getId());
  auto& numNeighbors = CreateArray<int32>(dataStructure, k_NumNeighborsPath, {3}, {1}, featureData->getId());
  auto& featureMarker = CreateArray<int32>(dataStructure, k_FeatureMarkerPath, {3}, {1}, featureData->getId());

  std::vector<int32> featureIdSlice(k_SliceTuples);
  std::vector<float32> valueSlice(k_SliceTuples * k_ValueComponents);
  for(usize z = 0; z < k_Dim; z++)
  {
    for(usize y = 0; y < k_Dim; y++)
    {
      for(usize x = 0; x < k_Dim; x++)
      {
        const usize inSlice = y * k_Dim + x;
        const usize globalIndex = z * k_SliceTuples + inSlice;
        featureIdSlice[inSlice] = (x % 2 == 0) ? 1 : 2;
        valueSlice[inSlice * k_ValueComponents] = static_cast<float32>(globalIndex % 1024);
        valueSlice[inSlice * k_ValueComponents + 1] = static_cast<float32>(x + 2 * y);
        valueSlice[inSlice * k_ValueComponents + 2] = static_cast<float32>(3 * z + y);
      }
    }

    SIMPLNX_RESULT_REQUIRE_VALID(featureIds.getDataStoreRef().copyFromBuffer(z * k_SliceTuples, nonstd::span<const int32>(featureIdSlice.data(), featureIdSlice.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(cellValues.getDataStoreRef().copyFromBuffer(z * k_SliceTuples * k_ValueComponents, nonstd::span<const float32>(valueSlice.data(), valueSlice.size())));
  }

  const std::array<int32, 3> numCellValues = {0, static_cast<int32>(k_TotalTuples / 2), 1};
  const std::array<int32, 3> neighborValues = {0, 6, 0};
  const std::array<int32, 3> markerValues = {0, 111, 222};
  SIMPLNX_RESULT_REQUIRE_VALID(numCells.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(numCellValues.data(), numCellValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(numNeighbors.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(neighborValues.data(), neighborValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(featureMarker.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(markerValues.data(), markerValues.size())));
}

template <typename T>
uint64 HashStore(const AbstractDataStore<T>& store)
{
  constexpr uint64 k_OffsetBasis = 1469598103934665603ULL;
  constexpr uint64 k_Prime = 1099511628211ULL;
  constexpr usize k_ChunkElements = k_SliceTuples * (std::is_same_v<T, float32> ? k_ValueComponents : 1);
  std::vector<T> buffer(k_ChunkElements);
  uint64 hash = k_OffsetBasis;
  for(usize offset = 0; offset < store.getSize(); offset += buffer.size())
  {
    const usize count = std::min(buffer.size(), store.getSize() - offset);
    store.copyIntoBuffer(offset, nonstd::span<T>(buffer.data(), count));
    for(usize index = 0; index < count; index++)
    {
      uint32 bits = 0;
      if constexpr(std::is_same_v<T, float32>)
      {
        bits = std::bit_cast<uint32>(buffer[index]);
      }
      else
      {
        bits = static_cast<uint32>(buffer[index]);
      }
      hash ^= bits;
      hash *= k_Prime;
    }
  }
  return hash;
}
} // namespace nx::core::FeatureRemovalTest
