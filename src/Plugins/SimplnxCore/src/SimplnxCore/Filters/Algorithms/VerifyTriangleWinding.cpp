#include "VerifyTriangleWinding.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "Eigen/Core"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
class Mesh
{
public:
  using Self = Mesh;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;

  static Pointer NullPointer()
  {
    return Pointer(static_cast<Self*>(nullptr));
  }

  static Pointer New();

  virtual ~Mesh()
  {
  }
  int getMaxLabel()
  {
    return -1;
  }
  int getMinLabel()
  {
    return -1;
  }
  void setMaxLabel(int32 l)
  {
    m_MaxLabel = l;
  }
  void setMinLabel(int32 l)
  {
    m_MinLabel = l;
  }
  void incrementMaxLabel()
  {
    m_MaxLabel++;
  }

protected:
  Mesh()
  {
  }

private:
  int32 m_MinLabel;
  int32 m_MaxLabel;

  Mesh(const Mesh&);           // Copy Constructor Not Implemented
  void operator=(const Mesh&); // Operator '=' Not Implemented
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
class LabelVisitorInfo
{
public:
  //   typedef Mesh<int32>::Pointer                MeshPtrType;
  typedef TriangleGeom::SharedFaceList::value_type FaceType;
  typedef std::vector<int32> FaceListType;

  using Self = LabelVisitorInfo;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;

  static Pointer NullPointer()
  {
    return Pointer(static_cast<Self*>(nullptr));
  }

  static Pointer New(int32 label, usize tIndex)
  {
    Pointer sharedPtr(new LabelVisitorInfo(label, tIndex));
    return sharedPtr;
  }

  virtual ~LabelVisitorInfo() = default;

private:
  int32 m_Label;

public:
  int32 getLabel()
  {
    if(m_Relabeled)
    {
      return m_NewLabel;
    }
    return m_Label;
  }

  // -----------------------------------------------------------------------------
  void setStartIndex(const usize& value)
  {
    m_StartIndex = value;
  }

  // -----------------------------------------------------------------------------
  usize getStartIndex() const
  {
    return m_StartIndex;
  }

  // -----------------------------------------------------------------------------
  void setPrimed(bool value)
  {
    m_Primed = value;
  }

  // -----------------------------------------------------------------------------
  bool getPrimed() const
  {
    return m_Primed;
  }

  // -----------------------------------------------------------------------------
  void setNewLabel(int32 value)
  {
    m_NewLabel = value;
  }

  // -----------------------------------------------------------------------------
  int32 getNewLabel() const
  {
    return m_NewLabel;
  }

  // -----------------------------------------------------------------------------
  void setRelabeled(bool value)
  {
    m_Relabeled = value;
  }

  // -----------------------------------------------------------------------------
  bool getRelabeled() const
  {
    return m_Relabeled;
  }

  /**
   *
   * @param mesh
   * @param masterVisited
   * @return
   */
  Pointer relabelFaces(Mesh::Pointer mesh, Int32AbstractDataStore& masterFaceList, const std::vector<bool>& masterVisited)
  {
    const usize triangleIndex = *m_Faces.begin();
    const int32 newLabel = mesh->getMaxLabel() + 1;
    mesh->incrementMaxLabel();
    LabelVisitorInfo::Pointer p = LabelVisitorInfo::New(m_Label, triangleIndex);

    p->setPrimed(false);
    p->setNewLabel(newLabel);
    p->setRelabeled(true);
    p->m_Faces = m_Faces;            // This will make a copy of the current set of triangles
    p->m_OriginalFaceList = m_Faces; // Make a copy that does NOT get updated
    bool seedIsSet = false;
    for(auto face : m_Faces)
    {
      if(masterFaceList[face * 2] == m_Label)
      {
        masterFaceList[face * 2] = newLabel;
      }
      if(masterFaceList[face * 2 + 1] == m_Label)
      {
        masterFaceList[face * 2 + 1] = newLabel;
      }
      if(masterVisited[face])
      {
        p->setStartIndex(face);
        seedIsSet = true;
      }
    }
    if(seedIsSet)
    {
      p->setPrimed(true);
    }

    return p;
  }

  /**
   *
   * @param mesh
   */
  void revertFaceLabels(Int32AbstractDataStore& masterFaceList)
  {

    if(m_Relabeled)
    {
      //  qDebug() << "    Reverting Label " << m_NewLabel << " To " << m_Label << "\n";
      for(auto face : m_OriginalFaceList)
      {
        if(masterFaceList[face * 2] == m_NewLabel)
        {
          masterFaceList[face * 2] = m_Label;
        }
        if(masterFaceList[face * 2 + 1] == m_NewLabel)
        {
          masterFaceList[face * 2 + 1] = m_Label;
        }
      }
      m_Relabeled = false;
      m_NewLabel = m_Label;
      m_Primed = false;
    }
  }

  std::set<int32> m_Faces;
  std::set<int32> m_OriginalFaceList;

protected:
  LabelVisitorInfo(int32 label, usize tIndex)
  : m_Label(label)
  , m_StartIndex(tIndex)
  , m_Primed(false)
  , m_NewLabel(label)
  , m_Relabeled(false)
  {
  }

private:
  usize m_StartIndex = {};
  bool m_Primed = {};
  int32 m_NewLabel = {};
  bool m_Relabeled = {};

  LabelVisitorInfo(const LabelVisitorInfo&); // Copy Constructor Not Implemented
  void operator=(const LabelVisitorInfo&);   // Operator '=' Not Implemented
};

typedef std::map<int32, std::set<int>> LabelFaceMap_t;
typedef std::vector<int32> FaceList_t;

// -----------------------------------------------------------------------------
// Groups the triangles according to which Feature they are a part of
// -----------------------------------------------------------------------------
void GetLabelTriangleMap(LabelFaceMap_t& trianglesToLabelMap, const Int32AbstractDataStore& faceLabels, const usize numTraingles)
{
  // Loop over all the triangles and group them according to which feature/region they are a part of
  for(usize i = 0; i < numTraingles; i++)
  {
    usize index = i * 2;
    trianglesToLabelMap[faceLabels[index + 0]].insert(i);
    trianglesToLabelMap[faceLabels[index + 1]].insert(i);
  }
}

Result<> GenerateFaceConnectivity(const DataPath& geomPath, const DataStructure& dataStructure, const std::atomic_bool& shouldCancel)
{
  // We take the path and get the geometry directly here since we are making modifications to arrays and don't want to invalidate refs
  auto* nodeGeometry2DPtr = dataStructure.getDataAs<INodeGeometry2D>(geomPath);
  if(nodeGeometry2DPtr == nullptr)
  {
    return MakeErrorResult(-808, fmt::format("Object at path {} is not a valid 2D geometry.", geomPath.toString()));
  }

  const INodeGeometry2D::SharedFaceList* facesPtr = nodeGeometry2DPtr->getFaces();
  if(nodeGeometry2DPtr == nullptr)
  {
    return MakeErrorResult(-809, fmt::format("Geometry {} does not have a valid SharedFaceList.", nodeGeometry2DPtr->getName()));
  }

  // Make sure the Face Connectivity is created because the FindNRing algorithm needs this and will
  // assert if the data is NOT in the SurfaceMesh Data Container
  if(nullptr == facesPtr->getFacesContainingVert())
  {
    facesPtr->findFacesContainingVert();
  }
  if(shouldCancel)
  {
    return {};
  }
  if(nullptr == facesPtr->getFaceNeighbors())
  {
    facesPtr->findFaceNeighbors();
  }
  if(shouldCancel)
  {
    return {};
  }
  if(facesPtr->getUniqueEdges() == nullptr)
  {
    facesPtr->generateUniqueEdgeIds();
  }
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
VerifyTriangleWinding::VerifyTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             VerifyTriangleWindingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& VerifyTriangleWinding::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VerifyTriangleWinding::operator()()
{
  // Make sure the Face Connectivity is created because the FindNRing algorithm needs this and will
  // assert if the data is NOT in the SurfaceMesh Data Container
  m_MessageHandler("Generating Face List for each Node");
  const Result<> connResult = ::GenerateFaceConnectivity(m_InputValues->TargetGeometryPath, m_DataStructure, m_ShouldCancel);
  if(connResult.invalid() || m_ShouldCancel)
  {
    return connResult;
  }

  // Execute the actual verification step.
  m_MessageHandler("Generating Connectivity Complete. Starting Analysis");
  // -----------------------------------------------------------------------------
  // Previously ValidateTriangleWindingFunction
  // -----------------------------------------------------------------------------
  const auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
  const TriangleGeom::SharedVertexList::store_type& verts = triGeom.getVertices()->getDataStoreRef();
  const TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();

  const Int32AbstractDataStore& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsPath)->getDataStoreRef();

  const usize numFaces = triangles.getNumberOfTuples();

  const Mesh::Pointer mesh = Mesh::New();
  int32 min = std::numeric_limits<int32>::max();
  int32 max = std::numeric_limits<int32>::min();
  // Set the min and Max labels in the Mesh class;
  for(usize i = 0; i < numFaces; ++i)
  {
    int32 firstLabel = faceLabelsStore[i * 2];
    if(firstLabel < min)
    {
      min = firstLabel;
    }
    if(firstLabel > max)
    {
      max = firstLabel;
    }
    int32 secondLabel = faceLabelsStore[i * 2 + 1];
    if(secondLabel < min)
    {
      min = secondLabel;
    }
    if(secondLabel > max)
    {
      max = secondLabel;
    }
  }
  mesh->setMaxLabel(max);
  mesh->setMinLabel(min);

  // Get a grouping of triangles by feature ID
  LabelFaceMap_t trianglesToLabelMap = {};
  ::GetLabelTriangleMap(trianglesToLabelMap, faceLabelsStore, numFaces);

  int32 currentLabel = 0;

  // Find the first non-zero Feature ID (Label). This is going to be our starting feature.
  for(const auto& [key, value] : trianglesToLabelMap)
  {
    if(key > 0)
    {
      currentLabel = key;
      break;
    }
  }

  std::deque<LabelVisitorInfo::Pointer> labelObjectsToVisit;

  // Keeps a list of all the triangles that have been visited.
  std::vector<bool> masterVisited(masterFaceList->getNumberOfTuples(), false);

  STDEXT::hash_set<int32> labelsVisitedSet;
  STDEXT::hash_set<int32> labelsToVisitSet;
  int32 triIndex = 0;

  bool firstLabel = false;

  // Get the first triangle in the list

  // Now that we have the starting Feature, lets try and get a seed triangle that is oriented in the proper direction.
  /* Make sure the winding is correct on the first triangle of the first label that will be checked. */
  // -----------------------------------------------------------------------------
  // Previously GetSeedTriangle function
  // -----------------------------------------------------------------------------
  float32 xMax = std::numeric_limits<float32>::min();
  float32 avgX = 0.0f;
  int32 seedFaceIdx = 0;

  const usize triComp = triangles.getNumberOfComponents();
  for(auto i : trianglesToLabelMap[currentLabel])
  {
    avgX = static_cast<float32>(verts[triangles[i * triComp]] + verts[triangles[(i * triComp) + 1]] + verts[triangles[(i * triComp) + 2]]) / 3.0; // Get X value of all vertices
    if(avgX > xMax)
    {
      xMax = avgX;
      seedFaceIdx = i;
    }
  }
  // Now we have the "right most" triangle based on x component of the centroid of the triangles for this label.

  // Let's now figure out if the normal points generally in the positive or negative X direction.
  Eigen::Vector3f normal;
  const usize vertexIndex = triangles[seedFaceIdx * 3];
  if(faceLabelsStore[(seedFaceIdx * 2) + 0] == currentLabel)
  {
    normal = Eigen::Vector3f(verts[vertexIndex + 0], verts[vertexIndex + 1], verts[vertexIndex + 2]).normalized();
  }
  else
  {
    normal = Eigen::Vector3f(verts[vertexIndex + 2], verts[vertexIndex + 1], verts[vertexIndex + 0]).normalized();
  }

  if(normal[0] < 0.0f) // X value of normal
  {
    const ReverseTriangleWindingFilter filter;

    Arguments args;

    const std::vector<DataPath> pathsToGeom = triGeom.getDataPaths();
    if(pathsToGeom.empty())
    {
      return MakeErrorResult<>(-807, fmt::format("Unable to get paths for geometry: {}", triGeom.getName()));
    }

    args.insertOrAssign(ReverseTriangleWindingFilter::k_TriGeomPath_Key, std::make_any<DataPath>(triGeom.getDataPaths()[0]));

    auto preflightResult = filter.preflight(m_DataStructure, args, m_MessageHandler, m_ShouldCancel);
    if(preflightResult.outputActions.invalid())
    {
      return ConvertResult(std::move(preflightResult.outputActions));
    }

    auto executeResult = filter.execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
    }

    if(faceLabelsStore[(seedFaceIdx * 2) + 0] == currentLabel)
    {
      normal = Eigen::Vector3f(verts[vertexIndex + 0], verts[vertexIndex + 1], verts[vertexIndex + 2]).normalized();
    }
    else
    {
      normal = Eigen::Vector3f(verts[vertexIndex + 2], verts[vertexIndex + 1], verts[vertexIndex + 0]).normalized();
    }
    if(normal[0] < 0.0f) // X value of normal
    {
      seedFaceIdx = -1;
      return MakeErrorResult<>(-802, "Error After attempted triangle winding reversal. Face normal is still oriented in wrong direction.");
    }
  }

  triIndex = seedFaceIdx;

  LabelVisitorInfo::Pointer ldo = LabelVisitorInfo::New(currentLabel, triIndex);
  labelObjectsToVisit.push_back(ldo);
  LabelVisitorInfo::Pointer curLdo = LabelVisitorInfo::NullPointer();
  labelsToVisitSet.insert(currentLabel);

  const float32 total = static_cast<float32>(trianglesToLabelMap.size());
  float32 curPercent = 0.0;
  int32 progressIndex = 0;

  // Start looping on all the Face Labels (Feature Ids) values
  while(labelObjectsToVisit.empty() == false)
  {
    if(getCancel())
    {
      return {};
    }
    if((progressIndex / total * 100.0f) > (curPercent))
    {
      m_MessageHandler(fmt::format("{}% Complete", static_cast<int32>(progressIndex / total * 100.0f)));
      curPercent += 5.0f;
    }
    ++progressIndex;

    std::map<int32, int32> neighborlabels;

    curLdo = labelObjectsToVisit.front();
    labelObjectsToVisit.pop_front();
    currentLabel = curLdo->getLabel();
    labelsToVisitSet.erase(currentLabel);
    labelsVisitedSet.insert(currentLabel);
    if(!curLdo->getPrimed() && !curLdo->getRelabeled())
    {
      curLdo->m_Faces = trianglesToLabelMap[currentLabel];
      curLdo->setPrimed(true);
    }
    else if(!curLdo->getPrimed() && curLdo->getRelabeled())
    {
      curLdo->setStartIndex(*(curLdo->m_Faces.begin()));
      curLdo->setPrimed(true);
    }

    if(firstLabel)
    {
      triIndex = 0;
      firstLabel = false;
    }
    else
    {
      triIndex = curLdo->getStartIndex();
    }
    // Get the number of triangles remaining to be visited for the current label
    usize nLabelTriStart = curLdo->m_Faces.size();

    STDEXT::hash_set<int> localVisited;
    std::deque<int> triangleDeque;
    // FaceArray::Face_t& triangle = triangles[triIndex];
    triangleDeque.push_back(triIndex);

    while(triangleDeque.empty() == false)
    {
      int32 triangleIndex = triangleDeque.front();
      FaceArray::Face_t& triangle = triangles[triangleIndex];
      int32* faceLabel = m_SurfaceMeshFaceLabels + (triangleIndex * 2); // Here we are getting a new pointer offset from the start of the labels array

      //   qDebug() << " $ tIndex: " << triangleIndex << "\n";
      FaceArray::Pointer facesPtr = sm->getFaces();
      if(facesPtr == nullptr)
      {
        break;
      }
      // FaceArray::Face_t* faces = facesPtr->getPointer(0);

      std::vector<int32> adjTris = TriangleOps::findAdjacentTriangles(facesPtr, triangleIndex, m_SurfaceMeshFaceLabelsPtr.lock(), currentLabel);

      for(FaceList_t::iterator adjTri = adjTris.begin(); adjTri != adjTris.end(); ++adjTri)
      {
        if(masterVisited[*adjTri] == false)
        {
          //          if(TriangleOps::verifyWinding(triangle, triangles[*adjTri], faceLabel, m_SurfaceMeshFaceLabels + (*adjTri * 2), currentLabel) == true)
          //          {
          //            // Face Winding Flipped
          //          }
        }

        if(localVisited.find(*adjTri) == localVisited.end() && find(triangleDeque.begin(), triangleDeque.end(), *adjTri) == triangleDeque.end())
        {
          triangleDeque.push_back(*adjTri);
          localVisited.insert(*adjTri);
          masterVisited[*adjTri] = true;
        }
      }

      // Just add the neighbor label to a set so we end up with a list of unique
      // labels that are neighbors to the current label
      if(currentLabel != faceLabel[0])
      {
        neighborlabels.insert(faceLabel[0], triangleIndex);
      }
      if(currentLabel != faceLabel[1])
      {
        neighborlabels.insert(faceLabel[1], triangleIndex);
      }

      localVisited.insert(triangleIndex);
      masterVisited[triangleIndex] = true;
      curLdo->m_Faces.remove(triangleIndex);
      triangleDeque.pop_front();
    } // End of loop to visit all triangles in the 'currentLabel'

    // Find the Next label to push onto the end of the labels List, but ONLY if it is NOT
    // currently on the list and NOT currently on the label visited list.
    for(auto label : neighborlabels)
    {
      int32 triangleIndex = label;
      int32* triangleLabel = m_SurfaceMeshFaceLabels + triangleIndex * 2;

      if(labelsToVisitSet.find(triangleLabel[0]) == labelsToVisitSet.end() && (labelsVisitedSet.find(triangleLabel[0]) == labelsVisitedSet.end()))
      {
        // Push the int32 value into the "set"
        labelsToVisitSet.insert(triangleLabel[0]);
        LabelVisitorInfo::Pointer l = LabelVisitorInfo::New(triangleLabel[0], triangleIndex);
        labelObjectsToVisit.push_back(l);
      }
      if(labelsToVisitSet.find(triangleLabel[1]) == labelsToVisitSet.end() && (labelsVisitedSet.find(triangleLabel[1]) == labelsVisitedSet.end()))
      {
        // Push the int32 value into the "set"
        labelsToVisitSet.insert(triangleLabel[1]);
        LabelVisitorInfo::Pointer l = LabelVisitorInfo::New(triangleLabel[1], triangleIndex);
        labelObjectsToVisit.push_back(l);
      }
    }

    usize nLabelTriEnd = curLdo->m_Faces.size();
    if(nLabelTriStart == nLabelTriEnd)
    {
      // !--!++! No new triangles visited for int32 " << currentLabel << "\n";
    }
    else if(nLabelTriEnd == 0)
    {
      //  No Faces remain to be visited for label
      // If this currentLabel was the result of a "relabeling" then revert back to the original
      // label for those triangles.
      curLdo->revertFaceLabels(m_SurfaceMeshFaceLabelsPtr.lock());
    }
    else
    {
      // Relabel Triangles not yet visited
      LabelVisitorInfo::Pointer p = curLdo->relabelFaces(mesh, m_SurfaceMeshFaceLabelsPtr.lock(), masterVisited);
      labelObjectsToVisit.push_back(p);
      labelsToVisitSet.insert(p->getLabel());
      // Revert the current labelVisitorObject to its original label
      curLdo->revertFaceLabels(m_SurfaceMeshFaceLabelsPtr.lock());
    }
  } // End of Loop over each int32

  if(labelsToVisitSet.size() != 0)
  {
    for(STDEXT::hash_set<int32>::iterator iter = labelsToVisitSet.begin(); iter != labelsToVisitSet.end(); ++iter)
    {
      // Handle label not yet visited
    }
  }

  std::set<int32> thelabels = TriangleOps::generateUniqueLabels(m_SurfaceMeshFaceLabelsPtr.lock());
  for(STDEXT::hash_set<int32>::iterator iter = labelsVisitedSet.begin(); iter != labelsVisitedSet.end(); ++iter)
  {
    thelabels.remove(*iter);
  }

  // TODO:
  //  Verify all labels have been visited by here

  return {};
}
