#include "ExtractVertexGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"

using namespace complex;

namespace
{
template <typename T>
void copyData(const DataArray<T>& srcArray, DataArray<T>& destArray, std::optional<const BoolArray*> maskArray)
{
  usize destIdx = 0;
  usize srcSize = srcArray.getSize();
  for(size_t idx = 0; idx < srcSize; idx++)
  {
    if(maskArray.has_value())
    {
      if((*maskArray)->at(idx))
      {
        destArray[destIdx] = srcArray[idx];
        destIdx++;
      }
    }
    else
    {
      destArray[idx] = srcArray[idx];
    }
  }
}
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
  const IGridGeometry& inputGeometry = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->InputGeometryPath);
  VertexGeom& vertexGeometry = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->VertexGeometryPath);

  SizeVec3 dims = inputGeometry.getDimensions();
  usize cellCount = std::accumulate(dims.begin(), dims.end(), static_cast<usize>(1), std::multiplies<usize>());
  usize totalCells = cellCount; // We save this here because it may change based on the use_mask flag.

  if(m_InputValues->UseMask)
  {
    const BoolArray& maskArray = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath);
    cellCount = std::count(maskArray.begin(), maskArray.end(), true);

    vertexGeometry.resizeVertexList(cellCount);
  }

  IGeometry::SharedVertexList& vertices = vertexGeometry.getVerticesRef();
  auto& verticesDataStore = vertices.getDataStoreRef();

  // Use the APIs from the IGeometryGrid to get the XYZ coord for the center of each cell and then set that into
  // the new VertexGeometry
  usize vertIdx = 0;
  for(usize idx = 0; idx < totalCells; idx++)
  {
    if(m_InputValues->UseMask)
    {
      const BoolArray& maskArray = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath);
      if(maskArray[idx])
      {
        Point3D<float32> coords = inputGeometry.getCoordsf(idx);
        verticesDataStore.setTuple(vertIdx, coords.toArray());
        vertIdx++;
      }
    }
    else
    {
      Point3D<float32> coords = inputGeometry.getCoordsf(idx);
      verticesDataStore.setTuple(idx, coords.toArray());
    }
  }

  // Copy the data from the cell data arrays to the vertex data arrays
  if(!m_InputValues->IncludedDataArrayPaths.empty())
  {
    DataPath vertexGeomPath = m_InputValues->VertexGeometryPath;
    DataPath vertexAttrMatrixPath = vertexGeomPath.createChildPath(m_InputValues->IncludedDataArrayPaths[0].getParent().getTargetName());
    AttributeMatrix& vertexAttrMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(vertexAttrMatrixPath);

    std::optional<const BoolArray*> maskArray;
    if(m_InputValues->UseMask)
    {
      const BoolArray* maskArrayPtr = m_DataStructure.getDataAs<BoolArray>(m_InputValues->MaskArrayPath);
      cellCount = std::count(maskArrayPtr->begin(), maskArrayPtr->end(), true);
      vertexAttrMatrix.setShape({cellCount});

      for(const auto& srcDataArrayPath : m_InputValues->IncludedDataArrayPaths)
      {
        DataPath destDataArrayPath = vertexAttrMatrixPath.createChildPath(srcDataArrayPath.getTargetName());
        IDataArray& destDataArray = m_DataStructure.getDataRefAs<IDataArray>(destDataArrayPath);
        auto& destDataArrayStore = destDataArray.getIDataStoreRef();
        destDataArrayStore.reshapeTuples({cellCount});
      }

      maskArray = maskArrayPtr;
    }

    for(const auto& dataArrayPath : m_InputValues->IncludedDataArrayPaths)
    {
      const IDataArray& srcIDataArray = m_DataStructure.getDataRefAs<IDataArray>(dataArrayPath);

      DataPath destDataArrayPath = vertexAttrMatrixPath.createChildPath(srcIDataArray.getName());

      switch(srcIDataArray.getDataType())
      {
      case DataType::int8: {
        const Int8Array& srcDataArray = m_DataStructure.getDataRefAs<Int8Array>(dataArrayPath);
        Int8Array& destDataArray = m_DataStructure.getDataRefAs<Int8Array>(destDataArrayPath);
        copyData<int8>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::uint8: {
        const UInt8Array& srcDataArray = m_DataStructure.getDataRefAs<UInt8Array>(dataArrayPath);
        UInt8Array& destDataArray = m_DataStructure.getDataRefAs<UInt8Array>(destDataArrayPath);
        copyData<uint8>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::int16: {
        const Int16Array& srcDataArray = m_DataStructure.getDataRefAs<Int16Array>(dataArrayPath);
        Int16Array& destDataArray = m_DataStructure.getDataRefAs<Int16Array>(destDataArrayPath);
        copyData<int16>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::uint16: {
        const UInt16Array& srcDataArray = m_DataStructure.getDataRefAs<UInt16Array>(dataArrayPath);
        UInt16Array& destDataArray = m_DataStructure.getDataRefAs<UInt16Array>(destDataArrayPath);
        copyData<uint16>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::int32: {
        const Int32Array& srcDataArray = m_DataStructure.getDataRefAs<Int32Array>(dataArrayPath);
        Int32Array& destDataArray = m_DataStructure.getDataRefAs<Int32Array>(destDataArrayPath);
        copyData<int32>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::uint32: {
        const UInt32Array& srcDataArray = m_DataStructure.getDataRefAs<UInt32Array>(dataArrayPath);
        UInt32Array& destDataArray = m_DataStructure.getDataRefAs<UInt32Array>(destDataArrayPath);
        copyData<uint32>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::int64: {
        const Int64Array& srcDataArray = m_DataStructure.getDataRefAs<Int64Array>(dataArrayPath);
        Int64Array& destDataArray = m_DataStructure.getDataRefAs<Int64Array>(destDataArrayPath);
        copyData<int64>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::uint64: {
        const UInt64Array& srcDataArray = m_DataStructure.getDataRefAs<UInt64Array>(dataArrayPath);
        UInt64Array& destDataArray = m_DataStructure.getDataRefAs<UInt64Array>(destDataArrayPath);
        copyData<uint64>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::float32: {
        const Float32Array& srcDataArray = m_DataStructure.getDataRefAs<Float32Array>(dataArrayPath);
        Float32Array& destDataArray = m_DataStructure.getDataRefAs<Float32Array>(destDataArrayPath);
        copyData<float32>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::float64: {
        const Float64Array& srcDataArray = m_DataStructure.getDataRefAs<Float64Array>(dataArrayPath);
        Float64Array& destDataArray = m_DataStructure.getDataRefAs<Float64Array>(destDataArrayPath);
        copyData<float64>(srcDataArray, destDataArray, maskArray);
        break;
      }
      case DataType::boolean: {
        const BoolArray& srcDataArray = m_DataStructure.getDataRefAs<BoolArray>(dataArrayPath);
        BoolArray& destDataArray = m_DataStructure.getDataRefAs<BoolArray>(destDataArrayPath);
        copyData<bool>(srcDataArray, destDataArray, maskArray);
        break;
      }
      }
    }
  }

  return {};
}
