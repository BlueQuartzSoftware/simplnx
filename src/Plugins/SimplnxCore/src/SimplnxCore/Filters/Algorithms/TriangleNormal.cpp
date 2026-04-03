#include "TriangleNormal.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

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

  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage(fmt::format("Computing Triangle Normals for {} triangles...", triangleGeom.getNumberOfFaces()));

  // Parallel algorithm to calculate normals
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(MeshingUtilities::CalculateNormalsImpl(triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getVertices()->getDataStoreRef(), normalsRef, m_ShouldCancel));

  return {};
}
