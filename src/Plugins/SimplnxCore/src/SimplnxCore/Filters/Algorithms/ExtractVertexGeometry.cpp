#include "ExtractVertexGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct CopyDataFunctor
{
  template <typename T>
  void copyTuple(const AbstractDataStore<T>& srcStore, AbstractDataStore<T>& destStore, usize srcTupleIdx, usize destTupleIndex)
  {
    usize numComps = srcStore.getNumberOfComponents();
    for(usize cIdx = 0; cIdx < numComps; cIdx++)
    {
      destStore[destTupleIndex * numComps + cIdx] = srcStore[srcTupleIdx * numComps + cIdx];
    }
  }

  template <typename T>
  void operator()(const IDataArray* srcIArray, IDataArray* destIArray, const std::vector<bool>& maskArray)
  {
    const auto& srcArray = srcIArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destArray = destIArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    bool useMask = !maskArray.empty();
    usize destTupleIdx = 0;
    usize srcSize = srcArray.getNumberOfTuples();
    for(size_t tupleIdx = 0; tupleIdx < srcSize; tupleIdx++)
    {
      if(useMask && maskArray[tupleIdx])
      {
        copyTuple(srcArray, destArray, tupleIdx, destTupleIdx);
        destTupleIdx++;
      }
      else if(!useMask)
      {
        copyTuple(srcArray, destArray, tupleIdx, tupleIdx);
      }
    }
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
  usize totalCells = cellCount; // We save this here because it may change based on the use_mask flag.
  usize vertexCount = cellCount;
  DataPath vertexAttributeMatrixDataPath = vertexGeometry.getVertexAttributeMatrixDataPath();

  DataPath maskArrayPath = m_InputValues->MaskArrayPath;
  // The mask array may have gotten moved to the vertex array, if so, we need to find that out.
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

  // Copy the mask array into a std::vector<bool>. This is just easier in
  // case the mask array is indeed one of the copied or moved arrays.
  std::vector<bool> maskedPoints;
  if(m_InputValues->UseMask)
  {
    std::unique_ptr<MaskCompare> maskArrayPtr = nullptr;
    try
    {
      maskArrayPtr = std::move(InstantiateMaskCompare(m_DataStructure, maskArrayPath));
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // some other context that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      return MakeErrorResult(-53900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", maskArrayPath.toString()));
    }

    vertexCount = 0;
    maskedPoints.resize(totalCells, false);
    for(usize i = 0; i < totalCells; i++)
    {
      if(maskArrayPtr->isTrue(i))
      {
        maskedPoints[i] = true;
        vertexCount++;
      }
    }
    vertexGeometry.resizeVertexList(vertexCount);
  }

  // Use the APIs from the IGeometryGrid to get the XYZ coord for the center
  // of each cell and then set that into the new VertexGeometry
  IGeometry::SharedVertexList& vertices = vertexGeometry.getVerticesRef();
  auto& verticesDataStore = vertices.getDataStoreRef();
  usize vertIdx = 0;
  for(usize idx = 0; idx < totalCells; idx++)
  {
    if(m_InputValues->UseMask)
    {
      if(maskedPoints[idx])
      {
        const Point3D<float32> coords = inputGeometry.getCoordsf(idx);
        verticesDataStore.setTuple(vertIdx, coords.toArray());
        vertIdx++;
      }
    }
    else
    {
      const Point3D<float32> coords = inputGeometry.getCoordsf(idx);
      verticesDataStore.setTuple(idx, coords.toArray());
    }
  }

  // If we are copying arrays, either with or without a mask, this code is applicable.
  if(m_InputValues->ArrayHandling == to_underlying(ArrayHandlingType::Copy))
  {
    // Since we made copies of the DataArrays, we can safely resize the entire Attribute Matrix,
    // which will resize all the contained DataArrays
    AttributeMatrix& vertexAttrMatrix = vertexGeometry.getVertexAttributeMatrixRef();
    vertexAttrMatrix.resizeTuples({vertexCount});
    for(const auto& dataArrayPath : m_InputValues->IncludedDataArrayPaths)
    {
      const auto* srcIDataArray = m_DataStructure.getDataAs<IDataArray>(dataArrayPath);
      DataPath destDataArrayPath = vertexAttributeMatrixDataPath.createChildPath(srcIDataArray->getName());
      auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destDataArrayPath);
      ExecuteDataFunction(CopyDataFunctor{}, srcIDataArray->getDataType(), srcIDataArray, destDataArray, maskedPoints);
    }
  }

  // If we are MOVING DataArrays, and we are NOT using a Mask then the MoveDataAction
  // took care of renaming/moving the arrays for us and we are done.

  // If we are MOVING arrays AND we are using a mask then we need this code block to execute
  if(m_InputValues->ArrayHandling == to_underlying(ArrayHandlingType::Move) && m_InputValues->UseMask && vertexCount != cellCount)
  {
    // The arrays have already been moved at this point, so the source and
    // destinations are the same. This should work.
    for(const auto& dataArrayPath : m_InputValues->IncludedDataArrayPaths)
    {
      DataPath srcDataArrayPath = vertexAttributeMatrixDataPath.createChildPath(dataArrayPath.getTargetName());
      DataPath destDataArrayPath = vertexAttributeMatrixDataPath.createChildPath(dataArrayPath.getTargetName());
      const auto* srcIDataArray = m_DataStructure.getDataAs<IDataArray>(srcDataArrayPath);
      auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destDataArrayPath);
      ExecuteDataFunction(CopyDataFunctor{}, srcIDataArray->getDataType(), srcIDataArray, destDataArray, maskedPoints);
    }
    AttributeMatrix& vertexAttrMatrix = vertexGeometry.getVertexAttributeMatrixRef();
    vertexAttrMatrix.resizeTuples({vertexCount});
  }

  return {};
}
