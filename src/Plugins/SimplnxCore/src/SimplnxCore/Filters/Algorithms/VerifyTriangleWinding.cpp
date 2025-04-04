#include "VerifyTriangleWinding.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"
#include "SimplnxCore/Filters/TriangleNormalFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
Result<> ImplementReversal(const std::vector<bool> reversalVotes, const DataPath& geomPath, const std::function<Result<>(const DataPath&)>& reversalFunction)
{
  if(std::count(reversalVotes.begin(), reversalVotes.end(), true) >= (reversalVotes.size() / 2))
  {
    return reversalFunction(geomPath);
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
  {
    // Sort Vertices For Merging
    Result<> result = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath).validate();
    if(result.invalid())
    {
      return result;
    }

    MeshingUtilities::SortedVerticesList sortedVerticesList = MeshingUtilities::OrderSharedVertices(m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath));
    auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
    if(MeshingUtilities::HasDuplicateVertices(triGeom.getVertices()->getDataStoreRef(), sortedVerticesList))
    {
      // Remove duplicates
      MeshingUtilities::RemoveDuplicateVertices(triGeom, sortedVerticesList);
    }
    else
    {
      // Sorting here to make ordering implicit rather than maintaining a mapping, feature parity with duplicate removal
      MeshingUtilities::SortGeomVertices(triGeom, sortedVerticesList);
    }
  }

  // Load container, node list, and node data respectively;
  auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);

  TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();

  // Load double-sided mesh grouping
  const Int32AbstractDataStore& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsPath)->getDataStoreRef();

  // This is reused since it may contain warnings
  Result<> windingResult = MeshingUtilities::RepairTriangleWinding(triangles, faceLabelsStore, m_ShouldCancel);
  if(windingResult.invalid())
  {
    return windingResult;
  }

  // TODO:
  //  - Revisit reversal system to see if its worth going cluster by cluster rather than overall
  //  - Assess viability of implementing an internal voting system within the cluster

  // Define a voting system for full reversal
  //std::vector<bool> reversalVote(maxFeature + 1);

  // TODO:
  //  Do reversal voting based on feature volume calculation

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

//  Result<> result = ::ImplementReversal(reversalVote, m_InputValues->TargetGeometryPath, f_ExecuteReverseTriangleWinding);
//  if(result.invalid())
//  {
//    return result;
//  }

  if(m_InputValues->RepairNormals)
  {
    auto& normals = m_DataStructure.getDataAs<Float64Array>(m_InputValues->TriangleNormalsPath)->getDataStoreRef();

    // Parallel algorithm to calculate normals
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0ULL, static_cast<usize>(triGeom.getNumberOfFaces()));
    dataAlg.execute(MeshingUtilities::CalculateNormalsImpl(triangles, triGeom.getVertices()->getDataStoreRef(), normals, m_ShouldCancel));
  }

  return windingResult;
}
