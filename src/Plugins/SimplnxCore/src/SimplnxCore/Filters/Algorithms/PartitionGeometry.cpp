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

PartitionGeometry::~PartitionGeometry() noexcept = default;

Result<> PartitionGeometry::operator()()
{
  const DataPath partitionIdsPath = m_InputValues->InputGeomCellAMPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  const auto* partitionIdsArray = m_DataStructure.getDataAs<Int32Array>(partitionIdsPath);

  const IDataArray* partitionGridFeatureIdsArray = nullptr;
  const auto partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->PartitioningMode);
  if(partitioningMode != PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid)
  {
    const DataPath partitionGridFeatureIdsPath =
        m_InputValues->PartitionGridGeomPath.createChildPath(m_InputValues->PartitionGridCellAMName).createChildPath(m_InputValues->PartitionGridFeatureIDsArrayName);
    partitionGridFeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(partitionGridFeatureIdsPath);
  }

  const IDataArray* verticesArray = nullptr;
  const IGeometry& geometry = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->InputGeometryToPartition);
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const auto& nodeGeom = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->InputGeometryToPartition);
    verticesArray = nodeGeom.getVertices();
    break;
  }
  default:
    break;
  }

  const IDataArray* maskArray = m_InputValues->UseVertexMask ? m_DataStructure.getDataAs<BoolArray>(m_InputValues->VertexMaskPath) : nullptr;
  return DispatchAlgorithm<PartitionGeometryDirect, PartitionGeometryScanline>({partitionIdsArray, partitionGridFeatureIdsArray, verticesArray, maskArray}, m_DataStructure, m_MessageHandler,
                                                                               m_ShouldCancel, m_InputValues);
}
