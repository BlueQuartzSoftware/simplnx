/**
 * @file QuickSurfaceMesh.cpp
 * @brief Implements storage-aware QuickSurfaceMesh dispatch.
 *
 * The dispatcher selects QuickSurfaceMeshScanline when any source or output target
 * uses out-of-core storage. The scanline path uses bounded bulk I/O.
 */

#include "QuickSurfaceMesh.hpp"
#include "QuickSurfaceMeshDirect.hpp"
#include "QuickSurfaceMeshScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

QuickSurfaceMesh::QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> QuickSurfaceMesh::operator()()
{
  std::vector<const IArray*> targets;
  const auto append = [this, &targets](const DataPath& path) {
    if(const auto* arrayPtr = m_DataStructure.getDataAs<IDataArray>(path); arrayPtr != nullptr)
    {
      targets.push_back(arrayPtr);
    }
  };

  append(m_InputValues->feature_ids_array_path);
  append(m_InputValues->node_types_data_path);
  append(m_InputValues->face_labels_data_path);
  for(const auto& path : m_InputValues->selected_cell_data_array_paths)
  {
    append(path);
  }
  for(const auto& path : m_InputValues->selected_feature_data_array_paths)
  {
    append(path);
  }
  for(const auto& path : m_InputValues->created_data_array_paths)
  {
    append(path);
  }

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->triangle_geometry_path);
  targets.push_back(triangleGeom.getVertices());
  targets.push_back(triangleGeom.getFaces());
  return DispatchAlgorithm<QuickSurfaceMeshDirect, QuickSurfaceMeshScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
