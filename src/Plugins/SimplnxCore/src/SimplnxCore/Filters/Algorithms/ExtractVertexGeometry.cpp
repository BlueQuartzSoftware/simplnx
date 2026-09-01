#include "ExtractVertexGeometry.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
// Limit each mask and included-array transfer to a fixed tuple count.
constexpr usize k_ChunkTuples = 65536;

/**
 * @struct MaskChunkFunctor
 * @brief Normalizes one Bool or UInt8 mask chunk to Boolean flags.
 *
 * Consumers re-read the mask instead of keeping a cell-sized flag array. The
 * bulk-read Result is ignored.
 */
struct MaskChunkFunctor
{
  template <typename T>
  void operator()(const IDataArray* maskIArray, usize chunkStart, usize count, nonstd::span<bool> outFlags)
  {
    const auto& maskStore = maskIArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto buffer = std::make_unique<T[]>(count);
    maskStore.copyIntoBuffer(chunkStart, nonstd::span<T>(buffer.get(), count));
    for(usize i = 0; i < count; i++)
    {
      // Nonzero UInt8 values match MaskCompareUtilities mask semantics.
      outFlags[i] = static_cast<bool>(buffer[i]);
    }
  }
};

/**
 * @struct CopyDataFunctor
 * @brief Compacts one included cell array into vertex tuple order.
 *
 * Source and output values use bounded bulk transfers. Masked copying re-reads
 * each mask chunk. The function does not check cancellation, and it ignores all
 * transfer Result values.
 */
struct CopyDataFunctor
{
  template <typename T>
  void operator()(const IDataArray* srcIArray, IDataArray* destIArray, const IDataArray* maskIArray)
  {
    const auto& srcStore = srcIArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destStore = destIArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize numComps = srcStore.getNumberOfComponents();
    const usize srcTuples = srcStore.getNumberOfTuples();
    const usize chunkTuples = std::min(srcTuples, k_ChunkTuples);

    if(maskIArray == nullptr)
    {
      // Without a mask, source and destination tuple offsets are equal.
      auto buffer = std::make_unique<T[]>(chunkTuples * numComps);
      for(usize start = 0; start < srcTuples; start += k_ChunkTuples)
      {
        const usize count = std::min(k_ChunkTuples, srcTuples - start);
        srcStore.copyIntoBuffer(start * numComps, nonstd::span<T>(buffer.get(), count * numComps));
        destStore.copyFromBuffer(start * numComps, nonstd::span<const T>(buffer.get(), count * numComps));
      }
      return;
    }

    // Compact selected tuples in source order and flush full output chunks.
    auto inBuffer = std::make_unique<T[]>(chunkTuples * numComps);
    auto outBuffer = std::make_unique<T[]>(chunkTuples * numComps);
    auto flagBuffer = std::make_unique<bool[]>(chunkTuples);
    usize outTuples = 0;
    usize destOffset = 0;

    auto flushOut = [&]() {
      if(outTuples == 0)
      {
        return;
      }
      destStore.copyFromBuffer(destOffset * numComps, nonstd::span<const T>(outBuffer.get(), outTuples * numComps));
      destOffset += outTuples;
      outTuples = 0;
    };

    for(usize chunkStart = 0; chunkStart < srcTuples; chunkStart += k_ChunkTuples)
    {
      const usize chunkCount = std::min(k_ChunkTuples, srcTuples - chunkStart);
      ExecuteDataFunction(MaskChunkFunctor{}, maskIArray->getDataType(), maskIArray, chunkStart, chunkCount, nonstd::span<bool>(flagBuffer.get(), chunkCount));
      srcStore.copyIntoBuffer(chunkStart * numComps, nonstd::span<T>(inBuffer.get(), chunkCount * numComps));

      for(usize i = 0; i < chunkCount; i++)
      {
        if(!flagBuffer[i])
        {
          continue;
        }
        const T* srcTuple = inBuffer.get() + i * numComps;
        T* dstTuple = outBuffer.get() + outTuples * numComps;
        std::copy(srcTuple, srcTuple + numComps, dstTuple);
        outTuples++;
        if(outTuples == k_ChunkTuples)
        {
          flushOut();
        }
      }
    }
    flushOut();
  }
};
} // namespace

// -----------------------------------------------------------------------------
ExtractVertexGeometry::ExtractVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ExtractVertexGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractVertexGeometry::~ExtractVertexGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractVertexGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractVertexGeometry::operator()()
{
  const auto& inputGeometry = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->InputGeometryPath);
  auto& vertexGeometry = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->VertexGeometryPath);

  SizeVec3 dims = inputGeometry.getDimensions();
  const usize cellCount = std::accumulate(dims.begin(), dims.end(), static_cast<usize>(1), std::multiplies<>());
  usize totalCells = cellCount;
  usize vertexCount = cellCount;
  DataPath vertexAttributeMatrixDataPath = vertexGeometry.getVertexAttributeMatrixDataPath();

  DataPath maskArrayPath = m_InputValues->MaskArrayPath;
  // Resolve a mask that filter actions placed under the vertex AttributeMatrix.
  if(m_InputValues->UseMask && !m_DataStructure.containsData(m_InputValues->MaskArrayPath))
  {
    if(!m_InputValues->IncludedDataArrayPaths.empty() && m_InputValues->UseMask)
    {
      for(const auto& dataPath : m_InputValues->IncludedDataArrayPaths)
      {
        if(dataPath == m_InputValues->MaskArrayPath)
        {
          maskArrayPath = vertexAttributeMatrixDataPath.createChildPath(m_InputValues->MaskArrayPath.getTargetName());
        }
      }
    }
  }

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Preparing arrays for extraction..."));

  // Stream the mask for counting, coordinates, and each included array.
  // Repeated reads avoid a cell-sized keep bitmap.
  const IDataArray* maskIDataArray = nullptr;
  if(m_InputValues->UseMask)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    try
    {
      // Validate the mask path and type. Streaming does not use the returned adapter.
      MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, maskArrayPath);
    } catch(const std::out_of_range&)
    {
      // Direct algorithm calls can bypass filter preflight.
      return MakeErrorResult(-53900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", maskArrayPath.toString()));
    }

    maskIDataArray = m_DataStructure.getDataAs<IDataArray>(maskArrayPath);

    // Count selected tuples with one bounded flag chunk.
    const usize chunkTuples = std::min(totalCells, k_ChunkTuples);
    auto flagBuffer = std::make_unique<bool[]>(chunkTuples);
    vertexCount = 0;
    for(usize chunkStart = 0; chunkStart < totalCells; chunkStart += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, totalCells - chunkStart);
      ExecuteDataFunction(MaskChunkFunctor{}, maskIDataArray->getDataType(), maskIDataArray, chunkStart, count, nonstd::span<bool>(flagBuffer.get(), count));
      for(usize i = 0; i < count; i++)
      {
        if(flagBuffer[i])
        {
          vertexCount++;
        }
      }
    }
    vertexGeometry.resizeVertexList(vertexCount);
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // getCoordsf() derives each cell center from geometry metadata. Vertex
  // setTuple() writes follow selected source order. A mask requires another pass.
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Generating vertex geometry"));

  IGeometry::SharedVertexList& vertices = vertexGeometry.getVerticesRef();
  auto& verticesDataStore = vertices.getDataStoreRef();
  if(m_InputValues->UseMask)
  {
    const usize chunkTuples = std::min(totalCells, k_ChunkTuples);
    auto flagBuffer = std::make_unique<bool[]>(chunkTuples);
    usize vertIdx = 0;
    for(usize chunkStart = 0; chunkStart < totalCells; chunkStart += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, totalCells - chunkStart);
      ExecuteDataFunction(MaskChunkFunctor{}, maskIDataArray->getDataType(), maskIDataArray, chunkStart, count, nonstd::span<bool>(flagBuffer.get(), count));
      for(usize i = 0; i < count; i++)
      {
        if(flagBuffer[i])
        {
          const Point3D<float32> coords = inputGeometry.getCoordsf(chunkStart + i);
          verticesDataStore.setTuple(vertIdx, coords.toArray());
          vertIdx++;
        }
      }
    }
  }
  else
  {
    for(usize idx = 0; idx < totalCells; idx++)
    {
      const Point3D<float32> coords = inputGeometry.getCoordsf(idx);
      verticesDataStore.setTuple(idx, coords.toArray());
    }
  }

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Copying cell data to vertex geometry"));

  // Resize all preflight-created vertex arrays to the selected tuple count.
  AttributeMatrix& vertexAttrMatrix = vertexGeometry.getVertexAttributeMatrixRef();
  vertexAttrMatrix.resizeTuples({vertexCount});

  for(const auto& dataArrayPath : m_InputValues->IncludedDataArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* srcIDataArray = m_DataStructure.getDataAs<IDataArray>(dataArrayPath);
    DataPath destDataArrayPath = vertexAttributeMatrixDataPath.createChildPath(srcIDataArray->getName());
    auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destDataArrayPath);
    ExecuteDataFunction(CopyDataFunctor{}, srcIDataArray->getDataType(), srcIDataArray, destDataArray, maskIDataArray);
  }

  return {};
}
