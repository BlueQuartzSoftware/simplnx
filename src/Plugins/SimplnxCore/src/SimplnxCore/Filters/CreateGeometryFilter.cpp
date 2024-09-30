#include "CreateGeometryFilter.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry3DAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateRectGridGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataObjectUtilities.hpp"

#include <sstream>
#include <string>

using namespace std::string_literals;

using namespace nx::core;

namespace nx::core
{
namespace
{
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
      return {nonstd::make_unexpected(std::vector<Error>{Error{-8340, msg}})};
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

//------------------------------------------------------------------------------
std::string CreateGeometryFilter::name() const
{
  return FilterTraits<CreateGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateGeometryFilter::className() const
{
  return FilterTraits<CreateGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateGeometryFilter::uuid() const
{
  return FilterTraits<CreateGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateGeometryFilter::humanName() const
{
  return "Create Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Generation",
          "Geometry"
          "Create Geometry"};
}

//------------------------------------------------------------------------------
Parameters CreateGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GeometryType_Key, "Geometry Type", "The type of Geometry to create", 0, GetAllGeometryTypesAsStrings()));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnitType_Key, "Length Unit", "The length unit to be used in the geometry", to_underlying(IGeometry::LengthUnit::Millimeter),
                                                   IGeometry::GetAllLengthUnitStrings()));
  params.insert(std::make_unique<BoolParameter>(k_WarningsAsErrors_Key, "Treat Geometry Warnings as Errors", "Whether run time warnings for Geometries should be treated as errors", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ArrayHandling_Key, "Array Handling",
                                                   "Determines if the arrays that make up the geometry primitives should be Moved or Copied to the created Geometry object.",
                                                   to_underlying(ArrayHandlingType::Move), ChoicesParameter::Choices{"Copy Attribute Arrays", "Move Attribute Arrays" /*, "Reference Array"*/}));

  params.insert(std::make_unique<VectorUInt64Parameter>(k_Dimensions_Key, "Dimensions", "The number of cells in each of the X, Y, Z directions", std::vector<uint64_t>{20ULL, 60ULL, 200ULL},
                                                        std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of each of the axes in X, Y, Z order", std::vector<float32>(3), std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "The length scale of each voxel/pixel", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X"s, "Y"s, "Z"s}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_XBoundsPath_Key, "X Bounds", "The spatial locations of the planes along the x direction", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_YBoundsPath_Key, "Y Bounds", "The spatial locations of the planes along the y direction", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ZBoundsPath_Key, "Z Bounds", "The spatial locations of the planes along the z direction", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_VertexListPath_Key, "Shared Vertex List", "The complete path to the data array defining the point coordinates for the geometry", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_EdgeListPath_Key, "Edge List", "The complete path to the data array defining the edges for the geometry", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint64}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleListPath_Key, "Triangle List", "The complete path to the data array defining the (triangular) faces for the geometry", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint64}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuadrilateralListPath_Key, "Quadrilateral List", "The complete path to the data array defining the (quad) faces for the geometry",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TetrahedralListPath_Key, "Tetrahedral List", "The complete path to the data array defining the tetrahedral elements for the geometry",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_HexahedralListPath_Key, "Hexahedral List", "The complete path to the data array defining the hexahedral elements for the geometry",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64}, ArraySelectionParameter::AllowedComponentShapes{{8}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_GeometryPath_Key, "Geometry Name", "The complete path to the geometry to be created", DataPath({"Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "The name of the vertex attribute matrix to be created with the geometry",
                                                          INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "The name of the edge attribute matrix to be created with the geometry",
                                                          INodeGeometry1D::k_EdgeDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceAttributeMatrixName_Key, "Face Attribute Matrix", "The name of the face attribute matrix to be created with the geometry",
                                                          INodeGeometry2D::k_FaceDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the cell attribute matrix to be created with the geometry",
                                                          IGridGeometry::k_CellDataName));

  // setup linked parameters
  // image
  params.linkParameters(k_GeometryType_Key, k_Dimensions_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GeometryType_Key, k_Origin_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GeometryType_Key, k_Spacing_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GeometryType_Key, k_CellAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(0));
  // rect grid
  params.linkParameters(k_GeometryType_Key, k_XBoundsPath_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_GeometryType_Key, k_YBoundsPath_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_GeometryType_Key, k_ZBoundsPath_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_GeometryType_Key, k_CellAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(1));
  // vertex
  params.linkParameters(k_GeometryType_Key, k_VertexListPath_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(2));
  // edge
  params.linkParameters(k_GeometryType_Key, k_VertexListPath_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_GeometryType_Key, k_EdgeListPath_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_GeometryType_Key, k_EdgeAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(3));
  // triangle
  params.linkParameters(k_GeometryType_Key, k_VertexListPath_Key, std::make_any<ChoicesParameter::ValueType>(4));
  params.linkParameters(k_GeometryType_Key, k_TriangleListPath_Key, std::make_any<ChoicesParameter::ValueType>(4));
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(4));
  params.linkParameters(k_GeometryType_Key, k_FaceAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(4));
  // quad
  params.linkParameters(k_GeometryType_Key, k_VertexListPath_Key, std::make_any<ChoicesParameter::ValueType>(5));
  params.linkParameters(k_GeometryType_Key, k_QuadrilateralListPath_Key, std::make_any<ChoicesParameter::ValueType>(5));
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(5));
  params.linkParameters(k_GeometryType_Key, k_FaceAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(5));
  // tet
  params.linkParameters(k_GeometryType_Key, k_VertexListPath_Key, std::make_any<ChoicesParameter::ValueType>(6));
  params.linkParameters(k_GeometryType_Key, k_TetrahedralListPath_Key, std::make_any<ChoicesParameter::ValueType>(6));
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(6));
  params.linkParameters(k_GeometryType_Key, k_CellAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(6));
  // hex
  params.linkParameters(k_GeometryType_Key, k_VertexListPath_Key, std::make_any<ChoicesParameter::ValueType>(7));
  params.linkParameters(k_GeometryType_Key, k_HexahedralListPath_Key, std::make_any<ChoicesParameter::ValueType>(7));
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(7));
  params.linkParameters(k_GeometryType_Key, k_CellAttributeMatrixName_Key, std::make_any<ChoicesParameter::ValueType>(7));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateGeometryFilter::clone() const
{
  return std::make_unique<CreateGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto pGeometryType = filterArgs.value<ChoicesParameter::ValueType>(k_GeometryType_Key);
  auto pArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // collect arguments common to multiple geometry types all at once
  DataPath pVertexListPath;
  std::string pVertexAMName;
  std::string pFaceAMName;
  std::string pCellAMName;
  if(pGeometryType == k_VertexGeometry || pGeometryType == k_EdgeGeometry || pGeometryType == k_TriangleGeometry || pGeometryType == k_QuadGeometry || pGeometryType == k_TetGeometry ||
     pGeometryType == k_HexGeometry)
  {
    pVertexListPath = filterArgs.value<DataPath>(k_VertexListPath_Key);
    pVertexAMName = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);

    if(dataStructure.getDataAs<Float32Array>(pVertexListPath) == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9840, fmt::format("Cannot find selected vertex list at path '{}'", pVertexListPath.toString())}})};
    }
  }
  if(pGeometryType == k_TriangleGeometry || pGeometryType == k_QuadGeometry)
  {
    pFaceAMName = filterArgs.value<std::string>(k_FaceAttributeMatrixName_Key);
  }
  if(pGeometryType == k_ImageGeometry || pGeometryType == k_RectGridGeometry || pGeometryType == k_TetGeometry || pGeometryType == k_HexGeometry)
  {
    pCellAMName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  }

  // create geometry actions & deferred delete data actions if move arrays option selected
  if(pGeometryType == k_ImageGeometry) // ImageGeom
  {
    auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
    auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
    auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

    std::stringstream ss;
    ss << "Extents:\n"
       << "X Extent: 0 to " << pDimensionsValue[0] - 1 << " (dimension: " << pDimensionsValue[0] << ")\n"
       << "Y Extent: 0 to " << pDimensionsValue[1] - 1 << " (dimension: " << pDimensionsValue[1] << ")\n"
       << "Z Extent: 0 to " << pDimensionsValue[2] - 1 << " (dimension: " << pDimensionsValue[2] << ")\n"
       << "Bounds:\n"
       << "X Range: " << pOriginValue[0] << " to " << (pOriginValue[0] + static_cast<float>(pDimensionsValue[0]) * pSpacingValue[0])
       << " (delta: " << (static_cast<float>(pDimensionsValue[0]) * pSpacingValue[0]) << ")\n"
       << "Y Range: " << (pOriginValue[1]) << " to " << (pOriginValue[1] + static_cast<float>(pDimensionsValue[1]) * pSpacingValue[1])
       << " (delta: " << (static_cast<float>(pDimensionsValue[1]) * pSpacingValue[1]) << ")\n"
       << "Z Range: " << (pOriginValue[2]) << " to " << (pOriginValue[2] + static_cast<float>(pDimensionsValue[2]) * pSpacingValue[2])
       << " (delta: " << (static_cast<float>(pDimensionsValue[2]) * pSpacingValue[2]) << ")\n";
    std::string boxDimensions = ss.str();

    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(
        pGeometryPath, CreateImageGeometryAction::DimensionType({pDimensionsValue[0], pDimensionsValue[1], pDimensionsValue[2]}), pOriginValue, pSpacingValue, pCellAMName);

    resultOutputActions.value().appendAction(std::move(createImageGeometryAction));
    preflightUpdatedValues.push_back({"BoxDimensions", boxDimensions});
  }
  if(pGeometryType == k_RectGridGeometry) // RectGridGeom
  {
    auto pXBoundsPath = filterArgs.value<DataPath>(k_XBoundsPath_Key);
    auto pYBoundsPath = filterArgs.value<DataPath>(k_YBoundsPath_Key);
    auto pZBoundsPath = filterArgs.value<DataPath>(k_ZBoundsPath_Key);
    const auto xBounds = dataStructure.getDataAs<Float32Array>(pXBoundsPath);
    if(xBounds == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9841, fmt::format("Cannot find selected quadrilateral list at path '{}'", pXBoundsPath.toString())}})};
    }
    const auto yBounds = dataStructure.getDataAs<Float32Array>(pYBoundsPath);
    if(yBounds == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9842, fmt::format("Cannot find selected quadrilateral list at path '{}'", pYBoundsPath.toString())}})};
    }
    const auto zBounds = dataStructure.getDataAs<Float32Array>(pZBoundsPath);
    if(zBounds == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9843, fmt::format("Cannot find selected quadrilateral list at path '{}'", pZBoundsPath.toString())}})};
    }
    usize xTuples = xBounds->getNumberOfTuples();
    usize yTuples = yBounds->getNumberOfTuples();
    usize zTuples = zBounds->getNumberOfTuples();
    if(xTuples < 2 || yTuples < 2 || zTuples < 2)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{-9844, fmt::format("One of the bounds arrays has a size less than two; all sizes must be at least two\nX Bounds Size: {}\nY Bounds Size: {}\nZ Bounds Size: {}\n",
                                                      xBounds->getNumberOfTuples(), yBounds->getNumberOfTuples(), zBounds->getNumberOfTuples())}})};
    }

    auto createRectGridGeometryAction = std::make_unique<CreateRectGridGeometryAction>(pGeometryPath, pXBoundsPath, pYBoundsPath, pZBoundsPath, pCellAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createRectGridGeometryAction));
  }
  if(pGeometryType == k_VertexGeometry) // VertexGeom
  {
    auto createVertexGeomAction = std::make_unique<CreateVertexGeometryAction>(pGeometryPath, pVertexListPath, pVertexAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createVertexGeomAction));
  }
  if(pGeometryType == k_EdgeGeometry) // EdgeGeom
  {
    auto pEdgeListPath = filterArgs.value<DataPath>(k_EdgeListPath_Key);
    auto pEdgeAMName = filterArgs.value<std::string>(k_EdgeAttributeMatrixName_Key);
    if(const auto* edgeList = dataStructure.getDataAs<UInt64Array>(pEdgeListPath); edgeList == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9845, fmt::format("Cannot find selected edge list at path '{}'", pEdgeListPath.toString())}})};
    }

    auto createEdgeGeomAction = std::make_unique<CreateEdgeGeometryAction>(pGeometryPath, pVertexListPath, pEdgeListPath, pVertexAMName, pEdgeAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createEdgeGeomAction));
  }
  if(pGeometryType == k_TriangleGeometry) // TriangleGeom
  {
    auto pTriangleListPath = filterArgs.value<DataPath>(k_TriangleListPath_Key);
    if(const auto* triangleList = dataStructure.getDataAs<UInt64Array>(pTriangleListPath); triangleList == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9846, fmt::format("Cannot find selected triangle list at path '{}'", pTriangleListPath.toString())}})};
    }

    auto createTriangleGeomAction = std::make_unique<CreateTriangleGeometryAction>(pGeometryPath, pVertexListPath, pTriangleListPath, pVertexAMName, pFaceAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createTriangleGeomAction));
  }
  if(pGeometryType == k_QuadGeometry) // QuadGeom
  {
    auto pQuadListPath = filterArgs.value<DataPath>(k_QuadrilateralListPath_Key);
    if(const auto* quadList = dataStructure.getDataAs<UInt64Array>(pQuadListPath); quadList == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9847, fmt::format("Cannot find selected quadrilateral list at path '{}'", pQuadListPath.toString())}})};
    }

    auto createQuadGeomAction = std::make_unique<CreateQuadGeometryAction>(pGeometryPath, pVertexListPath, pQuadListPath, pVertexAMName, pFaceAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createQuadGeomAction));
  }
  if(pGeometryType == k_TetGeometry) // TetrahedralGeom
  {
    auto pTetListPath = filterArgs.value<DataPath>(k_TetrahedralListPath_Key);
    if(const auto* tetList = dataStructure.getDataAs<UInt64Array>(pTetListPath); tetList == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9848, fmt::format("Cannot find selected quadrilateral list at path '{}'", pTetListPath.toString())}})};
    }

    auto createTetGeomAction = std::make_unique<CreateTetrahedralGeometryAction>(pGeometryPath, pVertexListPath, pTetListPath, pVertexAMName, pCellAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createTetGeomAction));
  }
  if(pGeometryType == k_HexGeometry) // HexahedralGeom
  {
    auto pHexListPath = filterArgs.value<DataPath>(k_HexahedralListPath_Key);
    if(const auto* hexList = dataStructure.getDataAs<UInt64Array>(pHexListPath); hexList == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-9849, fmt::format("Cannot find selected quadrilateral list at path '{}'", pHexListPath.toString())}})};
    }

    auto createHexGeomAction = std::make_unique<CreateHexahedralGeometryAction>(pGeometryPath, pVertexListPath, pHexListPath, pVertexAMName, pCellAMName, ArrayHandlingType{pArrayHandling});
    resultOutputActions.value().appendAction(std::move(createHexGeomAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto geometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto geometryType = filterArgs.value<ChoicesParameter::ValueType>(k_GeometryType_Key);
  auto treatWarningsAsErrors = filterArgs.value<bool>(k_WarningsAsErrors_Key);

  auto* iGeometry = dataStructure.getDataAs<IGeometry>(geometryPath);
  auto lengthUnit = static_cast<IGeometry::LengthUnit>(filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnitType_Key));
  iGeometry->setUnits(lengthUnit);

  DataPath sharedVertexListArrayPath;
  DataPath sharedFaceListArrayPath;
  DataPath sharedCellListArrayPath;

  if(geometryType == k_VertexGeometry || geometryType == k_EdgeGeometry || geometryType == k_TriangleGeometry || geometryType == k_QuadGeometry || geometryType == k_TetGeometry || geometryType == 7)
  {
    sharedVertexListArrayPath = filterArgs.value<DataPath>(k_VertexListPath_Key);
  }
  if(geometryType == k_TriangleGeometry)
  {
    sharedFaceListArrayPath = filterArgs.value<DataPath>(k_TriangleListPath_Key);
  }
  if(geometryType == k_QuadGeometry)
  {
    sharedFaceListArrayPath = filterArgs.value<DataPath>(k_QuadrilateralListPath_Key);
  }
  if(geometryType == k_TetGeometry)
  {
    sharedCellListArrayPath = filterArgs.value<DataPath>(k_TetrahedralListPath_Key);
  }
  if(geometryType == k_HexGeometry)
  {
    sharedCellListArrayPath = filterArgs.value<DataPath>(k_HexahedralListPath_Key);
  }

  Result<> warningResults;

  // These checks must be done in execute since we are accessing the array values!
  if(geometryType == k_EdgeGeometry)
  {
    auto sharedEdgeListArrayPath = filterArgs.value<DataPath>(k_EdgeListPath_Key);
    const DataPath destEdgeListPath = geometryPath.createChildPath(sharedEdgeListArrayPath.getTargetName());
    const auto& edgesList = dataStructure.getDataAs<UInt64Array>(destEdgeListPath)->getDataStoreRef();
    const auto& vertexList = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(sharedVertexListArrayPath.getTargetName()))->getDataStoreRef();
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
    const auto& faceList = dataStructure.getDataAs<UInt64Array>(destFaceListPath)->getDataStoreRef();
    const auto& vertexList = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(sharedVertexListArrayPath.getTargetName()))->getDataStoreRef();
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
    const auto& cellList = dataStructure.getDataAs<UInt64Array>(destCellListPath)->getDataStoreRef();
    const auto& vertexList = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(sharedVertexListArrayPath.getTargetName()))->getDataStoreRef();
    auto results = checkGeometryArraysCompatible(vertexList, cellList, treatWarningsAsErrors, (geometryType == 6 ? "tetrahedral" : "hexahedral"));
    if(results.invalid())
    {
      return results;
    }
    warningResults.warnings().insert(warningResults.warnings().end(), results.warnings().begin(), results.warnings().end());
  }
  if(geometryType == k_RectGridGeometry)
  {
    auto xBoundsArrayPath = filterArgs.value<DataPath>(k_XBoundsPath_Key);
    auto yBoundsArrayPath = filterArgs.value<DataPath>(k_YBoundsPath_Key);
    auto zBoundsArrayPath = filterArgs.value<DataPath>(k_ZBoundsPath_Key);
    const auto& srcXBounds = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(xBoundsArrayPath.getTargetName()))->getDataStoreRef();
    const auto& srcYBounds = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(yBoundsArrayPath.getTargetName()))->getDataStoreRef();
    const auto& srcZBounds = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(zBoundsArrayPath.getTargetName()))->getDataStoreRef();
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
} // namespace nx::core
