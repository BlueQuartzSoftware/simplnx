/**
 * @file SurfaceNets.cpp
 * @brief Implements storage dispatch for Surface Nets.
 */

#include "SurfaceNets.hpp"
#include "SurfaceNetsDirect.hpp"
#include "SurfaceNetsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <vector>

using namespace nx::core;

SurfaceNets::SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

SurfaceNets::~SurfaceNets() noexcept = default;

const std::atomic_bool& SurfaceNets::getCancel()
{
  return m_ShouldCancel;
}

Result<> SurfaceNets::operator()()
{
  std::vector<const IArray*> dispatchTargets;
  const auto appendArray = [&dispatchTargets](const IArray* array) {
    if(array != nullptr)
    {
      dispatchTargets.push_back(array);
    }
  };

  appendArray(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath));
  for(const auto& path : m_InputValues->SelectedCellDataArrayPaths)
  {
    appendArray(m_DataStructure.getDataAs<IDataArray>(path));
  }
  for(const auto& path : m_InputValues->SelectedFeatureDataArrayPaths)
  {
    appendArray(m_DataStructure.getDataAs<IDataArray>(path));
  }
  for(const auto& path : m_InputValues->CreatedDataArrayPaths)
  {
    appendArray(m_DataStructure.getDataAs<IDataArray>(path));
  }
  appendArray(m_DataStructure.getDataAs<IDataArray>(m_InputValues->NodeTypesDataPath));
  appendArray(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FaceLabelsDataPath));

  if(const auto* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath); triangleGeom != nullptr)
  {
    appendArray(triangleGeom->getVertices());
    appendArray(triangleGeom->getFaces());
  }
  return DispatchAlgorithm<SurfaceNetsDirect, SurfaceNetsScanline>(AlgorithmArrayTargets(std::move(dispatchTargets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
