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

// -----------------------------------------------------------------------------
Result<> TriangleNormal::operator()()
{
  auto pTriangleGeometryDataPath = m_InputValues->input_triangle_geometry_path;
  auto pNormalsName = m_InputValues->output_normals_array_name;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrixPtr = triangleGeom.getFaceAttributeMatrix();

  DataPath pNormalsArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrixPtr->getName()).createChildPath(pNormalsName);
  auto& normalsRef = m_DataStructure.getDataAs<Float64ArrayType>(pNormalsArrayPath)->getDataStoreRef();

  return meshing_utilities::CalculateNormals(*triangleGeom.getFaces(), *triangleGeom.getVertices(), normalsRef, m_ShouldCancel);
}
