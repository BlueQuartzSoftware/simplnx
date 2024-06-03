#include "RemoveFlaggedEdges.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
constexpr usize k_NumVerts = 2;

/**
 * @brief
 * @tparam T
 */
template <typename T>
class CopyCellDataArray
{
public:
  CopyCellDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const std::vector<usize>& newEdgesIndex, const std::atomic_bool& shouldCancel)
  : m_OldCellArray(dynamic_cast<const DataArray<T>&>(oldCellArray))
  , m_NewCellArray(dynamic_cast<DataArray<T>&>(newCellArray))
  , m_NewEdgesIndex(newEdgesIndex)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~CopyCellDataArray() = default;

  CopyCellDataArray(const CopyCellDataArray&) = default;
  CopyCellDataArray(CopyCellDataArray&&) noexcept = default;
  CopyCellDataArray& operator=(const CopyCellDataArray&) = delete;
  CopyCellDataArray& operator=(CopyCellDataArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    size_t numComps = m_OldCellArray.getNumberOfComponents();
    const auto& oldCellData = m_OldCellArray.getDataStoreRef();

    auto& dataStore = m_NewCellArray.getDataStoreRef();
    std::fill(dataStore.begin(), dataStore.end(), static_cast<T>(-1));

    uint64 destTupleIndex = 0;
    for(const auto& srcIndex : m_NewEdgesIndex)
    {
      for(size_t compIndex = 0; compIndex < numComps; compIndex++)
      {
        dataStore.setValue(destTupleIndex * numComps + compIndex, oldCellData.getValue(srcIndex * numComps + compIndex));
      }
      destTupleIndex++;
    }
  }

private:
  const DataArray<T>& m_OldCellArray;
  DataArray<T>& m_NewCellArray;
  const std::vector<usize>& m_NewEdgesIndex;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief The PopulateReducedGeometryEdgesImpl pulls the vertices associated with a triangle then locates the indices in
 * the new VertexList then assigns that "new" triangle to Reduced Geometry
 */
class PopulateReducedGeometryEdgesImpl
{
public:
  PopulateReducedGeometryEdgesImpl(const EdgeGeom& originalTriangle, EdgeGeom& reducedTriangle, const std::vector<usize>& newEdgesIndex, const std::vector<usize>& newVerticesIndex)
  : m_OriginalTriangle(originalTriangle)
  , m_ReducedEgeGeom(reducedTriangle)
  , m_NewEdgesIndex(newEdgesIndex)
  , m_NewVerticesIndex(newVerticesIndex)
  {
  }
  ~PopulateReducedGeometryEdgesImpl() = default;

  PopulateReducedGeometryEdgesImpl(const PopulateReducedGeometryEdgesImpl&) = default;           // Copy Constructor Not Implemented
  PopulateReducedGeometryEdgesImpl(PopulateReducedGeometryEdgesImpl&&) = delete;                 // Move Constructor Not Implemented
  PopulateReducedGeometryEdgesImpl& operator=(const PopulateReducedGeometryEdgesImpl&) = delete; // Copy Assignment Not Implemented
  PopulateReducedGeometryEdgesImpl& operator=(PopulateReducedGeometryEdgesImpl&&) = delete;      // Move Assignment Not Implemented

  void generate(usize start, usize end) const
  {
    for(usize index = start; index < end; index++)
    {
      // pull old vertices
      usize oldVertexIndices[k_NumVerts] = {0, 0};
      m_OriginalTriangle.getEdgePointIds(m_NewEdgesIndex[index], oldVertexIndices);

      // locate new vertex index for each vertex index
      usize newVertexIndices[k_NumVerts] = {0, 0};
      for(usize vertIndex = 0; vertIndex < k_NumVerts; vertIndex++)
      {
        const auto& itr = lower_bound(m_NewVerticesIndex.cbegin(), m_NewVerticesIndex.cend(), oldVertexIndices[vertIndex]); // find first instance of value as iterator
        usize indexOfTarget = std::distance(m_NewVerticesIndex.cbegin(), itr);
        newVertexIndices[vertIndex] = indexOfTarget;
      }

      // set the triangle in reduced
      m_ReducedEgeGeom.setEdgePointIds(index, newVertexIndices);
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const EdgeGeom& m_OriginalTriangle;
  EdgeGeom& m_ReducedEgeGeom;
  const std::vector<usize>& m_NewEdgesIndex;
  const std::vector<usize>& m_NewVerticesIndex;
};

void transferElementData(DataStructure& m_DataStructure, AttributeMatrix& destCellDataAM, const std::vector<DataPath>& sourceDataPaths, const std::vector<usize>& newEdgesIndexList,
                         const std::atomic_bool& m_ShouldCancel, const IFilter::MessageHandler& m_MessageHandler)
{
  // The actual cropping of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  for(const auto& edgeDataArrayPath : sourceDataPaths)
  {
    if(m_ShouldCancel)
    {
      return;
    }

    const auto& oldDataArray = m_DataStructure.getDataRefAs<IDataArray>(edgeDataArrayPath);
    const std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler(fmt::format("Reducing Edge Geometry || Copying Data Array {}", srcName));
    ExecuteParallelFunction<CopyCellDataArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, newEdgesIndexList, m_ShouldCancel);
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
}

} // namespace

// -----------------------------------------------------------------------------
RemoveFlaggedEdges::RemoveFlaggedEdges(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedEdgesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& RemoveFlaggedEdges::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedEdges::operator()()
{
  // Remove Edges from reduced according to removeEdgesIndex
  const auto& originalEdgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->EdgeGeometry);
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
  auto& reducedEdgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->ReducedEdgeGeometry);

  // Set up allocated masks
  usize size = originalEdgeGeom.getNumberOfEdges();
  std::vector<usize> newEdgesIndexList;
  newEdgesIndexList.reserve(size);

  // parse mask Edges list and load a list of indices for Edges to keep
  for(usize index = 0; index < size; index++)
  {
    if(!maskCompare->isTrue(index))
    {
      newEdgesIndexList.push_back(index);
    }
  }
  newEdgesIndexList.shrink_to_fit();

  if(getCancel())
  {
    return {};
  }
  if(newEdgesIndexList.empty())
  {
    return MakeErrorResult(-67880, "Re-evaluate mask conditions - with current configuration all Edges will be stripped!");
  }

  // flatten a list of the indices of vertices used by the Edges
  std::vector<usize> vertexListIndices; // also used as a pseudo look up table in PopulateReducedGeometryEdgesImpl
  usize vertIDs[k_NumVerts] = {0, 0};
  for(usize& index : newEdgesIndexList)
  {

    originalEdgeGeom.getEdgePointIds(index, vertIDs);
    vertexListIndices.push_back(vertIDs[0]);
    vertexListIndices.push_back(vertIDs[1]);
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
  reducedEdgeGeom.resizeVertexList(size); // resize accordingly
  reducedEdgeGeom.getVertexAttributeMatrix()->resizeTuples({size});

  // load reduced Geometry Vertex list according to used vertices
  Point3Df coords = {0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < size; i++)
  {
    coords = originalEdgeGeom.getVertexCoordinate(vertexListIndices[i]);
    reducedEdgeGeom.setVertexCoordinate(i, coords);
  }

  if(getCancel())
  {
    return {};
  }

  // Set up preprocessing conditions (allocation for parallelization)
  size = newEdgesIndexList.size();
  reducedEdgeGeom.resizeEdgeList(size); // resize accordingly
  reducedEdgeGeom.getEdgeAttributeMatrix()->resizeTuples({size});

  // parse Edges and reassign indexes to match new vertex list
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, size);
  dataAlg.execute(PopulateReducedGeometryEdgesImpl(originalEdgeGeom, reducedEdgeGeom, newEdgesIndexList, vertexListIndices));

  /** This section will copy any user defined Edge Data Arrays from the old to the reduced edge geometry **/
  if(m_InputValues->EdgeDataHandling == k_CopySelectedEdgeArraysIdx)
  {
    ::transferElementData(m_DataStructure, reducedEdgeGeom.getEdgeAttributeMatrixRef(), m_InputValues->SelectedEdgeData, newEdgesIndexList, m_ShouldCancel, m_MessageHandler);
  }
  else if(m_InputValues->EdgeDataHandling == k_CopyAllEdgeArraysIdx)
  {
    std::vector<DataPath> ignorePaths;
    auto getChildrenResult = GetAllChildArrayDataPaths(m_DataStructure, m_InputValues->EdgeAttributeMatrixPath, ignorePaths);
    if(getChildrenResult.has_value())
    {
      ::transferElementData(m_DataStructure, reducedEdgeGeom.getEdgeAttributeMatrixRef(), getChildrenResult.value(), newEdgesIndexList, m_ShouldCancel, m_MessageHandler);
    }
  }

  /** This section will copy any user defined Vertex Data Arrays from the old to the reduced Vertex geometry **/
  if(m_InputValues->VertexDataHandling == k_CopySelectedVertexArraysIdx)
  {
    ::transferElementData(m_DataStructure, reducedEdgeGeom.getVertexAttributeMatrixRef(), m_InputValues->SelectedVertexData, vertexListIndices, m_ShouldCancel, m_MessageHandler);
  }
  else if(m_InputValues->VertexDataHandling == k_CopyAllVertexArraysIdx)
  {
    std::vector<DataPath> ignorePaths;
    auto getChildrenResult = GetAllChildArrayDataPaths(m_DataStructure, m_InputValues->VertexAttributeMatrixPath, ignorePaths);
    if(getChildrenResult.has_value())
    {
      ::transferElementData(m_DataStructure, reducedEdgeGeom.getVertexAttributeMatrixRef(), getChildrenResult.value(), vertexListIndices, m_ShouldCancel, m_MessageHandler);
    }
  }

  return {};
}
