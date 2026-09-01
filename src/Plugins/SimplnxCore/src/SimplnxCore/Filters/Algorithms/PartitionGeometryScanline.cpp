#include "PartitionGeometryScanline.hpp"

#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Writes sequential IDs to a created partition-grid array.
 * @param featureIdsStore Receives partition-grid Feature IDs.
 * @param startingFeatureId Specifies the first ID.
 * @param shouldCancel Stops before later output chunks when true.
 * @return First output error, or success after completion or cancellation.
 */
Result<> WritePartitionGridFeatureIds(Int32AbstractDataStore& featureIdsStore, int32 startingFeatureId, const std::atomic_bool& shouldCancel)
{
  const usize numTuples = featureIdsStore.getNumberOfTuples();
  auto featureIdsBuffer = std::make_unique<int32[]>(std::min(k_ChunkTuples, numTuples));

  for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, numTuples - offset);
    for(usize index = 0; index < count; index++)
    {
      featureIdsBuffer[index] = static_cast<int32>(offset + index) + startingFeatureId;
    }

    Result<> writeResult = featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(featureIdsBuffer.get(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  return {};
}
} // namespace

PartitionGeometryScanline::PartitionGeometryScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     PartitionGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

PartitionGeometryScanline::~PartitionGeometryScanline() noexcept = default;

const std::atomic_bool& PartitionGeometryScanline::getCancel()
{
  return m_ShouldCancel;
}

Result<> PartitionGeometryScanline::operator()()
{
  auto partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->PartitioningMode);

  DataPath partitionGridGeomPath;
  if(partitioningMode == PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid)
  {
    partitionGridGeomPath = m_InputValues->ExistingPartitionGridPath;
  }
  else
  {
    partitionGridGeomPath = m_InputValues->PartitionGridGeomPath;
    const DataPath partitionGridFeatureIdsPath =
        m_InputValues->PartitionGridGeomPath.createChildPath(m_InputValues->PartitionGridCellAMName).createChildPath(m_InputValues->PartitionGridFeatureIDsArrayName);
    auto& pgFeatureIdsStore = m_DataStructure.getDataAs<Int32Array>(partitionGridFeatureIdsPath)->getDataStoreRef();
    Result<> writeFeatureIdsResult = WritePartitionGridFeatureIds(pgFeatureIdsStore, m_InputValues->StartingFeatureID, m_ShouldCancel);
    if(writeFeatureIdsResult.invalid() || m_ShouldCancel)
    {
      return writeFeatureIdsResult;
    }
  }

  const ImageGeom& partitionGridGeom = m_DataStructure.getDataRefAs<ImageGeom>({partitionGridGeomPath});

  std::optional<BoolArray> vertexMask = {};
  if(m_InputValues->UseVertexMask)
  {
    vertexMask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->VertexMaskPath);
  }

  const DataPath partitionIdsPath = m_InputValues->InputGeomCellAMPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  auto& partitionIdsStore = m_DataStructure.getDataAs<Int32Array>(partitionIdsPath)->getDataStoreRef();

  const IGeometry& iGeomToPartition = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->InputGeometryToPartition);
  Result<> result;
  switch(iGeomToPartition.getGeomType())
  {
  case IGeometry::Type::Image: {
    const ImageGeom& inputGeomToPartition = m_DataStructure.getDataRefAs<ImageGeom>({m_InputValues->InputGeometryToPartition});
    result = partitionCellBasedGeometry(inputGeomToPartition, partitionIdsStore, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID);
    break;
  }
  case IGeometry::Type::RectGrid: {
    const RectGridGeom& inputGeomToPartition = m_DataStructure.getDataRefAs<RectGridGeom>({m_InputValues->InputGeometryToPartition});
    result = partitionCellBasedGeometry(inputGeomToPartition, partitionIdsStore, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID);
    break;
  }
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const INodeGeometry0D& inputGeomToPartition = m_DataStructure.getDataRefAs<INodeGeometry0D>({m_InputValues->InputGeometryToPartition});
    const AbstractDataStore<IGeometry::SharedVertexList::value_type>& vertexListStore = inputGeomToPartition.getVertices()->getDataStoreRef();
    result = partitionNodeBasedGeometry(vertexListStore, partitionIdsStore, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID, vertexMask);
    break;
  }
  default: {
    return MakeErrorResult(-3012, fmt::format("Unable to partition geometry at path '{}' - Unknown geometry type detected.", m_InputValues->InputGeometryToPartition.toString()));
  }
  }

  if(result.invalid())
  {
    return result;
  }

  return {};
}

Result<> PartitionGeometryScanline::partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue)
{
  const usize numTuples = partitionIdsStore.getNumberOfTuples();
  auto partitionIdsBuffer = std::make_unique<int32[]>(std::min(k_ChunkTuples, numTuples));

  for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, numTuples - offset);
    for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
    {
      const Point3D<float64> coord = inputGeometry.getCoords(offset + chunkIndex);
      const auto partitionIndexResult = psImageGeom.getIndex(coord[0], coord[1], coord[2]);
      partitionIdsBuffer[chunkIndex] = partitionIndexResult.has_value() ? static_cast<int32>(*partitionIndexResult) + m_InputValues->StartingFeatureID : outOfBoundsValue;
    }

    Result<> writeResult = partitionIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(partitionIdsBuffer.get(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  return {};
}

Result<> PartitionGeometryScanline::partitionNodeBasedGeometry(const VertexStore& vertexListStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                                               const std::optional<const BoolArray>& maskArrayOpt)
{
  const usize numTuples = vertexListStore.getNumberOfTuples();
  auto verticesBuffer = std::make_unique<float32[]>(std::min(k_ChunkTuples, numTuples) * 3);
  auto partitionIdsBuffer = std::make_unique<int32[]>(std::min(k_ChunkTuples, numTuples));
  auto maskBuffer = maskArrayOpt.has_value() ? std::make_unique<bool[]>(std::min(k_ChunkTuples, numTuples)) : nullptr;

  for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, numTuples - offset);
    Result<> verticesReadResult = vertexListStore.copyIntoBuffer(offset * 3, nonstd::span<float32>(verticesBuffer.get(), count * 3));
    if(verticesReadResult.invalid())
    {
      return verticesReadResult;
    }

    if(maskArrayOpt.has_value())
    {
      Result<> maskReadResult = maskArrayOpt->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(maskBuffer.get(), count));
      if(maskReadResult.invalid())
      {
        return maskReadResult;
      }
    }

    for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
    {
      const usize vertexOffset = chunkIndex * 3;
      const auto partitionIndexResult = psImageGeom.getIndex(verticesBuffer[vertexOffset], verticesBuffer[vertexOffset + 1], verticesBuffer[vertexOffset + 2]);
      if((maskArrayOpt.has_value() && !maskBuffer[chunkIndex]) || !partitionIndexResult.has_value())
      {
        partitionIdsBuffer[chunkIndex] = outOfBoundsValue;
      }
      else
      {
        partitionIdsBuffer[chunkIndex] = static_cast<int32>(*partitionIndexResult) + m_InputValues->StartingFeatureID;
      }
    }

    Result<> writeResult = partitionIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(partitionIdsBuffer.get(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  return {};
}
