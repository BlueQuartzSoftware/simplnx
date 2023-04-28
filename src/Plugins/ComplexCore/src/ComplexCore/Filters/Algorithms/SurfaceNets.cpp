#include "SurfaceNets.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "ComplexCore/SurfaceNets/MMSurfaceNet.h"

using namespace complex;

// -----------------------------------------------------------------------------
SurfaceNets::SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SurfaceNets::~SurfaceNets() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SurfaceNets::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SurfaceNets::operator()()
{
  // Get the ImageGeometry
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);

  // Get the Created Triangle Geometry
  TriangleGeom& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  IntVec3 arraySize(static_cast<int32>(gridDimensions[0]), static_cast<int32>(gridDimensions[1]), static_cast<int32>(gridDimensions[2]));

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdDataStore = featureIds.getIDataStoreRefAs<Int32DataStore>();
  MMSurfaceNet surfaceNet(featureIdDataStore.data(), arraySize.data(), voxelSize.data());

  // Use current parameters to relax the SurfaceNet
  if(m_InputValues->ApplySmoothing)
  {
    MMSurfaceNet::RelaxAttrs m_relaxAttrs;
    m_relaxAttrs.maxDistFromCellCenter = m_InputValues->MaxDistanceFromVoxel;
    m_relaxAttrs.numRelaxIterations = m_InputValues->SmoothingIterations;
    m_relaxAttrs.relaxFactor = m_InputValues->RelaxationFactor;

    surfaceNet.relax(m_relaxAttrs);
  }

  // Now Extract the Triangles from the SurfaceNets object.
  return {};
}
