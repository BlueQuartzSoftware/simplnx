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
Result<> RepairWindings


Result<usize> FindValidSeed(const int32 targetFeatureId, const TriangleGeom::SharedFaceList::store_type& triangles, const TriangleGeom::SharedVertexList::store_type& verts, const Int32AbstractDataStore& faceLabels, std::vector<bool>& reversalVote)
{
  usize seedFaceIdx = 0;
  float32 xMax = std::numeric_limits<float32>::min();
  float32 avgX = 0.0f;

  // walk the nodes within targetFeature
  const usize triComp = triangles.getNumberOfComponents();
  for(usize i = 0; i++ < faceLabels.getNumberOfTuples(); i++)
  {
    if(faceLabels[(i * 2) + 0] == targetFeatureId || faceLabels[(i * 2) + 1] == targetFeatureId)
    {
      avgX = static_cast<float32>(verts[triangles[i * triComp]] + verts[triangles[(i * triComp) + 1]] + verts[triangles[(i * triComp) + 2]]) / 3.0; // Get X value of all vertices
      if(avgX > xMax)
      {
        xMax = avgX;
        seedFaceIdx = i;
      }
    }
  }
  // Now we have the "right most" triangle based on x component of the centroid of the triangles for this label.

  // Let's now figure out if the normal points generally in the positive or negative X direction.
  Eigen::Vector3f normal;
  const usize vertexIndex = triangles[seedFaceIdx * 3];
  if(faceLabels[(seedFaceIdx * 2) + 0] == targetFeatureId)
  {
    normal = Eigen::Vector3f(verts[vertexIndex + 0], verts[vertexIndex + 1], verts[vertexIndex + 2]).normalized();
  }
  else
  {
    normal = Eigen::Vector3f(verts[vertexIndex + 2], verts[vertexIndex + 1], verts[vertexIndex + 0]).normalized();
  }

  if(normal[0] < 0.0f) // X value of normal
  {
    reversalVote[targetFeatureId] = true;
  }

  return {seedFaceIdx};
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
  // Load container, node list, and node data respectively
  const auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
  const TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();
  const TriangleGeom::SharedVertexList::store_type& verts = triGeom.getVertices()->getDataStoreRef();

  // Load double-sided mesh grouping
  const Int32AbstractDataStore& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsPath)->getDataStoreRef();

  // Get max group (feature id != 0)
  int32 maxFeature = 0;
  for(int32 i = 0; i < faceLabelsStore.getSize(); i++)
  {
    if(faceLabelsStore[i] > maxFeature)
    {
      maxFeature = faceLabelsStore[i];
    }
  }

  // TODO:
  //  - Revisit reversal system to see if its worth going cluster by cluster rather than overall
  //  - Assess viability of implementing an internal voting system within the cluster
  // Define a voting system for full reversal
  std::vector<bool> reversalVote(maxFeature + 1, false);

  // Walk the features repairing the graph group by group
  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    // Find baseline/seed node (correct winding)
    Result<usize> seedResult = ::FindValidSeed(feature, triangles, verts, faceLabelsStore, reversalVote);
    if(seedResult.invalid())
    {
      return ConvertResult(std::move(seedResult));
    }


  }

  // Define a capturing lambda to execute filter without passing member variables to free functions
  const std::function<Result<>(const DataPath&)> f_ExecuteReverseTriangleWinding = [this](const DataPath& triGeomPath) -> Result<> {
    const ReverseTriangleWindingFilter filter;

    Arguments args;

    args.insertOrAssign(ReverseTriangleWindingFilter::k_TriGeomPath_Key, std::make_any<DataPath>(triGeomPath));

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

    return {};
  };

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
