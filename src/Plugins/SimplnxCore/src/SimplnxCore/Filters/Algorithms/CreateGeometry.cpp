#include "CreateGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
constexpr uint64 k_ImageGeometry = 0;
constexpr uint64 k_RectGridGeometry = 1;
constexpr uint64 k_VertexGeometry = 2;
constexpr uint64 k_EdgeGeometry = 3;
constexpr uint64 k_TriangleGeometry = 4;
constexpr uint64 k_QuadGeometry = 5;
constexpr uint64 k_TetGeometry = 6;
constexpr uint64 k_HexGeometry = 7;

Result<> checkGeometryArraysCompatible(const Float32AbstractDataStore& vertices, const UInt64AbstractDataStore& cells, bool treatWarningsAsErrors, const std::string& cellType)
{
  Result<> warningResults;
  usize numVertices = vertices.getNumberOfTuples();
  uint64 idx = 0;
  for(usize i = 0; i < cells.getSize(); i++)
  {
    idx = std::max(cells[i], idx);
  }
  if((idx + 1) > numVertices)
  {
    std::string msg =
        fmt::format("Supplied {} list contains a vertex index larger than the total length of the supplied shared vertex list\nIndex Value: {}\nNumber of Vertices: {}", cellType, idx, numVertices);
    if(treatWarningsAsErrors)
    {
      return MakeErrorResult(-8340, msg);
    }
    warningResults.warnings().push_back(Warning{-9841, msg});
  }
  return warningResults;
}

Result<> checkGridBoundsResolution(const Float32AbstractDataStore& bounds, bool treatWarningsAsErrors, const std::string& boundType)
{
  Result<> warningResults;
  float32 val = bounds[0];
  for(usize i = 1; i < bounds.getNumberOfTuples(); i++)
  {
    if(val > bounds[i])
    {
      std::string msg =
          fmt::format("Supplied {} Bounds array is not strictly increasing; this results in negative resolutions\nIndex {} Value: {}\nIndex {} Value: {}", boundType, (i - 1), val, i, bounds[i]);
      if(treatWarningsAsErrors)
      {
        return MakeErrorResult(-8342, msg);
      }
      warningResults.warnings().push_back(Warning{-8343, msg});
    }
    val = bounds[i];
  }
  return warningResults;
}
} // namespace

// -----------------------------------------------------------------------------
CreateGeometry::CreateGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateGeometry::~CreateGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateGeometry::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Starting CreateGeometry...");

  auto geometryPath = m_InputValues->OutputGeometryPath;
  auto geometryType = m_InputValues->GeometryTypeIndex;
  auto treatWarningsAsErrors = m_InputValues->WarningsAsErrors;

  auto* iGeometry = m_DataStructure.getDataAs<IGeometry>(geometryPath);
  auto lengthUnit = static_cast<IGeometry::LengthUnit>(m_InputValues->LengthUnitIndex);
  iGeometry->setUnits(lengthUnit);

  DataPath sharedVertexListArrayPath;
  DataPath sharedFaceListArrayPath;
  DataPath sharedCellListArrayPath;

  if(geometryType == k_VertexGeometry || geometryType == k_EdgeGeometry || geometryType == k_TriangleGeometry || geometryType == k_QuadGeometry || geometryType == k_TetGeometry || geometryType == 7)
  {
    sharedVertexListArrayPath = m_InputValues->VertexListPath;
  }
  if(geometryType == k_TriangleGeometry)
  {
    sharedFaceListArrayPath = m_InputValues->TriangleListPath;
  }
  if(geometryType == k_QuadGeometry)
  {
    sharedFaceListArrayPath = m_InputValues->QuadrilateralListPath;
  }
  if(geometryType == k_TetGeometry)
  {
    sharedCellListArrayPath = m_InputValues->TetrahedralListPath;
  }
  if(geometryType == k_HexGeometry)
  {
    sharedCellListArrayPath = m_InputValues->HexahedralListPath;
  }

  Result<> warningResults;

  // These checks must be done in execute since we are accessing the array values!
  if(geometryType == k_EdgeGeometry)
  {
    auto sharedEdgeListArrayPath = m_InputValues->EdgeListPath;
    const DataPath destEdgeListPath = geometryPath.createChildPath(sharedEdgeListArrayPath.getTargetName());
    const auto& edgesList = m_DataStructure.getDataAs<UInt64Array>(destEdgeListPath)->getDataStoreRef();
    const auto& vertexList = m_DataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(sharedVertexListArrayPath.getTargetName()))->getDataStoreRef();
    auto results = checkGeometryArraysCompatible(vertexList, edgesList, treatWarningsAsErrors, "edge");
    if(results.invalid())
    {
      return results;
    }
    warningResults.warnings().insert(warningResults.warnings().end(), results.warnings().begin(), results.warnings().end());
  }
  if(geometryType == k_TriangleGeometry || geometryType == k_QuadGeometry)
  {
    const DataPath destFaceListPath = geometryPath.createChildPath(sharedFaceListArrayPath.getTargetName());
    const auto& faceList = m_DataStructure.getDataAs<UInt64Array>(destFaceListPath)->getDataStoreRef();
    const auto& vertexList = m_DataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(sharedVertexListArrayPath.getTargetName()))->getDataStoreRef();
    auto results = checkGeometryArraysCompatible(vertexList, faceList, treatWarningsAsErrors, (geometryType == 4 ? "triangle" : "quadrilateral"));
    if(results.invalid())
    {
      return results;
    }
    warningResults.warnings().insert(warningResults.warnings().end(), results.warnings().begin(), results.warnings().end());
  }
  if(geometryType == k_TetGeometry || geometryType == k_HexGeometry)
  {
    const DataPath destCellListPath = geometryPath.createChildPath(sharedCellListArrayPath.getTargetName());
    const auto& cellList = m_DataStructure.getDataAs<UInt64Array>(destCellListPath)->getDataStoreRef();
    const auto& vertexList = m_DataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(sharedVertexListArrayPath.getTargetName()))->getDataStoreRef();
    auto results = checkGeometryArraysCompatible(vertexList, cellList, treatWarningsAsErrors, (geometryType == 6 ? "tetrahedral" : "hexahedral"));
    if(results.invalid())
    {
      return results;
    }
    warningResults.warnings().insert(warningResults.warnings().end(), results.warnings().begin(), results.warnings().end());
  }
  if(geometryType == k_RectGridGeometry)
  {
    auto xBoundsArrayPath = m_InputValues->XBoundsPath;
    auto yBoundsArrayPath = m_InputValues->YBoundsPath;
    auto zBoundsArrayPath = m_InputValues->ZBoundsPath;
    const auto& srcXBounds = m_DataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(xBoundsArrayPath.getTargetName()))->getDataStoreRef();
    const auto& srcYBounds = m_DataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(yBoundsArrayPath.getTargetName()))->getDataStoreRef();
    const auto& srcZBounds = m_DataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(zBoundsArrayPath.getTargetName()))->getDataStoreRef();
    auto xResults = checkGridBoundsResolution(srcXBounds, treatWarningsAsErrors, "X");
    auto yResults = checkGridBoundsResolution(srcYBounds, treatWarningsAsErrors, "Y");
    auto zResults = checkGridBoundsResolution(srcZBounds, treatWarningsAsErrors, "Z");
    auto results = MergeResults(MergeResults(xResults, yResults), zResults);
    if(results.invalid())
    {
      return results;
    }
    warningResults.warnings().insert(warningResults.warnings().end(), results.warnings().begin(), results.warnings().end());
  }
  return warningResults;
}
