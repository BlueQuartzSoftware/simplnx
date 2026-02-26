#include "CropVertexGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct CopyDataToCroppedGeometryFunctor
{
  template <typename T>
  void operator()(const IDataArray* inDataRef, IDataArray* outDataRef, const std::vector<int64>& croppedPoints)
  {
    const auto& inputData = inDataRef->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& croppedData = outDataRef->template getIDataStoreRefAs<AbstractDataStore<T>>();

    usize nComps = inDataRef->getNumberOfComponents();

    for(std::vector<int64>::size_type i = 0; i < croppedPoints.size(); i++)
    {
      for(usize d = 0; d < nComps; d++)
      {
        usize tmpIndex = nComps * i + d;
        usize ptrIndex = nComps * croppedPoints[i] + d;
        croppedData[tmpIndex] = inputData[ptrIndex];
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
CropVertexGeometry::CropVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropVertexGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CropVertexGeometry::~CropVertexGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CropVertexGeometry::operator()()
{
  auto posMin = m_InputValues->MinPos;
  auto posMax = m_InputValues->MaxPos;

  auto xMin = posMin[0];
  auto yMin = posMin[1];
  auto zMin = posMin[2];
  auto xMax = posMax[0];
  auto yMax = posMax[1];
  auto zMax = posMax[2];

  auto& vertices = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->InputVertexGeometryPath);
  auto numVerts = static_cast<int64>(vertices.getNumberOfVertices());
  auto* verticesPtr = vertices.getVertices();
  auto& allVerts = verticesPtr->getDataStoreRef();
  std::vector<int64> croppedPoints;
  croppedPoints.reserve(numVerts);

  for(int64 i = 0; i < numVerts; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(allVerts[3 * i + 0] >= xMin && allVerts[3 * i + 0] <= xMax && allVerts[3 * i + 1] >= yMin && allVerts[3 * i + 1] <= yMax && allVerts[3 * i + 2] >= zMin && allVerts[3 * i + 2] <= zMax)
    {
      croppedPoints.push_back(i);
    }
  }

  croppedPoints.shrink_to_fit();

  auto& crop = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->OutputVertexGeometryPath);
  usize numTuples = croppedPoints.size();
  crop.resizeVertexList(numTuples);
  ShapeType tDims = {numTuples};

  DataPath croppedVertexDataPath = m_InputValues->OutputVertexGeometryPath.createChildPath(m_InputValues->VertexAttributeMatrixName);
  auto& vertexDataAttMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(croppedVertexDataPath);
  vertexDataAttMatrix.resizeTuples(tDims);

  for(usize i = 0; i < numTuples; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto coords = vertices.getVertexCoordinate(croppedPoints[i]);
    crop.setVertexCoordinate(i, coords);
  }

  for(auto&& targetArrayPath : m_InputValues->TargetArrayPaths)
  {
    DataPath destArrayPath(croppedVertexDataPath.createChildPath(targetArrayPath.getTargetName()));

    const auto* srcArray = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* destArray = m_DataStructure.getDataAs<IDataArray>(destArrayPath);

    ExecuteDataFunction(CopyDataToCroppedGeometryFunctor{}, srcArray->getDataType(), srcArray, destArray, croppedPoints);
  }

  return {};
}
