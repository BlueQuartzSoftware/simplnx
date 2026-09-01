#include "FlyingEdges3D.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/FlyingEdges.hpp"

using namespace nx::core;

namespace
{
/**
 * @struct ExecuteFlyingEdgesFunctor
 * @brief Dispatches and validates the four Flying Edges passes.
 */
struct ExecuteFlyingEdgesFunctor
{
  /**
   * @brief Runs one scalar specialization.
   * @tparam T Input scalar type.
   * @param image Supplies dimensions and coordinates.
   * @param iDataArray Supplies scalar point values.
   * @param isoVal Specifies the contour value before conversion to T.
   * @param triangleGeom Receives surface points and faces.
   * @param normals Receives point normals.
   * @param normAM Owns the normals array.
   * @return Success, or a source bulk-read error from pass 1, 2, or 4.
   */
  template <typename T>
  Result<> operator()(const ImageGeom& image, const IDataArray* iDataArray, float64 isoVal, TriangleGeom& triangleGeom, Float32AbstractDataStore& normals, AttributeMatrix& normAM)
  {
    FlyingEdgesAlgorithm flyingEdges = FlyingEdgesAlgorithm<T>(image, iDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), static_cast<T>(isoVal), triangleGeom, normals);
    if(Result<> result = flyingEdges.pass1(); result.invalid())
    {
      return result;
    }
    if(Result<> result = flyingEdges.pass2(); result.invalid())
    {
      return result;
    }
    flyingEdges.pass3();

    // Pass 3 resizes normals. Keep the parent AttributeMatrix consistent.
    normAM.resizeTuples(normals.getTupleShape());

    if(Result<> result = flyingEdges.pass4(); result.invalid())
    {
      return result;
    }
    triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleGeom.getNumberOfFaces()});
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
FlyingEdges3D::FlyingEdges3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FlyingEdges3DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FlyingEdges3D::~FlyingEdges3D() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FlyingEdges3D::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FlyingEdges3D::operator()()
{
  const auto& image = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->imageGeomPath);
  float64 isoVal = m_InputValues->isoVal;
  const auto* iDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->contouringArrayPath);
  auto triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->triangleGeomPath);
  auto& normalsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->normalsArrayPath)->getDataStoreRef();

  // Preflight creates normals under an AttributeMatrix.
  DataPath normAMPath = m_InputValues->normalsArrayPath.getParent();

  auto& normAM = m_DataStructure.getDataRefAs<AttributeMatrix>(normAMPath);

  const bool usesOutOfCoreStore = IsOutOfCore(*iDataArray);
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  return ExecuteNeighborFunction(ExecuteFlyingEdgesFunctor{}, iDataArray->getDataType(), image, iDataArray, isoVal, triangleGeom, normalsStore, normAM);
}
