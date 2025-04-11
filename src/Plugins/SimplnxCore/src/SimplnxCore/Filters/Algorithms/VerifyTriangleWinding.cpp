#include "VerifyTriangleWinding.hpp"

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
Result<> ImplementReversal(const std::vector<IGeometry::SharedVertexList::value_type>& volumes, const DataPath& geomPath, const std::function<Result<>(const DataPath&)>& reversalFunction)
{
  usize count = 0;
  for(auto volume : volumes)
  {
    if(std::signbit(volume))
    {
      // is negative
      count++;
    }
  }

  if(count >= (volumes.size() / 2))
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

  // Sort Vertices For Merging
  Result<> result = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath).validate();
  if(result.invalid())
  {
    return result;
  }

  m_MessageHandler("Checking for duplicates - sorting vertices");
  const MeshingUtilities::SortedVerticesList sortedVerticesList = MeshingUtilities::OrderSharedVertices(m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath), m_ShouldCancel);

  m_MessageHandler("Checking for duplicates - validating");
  auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
  if(MeshingUtilities::HasDuplicateVertices(triGeom.getVertices()->getDataStoreRef(), sortedVerticesList))
  {
    // Remove duplicates
    return MakeErrorResult(-56320, "Duplicate vertices found in mesh, please use cleanup filters to flag and remove these before rerunning.");
  }

  TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();

  // Load double-sided mesh grouping
  const Int32AbstractDataStore& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsPath)->getDataStoreRef();

  m_MessageHandler("Repairing Windings...");
  // This is reused since it may contain warnings
  Result<> windingResult = MeshingUtilities::RepairTriangleWinding(triangles, faceLabelsStore, m_ShouldCancel);
  if(windingResult.invalid())
  {
    return windingResult;
  }

  m_MessageHandler("Voting on reversal - calculating feature volumes");
  // Get max group (feature id != 0)
  int32 maxFeature = 0;
  for(int32 i = 0; i < faceLabelsStore.getSize(); i++)
  {
    if(faceLabelsStore[i] > maxFeature)
    {
      maxFeature = faceLabelsStore[i];
    }
  }
  std::vector<IGeometry::SharedVertexList::value_type> volumes(maxFeature);
  auto volumeResult = MeshingUtilities::CalculateFeatureVolumes(triangles, triGeom.getVertices()->getDataStoreRef(), faceLabelsStore, volumes, m_ShouldCancel);
  if(volumeResult.invalid())
  {
    return volumeResult;
  }

  m_MessageHandler("Voting on reversal - determining validity of reversal");
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

  // Do reversal voting based on feature volume calculation
  Result<> reversalResult = ::ImplementReversal(volumes, m_InputValues->TargetGeometryPath, f_ExecuteReverseTriangleWinding);
  if(reversalResult.invalid())
  {
    return reversalResult;
  }

  if(m_InputValues->RepairNormals)
  {
    m_MessageHandler("Recalculating normals");
    auto& normals = m_DataStructure.getDataAs<Float64Array>(m_InputValues->TriangleNormalsPath)->getDataStoreRef();

    // Parallel algorithm to calculate normals
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0ULL, static_cast<usize>(triGeom.getNumberOfFaces()));
    dataAlg.execute(MeshingUtilities::CalculateNormalsImpl(triangles, triGeom.getVertices()->getDataStoreRef(), normals, m_ShouldCancel));
  }

  return windingResult;
}
