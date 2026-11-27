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

  appendArray(m_DataStructure.getDataAs<IDataArray>(m_InputValues->feature_ids_array_path));
  for(const auto& path : m_InputValues->selected_cell_data_array_paths)
  {
    appendArray(m_DataStructure.getDataAs<IDataArray>(path));
  }
  for(const auto& path : m_InputValues->selected_feature_data_array_paths)
  {
    appendArray(m_DataStructure.getDataAs<IDataArray>(path));
  }
  for(const auto& path : m_InputValues->created_data_array_paths)
  {
    appendArray(m_DataStructure.getDataAs<IDataArray>(path));
  }
  appendArray(m_DataStructure.getDataAs<IDataArray>(m_InputValues->node_types_data_path));
  appendArray(m_DataStructure.getDataAs<IDataArray>(m_InputValues->face_labels_data_path));

  if(const auto* triangleGeomPtr = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->triangle_geometry_path); triangleGeomPtr != nullptr)
  {
    appendArray(triangleGeomPtr->getVertices());
    appendArray(triangleGeomPtr->getFaces());
  }
  return DispatchAlgorithm<SurfaceNetsDirect, SurfaceNetsScanline>(AlgorithmArrayTargets(std::move(dispatchTargets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
