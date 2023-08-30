#include "RemoveFlaggedTriangles.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

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
  virtual ~PopulateReducedGeometryTrianglesImpl() = default;

  void generate(usize start, usize end) const
  {
    for(usize index = start; index < end; index++)
    {
      // pull old vertices
      Point3Df oldVertexIndices = m_OriginalTriangle.getVertexCoordinate(m_NewTrianglesIndex[index]);

      // locate new vertex index for each vertex index
      usize newVertexIndices[3] = {0, 0, 0};
      for(usize vertIndex = 0; vertIndex < 3; vertIndex++)
      {
        const auto& itr = lower_bound(m_NewVerticesIndex.begin(), m_NewVerticesIndex.end(), oldVertexIndices[vertIndex]); // find first instance of value as iterator
        usize indexOfTarget = std::distance(m_NewVerticesIndex.begin(), itr);
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
RemoveFlaggedTriangles::~RemoveFlaggedTriangles() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RemoveFlaggedTriangles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedTriangles::operator()()
{
  // Remove Triangles from reduced according to removeTrianglesIndex
  auto& originalTriangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometry);
  auto& reducedTriangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->ReducedTriangleGeometry);
  auto& mask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath);

  // Set up allocated masks
  auto size = originalTriangle.getNumberOfFaces();
  std::vector<usize> newTrianglesIndexList;
  newTrianglesIndexList.reserve(size);

  // parse mask Triangles list and load a list of indices for triangles to keep
  for(usize index = 0; index < size; index++)
  {
    if(!mask[index])
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
  std::vector<usize> pseudoVertexList; // also used as a pseudo look up table in PopulateReducedGeometryTrianglesImpl
  for(usize& index : newTrianglesIndexList)
  {
    usize vertIDs[3] = {0, 0, 0};
    originalTriangle.getFacePointIds(index, vertIDs);
    pseudoVertexList.push_back(vertIDs[0]);
    pseudoVertexList.push_back(vertIDs[1]);
    pseudoVertexList.push_back(vertIDs[2]);
  }
  if(getCancel())
  {
    return {};
  }

  if(pseudoVertexList.empty())
  {
    return MakeErrorResult(-67881, "Re-evaluate mask conditions - with current configuration all vertices will be dumped!");
  }

  // clear duplicate values out of vector
  std::sort(pseudoVertexList.begin(), pseudoVertexList.end()); // orders ascending !!!!! Basis for later search !!!!!
  auto dupes = std::unique(pseudoVertexList.begin(), pseudoVertexList.end());
  pseudoVertexList.erase(dupes, pseudoVertexList.end());

  // define new sizing
  size = pseudoVertexList.size();
  reducedTriangle.resizeVertexList(size); // resize accordingly

  // load reduced Geometry Vertex list according to used vertices
  Point3Df coords = {0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < size; i++)
  {
    coords = originalTriangle.getVertexCoordinate(pseudoVertexList[i]);
    reducedTriangle.setVertexCoordinate(i, coords);
  }

  if(getCancel())
  {
    return {};
  }

  // Set up preprocessing conditions (allocation for parallelization)
  size = newTrianglesIndexList.size();
  reducedTriangle.resizeFaceList(size); // resize accordingly

  // parse triangles and reassign indexes to match new vertex list
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, size);
  dataAlg.execute(PopulateReducedGeometryTrianglesImpl(originalTriangle, reducedTriangle, newTrianglesIndexList, pseudoVertexList));

  return {};
}
