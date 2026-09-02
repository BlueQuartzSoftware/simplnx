#include "VerifyTriangleWinding.hpp"

#include <algorithm>

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
class ReverseWindingImpl
{
public:
  using TriStore = AbstractDataStore<TriangleGeom::SharedFaceList::value_type>;
  ReverseWindingImpl(TriStore& triangles, const Int32AbstractDataStore& idsStore, const std::vector<usize>& reversalTargets, const std::atomic_bool& shouldCancel)
  : m_Triangles(triangles)
  , m_IdsStore(idsStore)
  , m_ReversalTargets(reversalTargets)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~ReverseWindingImpl() = default;

  void generateWithLabels(usize start, usize end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 feature1 = m_IdsStore[(i * 2) + 0];
      const int32 feature2 = m_IdsStore[(i * 2) + 1];
      for(const usize target : m_ReversalTargets)
      {
        if(target != feature1 && target != feature2)
        {
          continue;
        }

        // Flip it
        const IGeometry::MeshIndexType tempValue = m_Triangles[(i * 3) + 0];
        m_Triangles[(i * 3) + 0] = m_Triangles[(i * 3) + 2];
        m_Triangles[(i * 3) + 2] = tempValue;
        break;
      }
    }
  }

  void generateWithRegions(usize start, usize end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 feature = m_IdsStore[i];
      for(const usize target : m_ReversalTargets)
      {
        if(target != feature)
        {
          continue;
        }

        // Flip it
        const IGeometry::MeshIndexType tempValue = m_Triangles[(i * 3) + 0];
        m_Triangles[(i * 3) + 0] = m_Triangles[(i * 3) + 2];
        m_Triangles[(i * 3) + 2] = tempValue;
        break;
      }
    }
  }

  void operator()(const Range& range) const
  {
    if(m_IdsStore.getNumberOfComponents() == 2)
    {
      generateWithLabels(range.min(), range.max());
    }
    else if(m_IdsStore.getNumberOfComponents() == 1)
    {
      generateWithRegions(range.min(), range.max());
    }
  }

private:
  TriStore& m_Triangles;
  const Int32AbstractDataStore& m_IdsStore;
  const std::vector<usize>& m_ReversalTargets;
  const std::atomic_bool& m_ShouldCancel;
};
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

  // Sort Vertices For Merging
  Result<> result = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath).validate();
  if(result.invalid())
  {
    return result;
  }

  m_MessageHandler.sendInfoMessage("Checking for duplicates - sorting vertices");
  const MeshingUtilities::SortedVerticesList sortedVerticesList = MeshingUtilities::OrderSharedVertices(m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath), m_ShouldCancel);

  m_MessageHandler.sendInfoMessage("Checking for duplicates - validating");
  auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
  if(MeshingUtilities::HasDuplicateVertices(triGeom.getVertices()->getDataStoreRef(), sortedVerticesList))
  {
    // Remove duplicates
    return MakeErrorResult(-56320, "Duplicate vertices found in mesh, please use cleanup filters to flag and remove these before rerunning.");
  }

  TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();

  // Load mesh grouping (face labels or regions)
  const Int32AbstractDataStore& idsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->LabelsPath)->getDataStoreRef();

  // Scoped because we invalidate connectivity at the end
  Result<> windingResult = {};
  {
    // Generate Connectivity
    m_MessageHandler.sendInfoMessage("Generating Connectivity and Triangle Neighbors...");
    triGeom.findElementNeighbors(true);
    const auto optionalId = triGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56321, fmt::format("Unable to generate the connectivity list for {} geometry.", triGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler.sendInfoMessage("Repairing Windings...");
    // This is reused since it may contain warnings
    windingResult = MeshingUtilities::RepairTriangleWinding(triangles, connectivity, idsStore, m_ShouldCancel, m_MessageHandler);
    if(windingResult.invalid())
    {
      return windingResult;
    }

    // Purge connectivity
    m_DataStructure.removeData(triGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triGeom.getElementNeighborsId().value());
  }

  m_MessageHandler.sendInfoMessage("Assessing reversal - calculating feature volumes");
  // Get max group (feature id != 0)
  int32 maxFeature = 0;
  for(int32 i = 0; i < idsStore.getSize(); i++)
  {
    maxFeature = std::max(idsStore[i], maxFeature);
  }

  std::vector<IGeometry::SharedVertexList::value_type> volumes(maxFeature + 1);
  auto volumeResult = MeshingUtilities::CalculateFeatureVolumes(triangles, triGeom.getVertices()->getDataStoreRef(), idsStore, volumes, m_ShouldCancel);
  if(volumeResult.invalid())
  {
    return volumeResult;
  }

  m_MessageHandler.sendInfoMessage("Implementing reversal in parallel");
  std::vector<usize> reversalTargets = {};
  reversalTargets.reserve(volumes.size());
  for(usize i = 1; i < volumes.size(); i++) // zero is an invalid feature label
  {
    if(volumes[i] < 0.0f)
    {
      reversalTargets.push_back(i);
    }
  }
  reversalTargets.shrink_to_fit();

  if(!reversalTargets.empty())
  {
    // Parallel algorithm to reverse based on feature label
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0ULL, static_cast<usize>(triangles.getNumberOfTuples()));
    dataAlg.execute(::ReverseWindingImpl(triangles, idsStore, reversalTargets, m_ShouldCancel));
  }

  if(m_InputValues->RepairNormals)
  {
    m_MessageHandler.sendInfoMessage("Recalculating normals");
    auto& normals = m_DataStructure.getDataAs<Float64Array>(m_InputValues->TriangleNormalsPath)->getDataStoreRef();

    // Parallel algorithm to calculate normals
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0ULL, static_cast<usize>(triGeom.getNumberOfFaces()));
    dataAlg.execute(MeshingUtilities::CalculateNormalsImpl(triangles, triGeom.getVertices()->getDataStoreRef(), normals, m_ShouldCancel));
  }

  return windingResult;
}
