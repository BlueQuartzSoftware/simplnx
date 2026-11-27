#include "PartitionGeometry.hpp"

#include "PartitionGeometryDirect.hpp"
#include "PartitionGeometryScanline.hpp"

#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"

using namespace nx::core;

PartitionGeometry::PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> PartitionGeometry::operator()()
{
  const DataPath partitionIdsPath = m_InputValues->input_geom_cell_am_path.createChildPath(m_InputValues->partition_ids_array_name);
  const auto* partitionIdsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(partitionIdsPath);

  const IDataArray* partitionGridFeatureIdsArrayPtr = nullptr;
  const auto partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->partitioning_mode);
  if(partitioningMode != PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid)
  {
    const DataPath partitionGridFeatureIdsPath =
        m_InputValues->partition_grid_geom_path.createChildPath(m_InputValues->partition_grid_cell_am_name).createChildPath(m_InputValues->partition_grid_feature_i_ds_array_name);
    partitionGridFeatureIdsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(partitionGridFeatureIdsPath);
  }

  const IDataArray* verticesArrayPtr = nullptr;
  const IGeometry& geometry = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->input_geometry_to_partition);
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const auto& nodeGeom = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->input_geometry_to_partition);
    verticesArrayPtr = nodeGeom.getVertices();
    break;
  }
  default:
    break;
  }

  const IDataArray* maskArrayPtr = m_InputValues->use_vertex_mask ? m_DataStructure.getDataAs<BoolArrayType>(m_InputValues->vertex_mask_path) : nullptr;
  return DispatchAlgorithm<PartitionGeometryDirect, PartitionGeometryScanline>({partitionIdsArrayPtr, partitionGridFeatureIdsArrayPtr, verticesArrayPtr, maskArrayPtr}, m_DataStructure,
                                                                               m_MessageHandler, m_ShouldCancel, m_InputValues);
}
