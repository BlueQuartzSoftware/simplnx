#include "RemoveFlaggedTriangles.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief The PopulateReducedGeometryTrianglesImpl pulls the vertices associated with a triangle then locates the indices in
 * the new VertexList then assigns that "new" triangle to Reduced Geometry
 */
class PopulateReducedGeometryTrianglesImpl
{
public:
  PopulateReducedGeometryTrianglesImpl(const TriangleGeom& originalTriangle, TriangleGeom& reducedTriangle, const std::vector<usize>& newTrianglesIndex, const std::vector<usize>& newVerticesIndex)
  : m_OriginalTriangle(originalTriangle)
  , m_ReducedTriangle(reducedTriangle)
  , m_NewTrianglesIndex(newTrianglesIndex)
  , m_NewVerticesIndex(newVerticesIndex)
  {
  }
  ~PopulateReducedGeometryTrianglesImpl() = default;

  void generate(usize start, usize end) const
  {
    for(usize index = start; index < end; index++)
    {
      // pull old vertices
      usize oldVertexIndices[3] = {0, 0, 0};
      m_OriginalTriangle.getFacePointIds(m_NewTrianglesIndex[index], oldVertexIndices);

      // locate new vertex index for each vertex index
      usize newVertexIndices[3] = {0, 0, 0};
      for(usize vertIndex = 0; vertIndex < 3; vertIndex++)
      {
        const auto& itr = lower_bound(m_NewVerticesIndex.cbegin(), m_NewVerticesIndex.cend(), oldVertexIndices[vertIndex]); // find first instance of value as iterator
        usize indexOfTarget = std::distance(m_NewVerticesIndex.cbegin(), itr);
        newVertexIndices[vertIndex] = indexOfTarget;
      }

      // set the triangle in reduced
      m_ReducedTriangle.setFacePointIds(index, newVertexIndices);
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const TriangleGeom& m_OriginalTriangle;
  TriangleGeom& m_ReducedTriangle;
  const std::vector<usize>& m_NewTrianglesIndex;
  const std::vector<usize>& m_NewVerticesIndex;
};
} // namespace

// -----------------------------------------------------------------------------
RemoveFlaggedTriangles::RemoveFlaggedTriangles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               RemoveFlaggedTrianglesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& RemoveFlaggedTriangles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedTriangles::operator()()
{
  // Remove Triangles from reduced according to removeTrianglesIndex
  const auto& originalTriangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometry);
  std::unique_ptr<MaskCompare> maskCompare;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54070, message);
  }
  auto& reducedTriangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->ReducedTriangleGeometry);

  // Set up allocated masks
  usize size = originalTriangle.getNumberOfFaces();
  std::vector<usize> newTrianglesIndexList;
  newTrianglesIndexList.reserve(size);

  // parse mask Triangles list and load a list of indices for triangles to keep
  for(usize index = 0; index < size; index++)
  {
    if(!maskCompare->isTrue(index))
    {
      newTrianglesIndexList.push_back(index);
    }
  }
  newTrianglesIndexList.shrink_to_fit();

  if(getCancel())
  {
    return {};
  }
  if(newTrianglesIndexList.empty())
  {
    return MakeErrorResult(-67880, "Re-evaluate mask conditions - with current configuration all triangles will be stripped!");
  }

  // flatten a list of the indices of vertices used by the triangles
  std::vector<usize> vertexListIndices; // also used as a pseudo look up table in PopulateReducedGeometryTrianglesImpl
  for(usize& index : newTrianglesIndexList)
  {
    usize vertIDs[3] = {0, 0, 0};
    originalTriangle.getFacePointIds(index, vertIDs);
    vertexListIndices.push_back(vertIDs[0]);
    vertexListIndices.push_back(vertIDs[1]);
    vertexListIndices.push_back(vertIDs[2]);
  }
  if(getCancel())
  {
    return {};
  }

  if(vertexListIndices.empty())
  {
    return MakeErrorResult(-67881, "Re-evaluate mask conditions - with current configuration all vertices will be dumped!");
  }

  // clear duplicate values out of vector
  std::sort(vertexListIndices.begin(), vertexListIndices.end()); // orders ascending !!!!! Basis for later search !!!!!
  auto dupes = std::unique(vertexListIndices.begin(), vertexListIndices.end());
  vertexListIndices.erase(dupes, vertexListIndices.end());

  // define new sizing
  size = vertexListIndices.size();
  reducedTriangleGeom.resizeVertexList(size); // resize accordingly
  reducedTriangleGeom.getVertexAttributeMatrix()->resizeTuples({size});

  // load reduced Geometry Vertex list according to used vertices
  Point3Df coords = {0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < size; i++)
  {
    coords = originalTriangle.getVertexCoordinate(vertexListIndices[i]);
    reducedTriangleGeom.setVertexCoordinate(i, coords);
  }

  if(getCancel())
  {
    return {};
  }

  // Set up preprocessing conditions (allocation for parallelization)
  size = newTrianglesIndexList.size();
  reducedTriangleGeom.resizeFaceList(size); // resize accordingly
  reducedTriangleGeom.getFaceAttributeMatrix()->resizeTuples({size});

  // parse triangles and reassign indexes to match new vertex list
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, size);
  dataAlg.execute(PopulateReducedGeometryTrianglesImpl(originalTriangle, reducedTriangleGeom, newTrianglesIndexList, vertexListIndices));

  /** This section will copy any user defined Triangle Data Arrays from the old to the reduced Triangle geometry **/
  if(m_InputValues->TriangleDataHandling == k_CopySelectedTriangleArraysIdx)
  {
    TransferGeometryElementData::transferElementData(m_DataStructure, reducedTriangleGeom.getFaceAttributeMatrixRef(), m_InputValues->SelectedTriangleData, newTrianglesIndexList, m_ShouldCancel,
                                                     m_MessageHandler);
  }
  else if(m_InputValues->TriangleDataHandling == k_CopyAllTriangleArraysIdx)
  {
    std::vector<DataPath> ignorePaths;
    auto getChildrenResult = GetAllChildArrayDataPaths(m_DataStructure, m_InputValues->TriangleAttributeMatrixPath, ignorePaths);
    if(getChildrenResult.has_value())
    {
      TransferGeometryElementData::transferElementData(m_DataStructure, reducedTriangleGeom.getFaceAttributeMatrixRef(), getChildrenResult.value(), newTrianglesIndexList, m_ShouldCancel,
                                                       m_MessageHandler);
    }
  }

  /** This section will copy any user defined Vertex Data Arrays from the old to the reduced Vertex geometry **/
  if(m_InputValues->VertexDataHandling == k_CopySelectedVertexArraysIdx)
  {
    TransferGeometryElementData::transferElementData(m_DataStructure, reducedTriangleGeom.getVertexAttributeMatrixRef(), m_InputValues->SelectedVertexData, vertexListIndices, m_ShouldCancel,
                                                     m_MessageHandler);
  }
  else if(m_InputValues->VertexDataHandling == k_CopyAllVertexArraysIdx)
  {
    std::vector<DataPath> ignorePaths;
    auto getChildrenResult = GetAllChildArrayDataPaths(m_DataStructure, m_InputValues->VertexAttributeMatrixPath, ignorePaths);
    if(getChildrenResult.has_value())
    {
      TransferGeometryElementData::transferElementData(m_DataStructure, reducedTriangleGeom.getVertexAttributeMatrixRef(), getChildrenResult.value(), vertexListIndices, m_ShouldCancel,
                                                       m_MessageHandler);
    }
  }

  return {};
}
