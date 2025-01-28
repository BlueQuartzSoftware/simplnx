#include "CombineNodeBasedGeometries.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/ConcatenateDataArrays.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
template <typename NodeGeomType, typename GetArrayFunc>
usize CalculateTotalTuples(std::vector<NodeGeomType*>& geomPtrs, GetArrayFunc getArray)
{
  usize tupleCount = 0;
  for(const auto& geomPtr : geomPtrs)
  {
    auto* array = getArray(geomPtr);
    tupleCount += array->getNumberOfTuples();
  }
  return tupleCount;
}

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
std::vector<std::vector<DataPath>> RetrieveGeometryElementPaths(std::vector<NodeGeomType*>& geomPtrs, GetArrayFunc getArray, GetAttrMatrixFunc getAttrMatrix)
{
  std::vector<std::vector<DataPath>> finalPaths = {std::vector<DataPath>(geomPtrs.size())};
  for(usize geomIdx = 0; geomIdx < geomPtrs.size(); ++geomIdx)
  {
    auto* geomPtr = geomPtrs[geomIdx];

    // Retrieve the data array paths (edge arrays, face arrays, polyhedra arrays, etc.)
    AttributeMatrix* attrMatrix = getAttrMatrix(geomPtr);
    if(attrMatrix != nullptr)
    {
      // Resize all the vectors
      if(attrMatrix->getSize() > 0)
      {
        finalPaths.resize(attrMatrix->getSize() + 1);
        for(auto& paths : finalPaths)
        {
          paths.resize(geomPtrs.size());
        }
      }

      usize attrMatrixItemIdx = 1;
      for(const auto& pair : *attrMatrix)
      {
        finalPaths[attrMatrixItemIdx++][geomIdx] = pair.second->getDataPaths()[0];
      }
    }

    // Retrieve the array path (edges, faces, polyhedra, etc.)
    auto* array = getArray(geomPtr);
    finalPaths[0][geomIdx] = array->getDataPaths()[0];
  }

  return finalPaths;
}

template <typename NodeGeomType, typename GetVerticesArrayFunc, typename GetCellArrayFunc>
void UpdateCellArrayIndices(DataStructure& ds, NodeGeomType* outputGeomPtr, std::vector<NodeGeomType*>& inputGeoms, GetVerticesArrayFunc getVerticesArray, GetCellArrayFunc getCellArray,
                            const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  // Update the cell array indices to correctly map to the expanded vertex array
  uint64 verticesOffset = 0;
  usize cellsOffset = 0;

  auto outputCellArray = getCellArray(outputGeomPtr);
  for(const auto& inputGeom : inputGeoms)
  {
    if(shouldCancel)
    {
      return;
    }

    auto inputVertexArray = getVerticesArray(inputGeom);
    auto inputCellArray = getCellArray(inputGeom);

    // Increase each value in the cell array by the verticesOffset
    // The verticesOffset is simply the total number of vertices for all the preceding geometries
    // The cellsOffset is calculated to make sure we are updating the values at the correct location in the output cell array
    std::transform(outputCellArray->begin() + cellsOffset, outputCellArray->begin() + cellsOffset + inputCellArray->getSize(), outputCellArray->begin() + cellsOffset,
                   [verticesOffset](uint64 value) -> uint64 { return value + verticesOffset; });
    verticesOffset += inputVertexArray->getNumberOfTuples();
    cellsOffset += inputCellArray->getSize();
  }
}

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
Result<> CombineGeometryElements(DataStructure& ds, NodeGeomType* outputGeomPtr, std::vector<NodeGeomType*>& inputGeoms, GetArrayFunc getArray, GetAttrMatrixFunc getAttrMatrix,
                                 const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  std::vector<std::vector<DataPath>> inputArrayPaths = RetrieveGeometryElementPaths(inputGeoms, getArray, getAttrMatrix);
  auto outputGeomPtrs = std::vector<NodeGeomType*>{outputGeomPtr};
  std::vector<std::vector<DataPath>> outputArrayPaths = RetrieveGeometryElementPaths(outputGeomPtrs, getArray, getAttrMatrix);

  usize totalTuples = CalculateTotalTuples(inputGeoms, getArray);
  auto* array = getArray(outputGeomPtr);
  array->resizeTuples({totalTuples});
  auto* attrMatrix = getAttrMatrix(outputGeomPtr);
  if(attrMatrix != nullptr)
  {
    attrMatrix->resizeTuples({totalTuples});
  }

  for(usize i = 0; i < inputArrayPaths.size(); ++i)
  {
    if(shouldCancel)
    {
      return {};
    }

    const auto& paths = inputArrayPaths[i];

    ConcatenateDataArraysInputValues inputValues;
    inputValues.InputArrayPaths = paths;
    inputValues.OutputArrayPath = outputArrayPaths[i][0];
    auto result = ConcatenateDataArrays(ds, msgHandler, shouldCancel, &inputValues)();
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> CombineVertexElements(DataStructure& ds, IGeometry* outputGeom, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                               const std::atomic_bool& shouldCancel)
{
  auto getVerticesArrayFunc = [](INodeGeometry0D* ptr) -> auto { return ptr->getVertices(); };
  auto getVertexAttrMatrixFunc = [](INodeGeometry0D* ptr) -> auto { return ptr->getVertexAttributeMatrix(); };

  auto* outputGeom0d = dynamic_cast<INodeGeometry0D*>(outputGeom);
  if(outputGeom0d == nullptr)
  {
    // This is not a 0D geometry, so just return
    return {};
  }

  auto* outputVertexArray = getVerticesArrayFunc(outputGeom0d);
  if(outputVertexArray == nullptr)
  {
    // There are no vertices, this is an error
    return MakeErrorResult(to_underlying(CombineNodeBasedGeometries::ErrorCodes::NodeGeometryHasNoVertices),
                           fmt::format("The chosen node geometries do not have a shared vertex array.  All node geometries MUST have a shared vertex array."));
  }

  msgHandler({IFilter::Message::Type::Info, fmt::format("Combining vertex data...")});
  std::vector<INodeGeometry0D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry0D>(path); });
  return CombineGeometryElements<INodeGeometry0D>(ds, outputGeom0d, inputGeoms, getVerticesArrayFunc, getVertexAttrMatrixFunc, msgHandler, shouldCancel);
}

Result<> CombineEdgeElements(DataStructure& ds, IGeometry* outputGeom, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  auto getEdgesArrayFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getEdges(); };
  auto getEdgeAttrMatrixFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getEdgeAttributeMatrix(); };

  auto* outputGeom1d = dynamic_cast<INodeGeometry1D*>(outputGeom);
  if(outputGeom1d == nullptr)
  {
    return {};
  }

  auto* outputEdgesArray = getEdgesArrayFunc(outputGeom1d);
  if(outputEdgesArray == nullptr)
  {
    // There are no edges, so just return
    return {};
  }

  std::vector<INodeGeometry1D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry1D>(path); });

  msgHandler({IFilter::Message::Type::Info, fmt::format("Combining edge data...")});
  auto result = CombineGeometryElements<INodeGeometry1D>(ds, outputGeom1d, inputGeoms, getEdgesArrayFunc, getEdgeAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  msgHandler({IFilter::Message::Type::Info, fmt::format("Mapping edge values to updated vertex indices...")});
  auto getVerticesArrayFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(ds, outputGeom1d, inputGeoms, getVerticesArrayFunc, getEdgesArrayFunc, msgHandler, shouldCancel);
  return {};
}

Result<> CombineFaceElements(DataStructure& ds, IGeometry* outputGeom, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  auto getFacesArrayFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getFaces(); };
  auto getFaceAttrMatrixFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getFaceAttributeMatrix(); };

  auto* outputGeom2d = dynamic_cast<INodeGeometry2D*>(outputGeom);
  if(outputGeom2d == nullptr)
  {
    return {};
  }

  auto* outputFacesArray = getFacesArrayFunc(outputGeom2d);
  if(outputFacesArray == nullptr)
  {
    // There are no faces, so just return
    return {};
  }

  msgHandler({IFilter::Message::Type::Info, fmt::format("Combining face data...")});
  std::vector<INodeGeometry2D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry2D>(path); });
  auto result = CombineGeometryElements<INodeGeometry2D>(ds, outputGeom2d, inputGeoms, getFacesArrayFunc, getFaceAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  msgHandler({IFilter::Message::Type::Info, fmt::format("Mapping face values to updated vertex indices...")});
  auto getVerticesArrayFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(ds, outputGeom2d, inputGeoms, getVerticesArrayFunc, getFacesArrayFunc, msgHandler, shouldCancel);
  return {};
}

Result<> CombinePolyElements(DataStructure& ds, IGeometry* outputGeom, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  auto getPolyArrayFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedra(); };
  auto getPolyAttrMatrixFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedraAttributeMatrix(); };

  auto* outputGeom3d = dynamic_cast<INodeGeometry3D*>(outputGeom);
  if(outputGeom3d == nullptr)
  {
    return {};
  }

  auto* outputPolyhedraArray = getPolyArrayFunc(outputGeom3d);
  if(outputPolyhedraArray == nullptr)
  {
    // There are no polyhedra, so just return
    return {};
  }

  msgHandler({IFilter::Message::Type::Info, fmt::format("Combining polyhedron data...")});
  std::vector<INodeGeometry3D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry3D>(path); });
  auto result = CombineGeometryElements<INodeGeometry3D>(ds, outputGeom3d, inputGeoms, getPolyArrayFunc, getPolyAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  msgHandler({IFilter::Message::Type::Info, fmt::format("Mapping polyhedron values to updated vertex indices...")});
  auto getVerticesArrayFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(ds, outputGeom3d, inputGeoms, getVerticesArrayFunc, getPolyArrayFunc, msgHandler, shouldCancel);
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
CombineNodeBasedGeometries::CombineNodeBasedGeometries(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                       CombineNodeBasedGeometriesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
CombineNodeBasedGeometries::~CombineNodeBasedGeometries() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineNodeBasedGeometries::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void CombineNodeBasedGeometries::sendMessage(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
Result<> CombineNodeBasedGeometries::operator()()
{
  auto* outputGeom = m_DataStructure.getDataAs<IGeometry>(m_InputValues->OutputGeometryPath);

  CombineVertexElements(m_DataStructure, outputGeom, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  CombineEdgeElements(m_DataStructure, outputGeom, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  CombineFaceElements(m_DataStructure, outputGeom, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  CombinePolyElements(m_DataStructure, outputGeom, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);

  return {};
}
