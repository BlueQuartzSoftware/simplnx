#include "TriangleNormal.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
TriangleNormal::TriangleNormal(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleNormalInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TriangleNormal::~TriangleNormal() noexcept = default;

// -----------------------------------------------------------------------------
Result<> TriangleNormal::operator()()
{
  auto pTriangleGeometryDataPath = m_InputValues->InputTriangleGeometryPath;
  auto pNormalsName = m_InputValues->OutputNormalsArrayName;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom.getFaceAttributeMatrix();

  DataPath pNormalsArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pNormalsName);
  auto& normalsRef = m_DataStructure.getDataAs<Float64Array>(pNormalsArrayPath)->getDataStoreRef();

  return MeshingUtilities::CalculateNormals(*triangleGeom.getFaces(), *triangleGeom.getVertices(), normalsRef, m_ShouldCancel);
}
