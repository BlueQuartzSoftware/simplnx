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

QuickSurfaceMesh::~QuickSurfaceMesh() noexcept = default;

Result<> QuickSurfaceMesh::operator()()
{
  std::vector<const IArray*> targets;
  const auto append = [this, &targets](const DataPath& path) {
    if(const auto* array = m_DataStructure.getDataAs<IDataArray>(path); array != nullptr)
    {
      targets.push_back(array);
    }
  };

  append(m_InputValues->FeatureIdsArrayPath);
  append(m_InputValues->NodeTypesDataPath);
  append(m_InputValues->FaceLabelsDataPath);
  for(const auto& path : m_InputValues->SelectedCellDataArrayPaths)
  {
    append(path);
  }
  for(const auto& path : m_InputValues->SelectedFeatureDataArrayPaths)
  {
    append(path);
  }
  for(const auto& path : m_InputValues->CreatedDataArrayPaths)
  {
    append(path);
  }

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  targets.push_back(triangleGeom.getVertices());
  targets.push_back(triangleGeom.getFaces());
  return DispatchAlgorithm<QuickSurfaceMeshDirect, QuickSurfaceMeshScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
