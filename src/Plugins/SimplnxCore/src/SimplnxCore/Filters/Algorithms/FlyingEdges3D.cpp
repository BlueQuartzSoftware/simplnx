#include "FlyingEdges3D.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/FlyingEdges.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidInputArrayError = -10512;
constexpr int32 k_ComponentShapeError = -10511;
constexpr int32 k_CellTupleCountError = -10510;
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
    if(Result<> result = flyingEdges.pass3(); result.invalid())
    {
      return result;
    }

    // Pass 3 resizes normals. Keep the parent AttributeMatrix consistent.
    if(Result<> result = normAM.resizeTuples(normals.getTupleShape()); result.invalid())
    {
      return result;
    }

    if(Result<> result = flyingEdges.pass4(); result.invalid())
    {
      return result;
    }
    if(Result<> result = triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleGeom.getNumberOfFaces()}); result.invalid())
    {
      return result;
    }
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
Result<> FlyingEdges3D::ValidateInputs(const DataStructure& dataStructure, const FlyingEdges3DInputValues& inputValues)
{
  const auto* array = dataStructure.getDataAs<IDataArray>(inputValues.contouringArrayPath);
  if(array == nullptr)
  {
    return MakeErrorResult(k_InvalidInputArrayError,
                           fmt::format("Input path '{}' is missing or is not a numeric data array. Select an existing numeric data array.", inputValues.contouringArrayPath.toString()));
  }
  if(array->getComponentShape() != ShapeType{1})
  {
    return MakeErrorResult(k_ComponentShapeError, fmt::format("Input array '{}' has component shape ({}), but requires {{1}}. Select an array with exactly one component dimension of size 1.",
                                                              inputValues.contouringArrayPath.toString(), StringUtilities::formatDimensions(array->getComponentShape())));
  }
  return ValidateCellTupleCount(dataStructure, inputValues.imageGeomPath, {inputValues.contouringArrayPath}, k_CellTupleCountError);
}

// -----------------------------------------------------------------------------
Result<> FlyingEdges3D::operator()()
{
  if(auto validation = ValidateInputs(m_DataStructure, *m_InputValues); validation.invalid())
  {
    return validation;
  }
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
