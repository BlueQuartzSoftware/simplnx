#include "CombineNodeBasedGeometriesFilter.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry3DAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/DataObjectUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/CombineNodeBasedGeometries.hpp"

#include <filesystem>
#include <ranges>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
struct GeometryArrayInfo
{
  std::string name;
  std::vector<usize> compDims;
  IArray::ArrayType arrayType;
  std::optional<DataType> dataType;
};

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
std::tuple<bool, bool, std::vector<GeometryArrayInfo>> FindGeometryElements(const IGeometry* geom, GetArrayFunc getArray, GetAttrMatrixFunc getAttrMatrix)
{
  bool arrayExists = false;
  bool attrMatrixExists = false;
  std::vector<GeometryArrayInfo> geometryArraysInfo;

  // Perform dynamic_cast to the specific geometry type
  const auto* nodeGeomPtr = dynamic_cast<const NodeGeomType*>(geom);
  if(nodeGeomPtr != nullptr)
  {
    // Retrieve the array (edges, faces, polyhedra, etc.)
    auto* array = getArray(nodeGeomPtr);
    arrayExists = (array != nullptr);

    // Retrieve the attribute matrix
    auto* attrMatrix = getAttrMatrix(nodeGeomPtr);
    attrMatrixExists = (attrMatrix != nullptr);
    if(attrMatrixExists)
    {
      for(const auto& item : *attrMatrix)
      {
        auto* iDataArray = dynamic_cast<IDataArray*>(item.second.get());
        auto* iNeighborList = dynamic_cast<INeighborList*>(item.second.get());
        auto* iArray = dynamic_cast<IArray*>(item.second.get());
        if(iDataArray != nullptr)
        {
          geometryArraysInfo.push_back({iDataArray->getName(), iDataArray->getComponentShape(), iDataArray->getArrayType(), iDataArray->getDataType()});
        }
        else if(iNeighborList != nullptr)
        {
          geometryArraysInfo.push_back({iNeighborList->getName(), iNeighborList->getComponentShape(), iNeighborList->getArrayType(), iNeighborList->getDataType()});
        }
        else if(iArray != nullptr)
        {
          geometryArraysInfo.push_back({iArray->getName(), iArray->getComponentShape(), iArray->getArrayType(), {}});
        }
      }
    }
  }

  return std::make_tuple(arrayExists, attrMatrixExists, geometryArraysInfo);
}

std::tuple<bool, bool, std::vector<GeometryArrayInfo>> FindVertexElements(const IGeometry* geom)
{
  auto getVerticesArrayFunc = [](const INodeGeometry0D* ptr) -> auto { return ptr->getVertices(); };
  auto getVertexAttrMatrixFunc = [](const INodeGeometry0D* ptr) -> auto { return ptr->getVertexAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry0D>(geom, getVerticesArrayFunc, getVertexAttrMatrixFunc);
}

std::tuple<bool, bool, std::vector<GeometryArrayInfo>> FindEdgeElements(const IGeometry* geom)
{
  auto getEdgesArrayFunc = [](const INodeGeometry1D* ptr) -> auto { return ptr->getEdges(); };
  auto getEdgeAttrMatrixFunc = [](const INodeGeometry1D* ptr) -> auto { return ptr->getEdgeAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry1D>(geom, getEdgesArrayFunc, getEdgeAttrMatrixFunc);
}

std::tuple<bool, bool, std::vector<GeometryArrayInfo>> FindFaceElements(const IGeometry* geom)
{
  auto getFacesArrayFunc = [](const INodeGeometry2D* ptr) -> auto { return ptr->getFaces(); };
  auto getFaceAttrMatrixFunc = [](const INodeGeometry2D* ptr) -> auto { return ptr->getFaceAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry2D>(geom, getFacesArrayFunc, getFaceAttrMatrixFunc);
}

std::tuple<bool, bool, std::vector<GeometryArrayInfo>> FindPolyElements(const IGeometry* geom)
{
  auto getPolyArrayFunc = [](const INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedra(); };
  auto getPolyAttrMatrixFunc = [](const INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedraAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry3D>(geom, getPolyArrayFunc, getPolyAttrMatrixFunc);
}

/**
 * Validates that the given geometry element exists across all input geometries.  Returns an error if the element exists in some geometries but not others.
 * @param elementsExist The vector of optional GeometryArrayInfo instances.  If the optional does not have a value, then that data array does not exist for the input geometry at that index.
 * @param inputGeometryPaths The paths to the input geometries (used for a more specific error message)
 * @return
 */
Result<bool> ValidateGeometryElementExists(const std::vector<bool>& elementsExist, const std::vector<DataPath>& inputGeometryPaths)
{
  // Check if all elements are true
  if(std::all_of(elementsExist.begin(), elementsExist.end(), [](bool b) { return b; }))
  {
    return {true};
  }

  // Check if all elements are false
  if(std::all_of(elementsExist.begin(), elementsExist.end(), [](bool b) { return !b; }))
  {
    return {false};
  }

  // Mixed case: report geometries that do and do not contain the given geometry element
  std::vector<size_t> trueIndices;
  std::vector<size_t> falseIndices;

  for(size_t i = 0; i < elementsExist.size(); ++i)
  {
    if(elementsExist[i])
    {
      trueIndices.push_back(i);
    }
    else
    {
      falseIndices.push_back(i);
    }
  }

  // Create error message
  std::ostringstream oss;

  for(size_t i = 0; i < falseIndices.size(); ++i)
  {
    oss << inputGeometryPaths[falseIndices[i]].toString();
    if(i != falseIndices.size() - 1)
    {
      oss << "\n";
    }
  }

  return MakeErrorResult<bool>(-1, oss.str());
}

Result<bool> DoesGeometryElementExist(const std::vector<bool>& elementsExist, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
{
  auto result = ValidateGeometryElementExists(elementsExist, inputGeometryPaths);
  if(result.invalid())
  {
    std::string message = fmt::format("A few of the selected geometries omit {} even though the other geometries contain {}. All geometries MUST be consistent and "
                                      "either contain {} or omit {}. The following geometries omit {}:\n\n{}",
                                      arrayDescription, arrayDescription, arrayDescription, arrayDescription, arrayDescription, result.errors()[0].message);
    return MakeErrorResult<bool>(to_underlying(CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements), message);
  }

  return {result.value()};
}

Result<bool> DoesGeometryElementExist(const std::vector<std::optional<GeometryArrayInfo>>& arraysInfo, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
{
  std::vector<bool> bools(arraysInfo.size());
  std::transform(arraysInfo.begin(), arraysInfo.end(), bools.begin(), [](const std::optional<GeometryArrayInfo>& opt) -> bool { return opt.has_value(); });
  return DoesGeometryElementExist(bools, inputGeometryPaths, arrayDescription);
}

template <typename Getter>
auto FindInconsistentDataArrayProperty(const std::vector<GeometryArrayInfo>& arraysInfo, Getter getter)
{
  if(arraysInfo.empty())
  {
    return arraysInfo.end();
  }

  const auto& firstElementInfo = arraysInfo[0];
  return std::find_if_not(arraysInfo.begin(), arraysInfo.end(), [&firstElementInfo, getter](const GeometryArrayInfo& arrayInfo) { return getter(arrayInfo) == getter(firstElementInfo); });
}

Result<> ValidateDataArrayTypes(const std::vector<GeometryArrayInfo>& arraysInfo)
{
  auto iter = FindInconsistentDataArrayProperty(arraysInfo, [](const GeometryArrayInfo& arrayInfo) { return arrayInfo.arrayType; });
  if(iter != arraysInfo.end())
  {
    auto first = arraysInfo[0];
    auto arrayInfo = *iter;
    return MakeErrorResult(to_underlying(CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementArrayTypes),
                           fmt::format("Object at path '{}' has array type '{}' and object at path '{}' has array type '{}'.  Both of these array types must be the same.", first.name,
                                       DataObjectTypeToString(ConvertArrayTypeToDataObjectType(first.arrayType)), arrayInfo.name,
                                       DataObjectTypeToString(ConvertArrayTypeToDataObjectType(arrayInfo.arrayType))));
  }
  return {};
}

Result<> ValidateDataArrayCompDimensions(const std::vector<GeometryArrayInfo>& arraysInfo)
{
  auto iter = FindInconsistentDataArrayProperty(arraysInfo, [](const GeometryArrayInfo& arrayInfo) { return arrayInfo.compDims; });
  if(iter != arraysInfo.end())
  {
    auto first = arraysInfo[0];
    auto arrayInfo = *iter;
    return MakeErrorResult(to_underlying(CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementCompDims),
                           fmt::format("Object at path '{}' has component dimensions '{}' and object at path '{}' has component dimensions '{}'.  Both of these component dimensions must be the same.",
                                       first.name, fmt::join(first.compDims, "x"), arrayInfo.name, fmt::join(arrayInfo.compDims, "x")));
  }
  return {};
}

Result<> ValidateDataArrayDataTypes(const std::vector<GeometryArrayInfo>& arraysInfo)
{
  auto iter = FindInconsistentDataArrayProperty(arraysInfo, [](const GeometryArrayInfo& arrayInfo) { return arrayInfo.dataType; });
  if(iter != arraysInfo.end())
  {
    auto first = arraysInfo[0];
    auto arrayInfo = *iter;
    std::string errMsg;
    if(!first.dataType.has_value())
    {
      errMsg = fmt::format("Object at path '{}' has no data type and object at path '{}' has data type '{}'.  Both of these data types must exist and be the same.", first.name, arrayInfo.name,
                           DataTypeToString(arrayInfo.dataType.value()));
    }
    else if(!arrayInfo.dataType.has_value())
    {
      errMsg = fmt::format("Object at path '{}' has data type '{}' and object at path '{}' has no data type.  Both of these data types must exist and be the same.", first.name,
                           DataTypeToString(first.dataType.value()), arrayInfo.name);
    }
    else
    {
      errMsg = fmt::format("Object at path '{}' has data type '{}' and object at path '{}' has data type '{}'.  Both of these data types must be the same.", first.name,
                           DataTypeToString(first.dataType.value()), arrayInfo.name, DataTypeToString(arrayInfo.dataType.value()));
    }

    return MakeErrorResult(to_underlying(CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementDataTypes), errMsg);
  }
  return {};
}

Result<> AddOutputArray(const GeometryArrayInfo& arrayInfo, const DataPath& outputGeomPath, const std::string& attrMatrixName, OutputActions& actions)
{
  switch(arrayInfo.arrayType)
  {
  case IArray::ArrayType::DataArray: {
    actions.appendAction(
        std::make_unique<CreateArrayAction>(arrayInfo.dataType.value(), std::vector<usize>{1}, arrayInfo.compDims, outputGeomPath.createChildPath(attrMatrixName).createChildPath(arrayInfo.name)));
    break;
  }
  case IArray::ArrayType::StringArray: {
    actions.appendAction(std::make_unique<CreateStringArrayAction>(std::vector<usize>{1}, outputGeomPath.createChildPath(attrMatrixName).createChildPath(arrayInfo.name)));
    break;
  }
  case IArray::ArrayType::NeighborListArray: {
    actions.appendAction(std::make_unique<CreateNeighborListAction>(arrayInfo.dataType.value(), 1, outputGeomPath.createChildPath(attrMatrixName).createChildPath(arrayInfo.name)));
    break;
  }
  case IArray::ArrayType::Any: {
    return MakeErrorResult(-56, fmt::format("Geometry at path '{}' has array with name '{}' that has array type 'Any'.  This should NEVER happen.  Please contact the developers.", arrayInfo.name,
                                            outputGeomPath.toString()));
  }
  }

  return {};
}

Result<> CreateINodeGeometry0DObjects(const DataPath& outputGeomPath, const std::vector<GeometryArrayInfo>& vertexDataArraysInfo, OutputActions& actions)
{
  for(const auto& vertexDataArrayInfo : vertexDataArraysInfo)
  {
    auto result = AddOutputArray(vertexDataArrayInfo, outputGeomPath, INodeGeometry0D::k_VertexAttributeMatrixName, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

template <class INodeGeom>
Result<> CreateINodeGeometry1DObjects(const DataPath& outputGeomPath, bool edgesArrayExists, bool edgeAttrMatrixExists, const std::vector<GeometryArrayInfo>& edgeDataArraysInfo,
                                      OutputActions& actions)
{
  if constexpr(!std::is_same_v<INodeGeometry1D, INodeGeom>)
  {
    // Create Edge Attribute Matrix and Edges Array
    if(edgeAttrMatrixExists)
    {
      actions.appendAction(std::make_unique<CreateAttributeMatrixAction>(outputGeomPath.createChildPath(INodeGeometry1D::k_EdgeAttributeMatrixName), AttributeMatrix::ShapeType{1}));
    }
    if(edgesArrayExists)
    {
      actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{2}, outputGeomPath.createChildPath(INodeGeometry1D::k_SharedEdgeListName)));
    }
  }

  for(const auto& edgeDataArrayInfo : edgeDataArraysInfo)
  {
    auto result = AddOutputArray(edgeDataArrayInfo, outputGeomPath, INodeGeometry1D::k_EdgeAttributeMatrixName, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

template <class NodeGeom, class INodeGeom>
Result<> CreateINodeGeometry2DObjects(const DataPath& outputGeomPath, bool facesArrayExists, bool faceAttrMatrixExists, const std::vector<GeometryArrayInfo>& faceDataArraysInfo,
                                      OutputActions& actions)
{
  if constexpr(!std::is_same_v<INodeGeometry2D, INodeGeom>)
  {
    // Create Face Attribute Matrix and Faces Array
    if(faceAttrMatrixExists)
    {
      actions.appendAction(std::make_unique<CreateAttributeMatrixAction>(outputGeomPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName), AttributeMatrix::ShapeType{1}));
    }
    if(facesArrayExists)
    {
      if constexpr(std::is_same_v<TetrahedralGeom, NodeGeom>)
      {
        actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{TetrahedralGeom::k_NumFaceVerts},
                                                                 outputGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName)));
      }
      else if constexpr(std::is_same_v<HexahedralGeom, NodeGeom>)
      {
        actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{HexahedralGeom::k_NumFaceVerts},
                                                                 outputGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName)));
      }
    }
  }

  for(const auto& faceDataArrayInfo : faceDataArraysInfo)
  {
    auto result = AddOutputArray(faceDataArrayInfo, outputGeomPath, INodeGeometry2D::k_FaceAttributeMatrixName, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> CreateINodeGeometry3DObjects(const DataPath& outputGeomPath, const std::vector<GeometryArrayInfo>& polyDataArraysInfo, OutputActions& actions)
{
  for(const auto& polyDataArrayInfo : polyDataArraysInfo)
  {
    auto result = AddOutputArray(polyDataArrayInfo, outputGeomPath, INodeGeometry3D::k_PolyhedronDataName, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

template <class NodeGeom, class INodeGeom>
Result<> CreateOtherAttrMatricesAndArrays(const DataPath& outputGeomPath, const std::vector<GeometryArrayInfo>& vertexDataArraysInfo, bool edgesArrayExists, bool edgeAttrMatrixExists,
                                          const std::vector<GeometryArrayInfo>& edgeDataArraysInfo, bool facesArrayExists, bool faceAttrMatrixExists,
                                          const std::vector<GeometryArrayInfo>& faceDataArraysInfo, const std::vector<GeometryArrayInfo>& polyDataArraysInfo, OutputActions& actions)
{
  if constexpr(std::is_base_of_v<INodeGeometry3D, NodeGeom>)
  {
    auto result = CreateINodeGeometry3DObjects(outputGeomPath, polyDataArraysInfo, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  if constexpr(std::is_base_of_v<INodeGeometry2D, NodeGeom>)
  {
    auto result = CreateINodeGeometry2DObjects<NodeGeom, INodeGeom>(outputGeomPath, facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  if constexpr(std::is_base_of_v<INodeGeometry1D, NodeGeom>)
  {
    auto result = CreateINodeGeometry1DObjects<INodeGeom>(outputGeomPath, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  if constexpr(std::is_base_of_v<INodeGeometry0D, NodeGeom>)
  {
    auto result = CreateINodeGeometry0DObjects(outputGeomPath, vertexDataArraysInfo, actions);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

template <typename FindFunc>
void RecordElementPresence(FindFunc findFunc, const IGeometry* geom, usize i, usize totalCount, std::vector<bool>& arraysExist, std::vector<bool>& attrMatricesExist,
                           std::map<std::string, std::vector<std::optional<GeometryArrayInfo>>>& dataArraysExistMap)
{
  // Call the provided find function, which returns a tuple.
  auto [arrayExists, attrMatrixExists, dataArraysInfo] = findFunc(geom);

  // Update the boolean arrays.
  arraysExist[i] = arrayExists;
  attrMatricesExist[i] = attrMatrixExists;

  // Update the map entries for each data array info.
  for(const auto& arrayInfo : dataArraysInfo)
  {
    const std::string& arrayName = arrayInfo.name;
    auto& arraysInfo = dataArraysExistMap[arrayName];
    if(arraysInfo.empty())
    {
      arraysInfo.resize(totalCount);
    }
    arraysInfo[i] = arrayInfo;
  }
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CombineNodeBasedGeometriesFilter::name() const
{
  return FilterTraits<CombineNodeBasedGeometriesFilter>::name;
}

//------------------------------------------------------------------------------
std::string CombineNodeBasedGeometriesFilter::className() const
{
  return FilterTraits<CombineNodeBasedGeometriesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CombineNodeBasedGeometriesFilter::uuid() const
{
  return FilterTraits<CombineNodeBasedGeometriesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineNodeBasedGeometriesFilter::humanName() const
{
  return "Combine Node Based Geometries";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineNodeBasedGeometriesFilter::defaultTags() const
{
  return {className(), "Combine", "Mix", "Blend", "Integrate", "Fuse", "Merge", "Node", "Geometry"};
}

//------------------------------------------------------------------------------
Parameters CombineNodeBasedGeometriesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(
      k_InputGeometries_Key, "Input Geometries", "The incoming geometries that will be combined together into the destination geometry.  All geometries must be of the same geometry type.",
      std::vector<DataPath>{}));

  params.insertSeparator(Parameters::Separator{"Output Parameter(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputGeometryPath_Key, "Combined Geometry", "The path to the combined geometry", DataPath({"Combined Geometry"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CombineNodeBasedGeometriesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineNodeBasedGeometriesFilter::clone() const
{
  return std::make_unique<CombineNodeBasedGeometriesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineNodeBasedGeometriesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto inputGeometryPaths = args.value<MultiPathSelectionParameter::ValueType>(k_InputGeometries_Key);
  auto outputGeometryPath = args.value<DataGroupCreationParameter::ValueType>(k_OutputGeometryPath_Key);

  if(inputGeometryPaths.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(CombineNodeBasedGeometries::ErrorCodes::FewerThanTwoPathsChosen),
                                           fmt::format("No input geometry paths have been chosen.  Please choose at least two input geometry paths."))};
  }

  if(inputGeometryPaths.size() == 1)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(CombineNodeBasedGeometries::ErrorCodes::FewerThanTwoPathsChosen),
                                           fmt::format("Only one input geometry path has been chosen.  Please choose at least two input geometry paths."))};
  }

  // Use an optional so that the first input geometry type sets the optional
  // and subsequent input geometries' types are checked against the optional
  std::optional<IGeometry::Type> geometryTypeOpt;

  // All of these structures are used to keep track of which attribute matrices
  // and arrays exist in each input geometry.  These are then used later in preflight
  // to check that all data is consistent across all the input geometries.
  std::vector<bool> vertexArraysExist(inputGeometryPaths.size());
  std::vector<bool> vertexAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<GeometryArrayInfo>>> vertexDataArraysExistMap;
  std::vector<bool> edgeArraysExist(inputGeometryPaths.size());
  std::vector<bool> edgeAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<GeometryArrayInfo>>> edgeDataArraysExistMap;
  std::vector<bool> faceArraysExist(inputGeometryPaths.size());
  std::vector<bool> faceAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<GeometryArrayInfo>>> faceDataArraysExistMap;
  std::vector<bool> polyArraysExist(inputGeometryPaths.size());
  std::vector<bool> polyAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<GeometryArrayInfo>>> polyDataArraysExistMap;

  // Loop over all the input geometries, check that each one is a node geometry of the same type,
  // and then record each geometry's vertex, edge, face, and polyhedra data in the various structures
  for(usize i = 0; i < inputGeometryPaths.size(); ++i)
  {
    const auto& inputGeometryPath = inputGeometryPaths[i];
    const auto* iGeomPtr = dataStructure.getDataAs<IGeometry>(inputGeometryPath);
    if(iGeomPtr == nullptr)
    {
      // This is not a geometry
      return {MakeErrorResult<OutputActions>(
          to_underlying(CombineNodeBasedGeometries::ErrorCodes::ObjectNotAGeometry),
          fmt::format("The data object at data path '{}' is not a geometry.  All data objects MUST be geometries with the same geometry type.", inputGeometryPath.toString()))};
    }
    const auto* iNodeGeomPtr = dataStructure.getDataAs<INodeGeometry0D>(inputGeometryPath);
    if(iNodeGeomPtr == nullptr)
    {
      // This is not a node geometry
      return {
          MakeErrorResult<OutputActions>(to_underlying(CombineNodeBasedGeometries::ErrorCodes::ObjectNotANodeGeometry),
                                         fmt::format("The data object at data path '{}' is not a node geometry.  Only node geometries are supported by this filter.", inputGeometryPath.toString()))};
    }

    auto* verticesArray = iNodeGeomPtr->getVertices();
    if(verticesArray == nullptr)
    {
      // There are no vertices, which means this is not a valid node geometry
      return {MakeErrorResult<OutputActions>(to_underlying(CombineNodeBasedGeometries::ErrorCodes::NodeGeometryHasNoVertices),
                                             fmt::format("The chosen node geometries do not have a shared vertex array.  All node geometries MUST have a shared vertex array."))};
    }

    auto& iGeom = *iGeomPtr;

    // If the optional has not been set yet, set it
    if(!geometryTypeOpt.has_value())
    {
      geometryTypeOpt = iGeom.getGeomType();
    }

    // Compare the current geometry's type with the optional to verify the types are the same
    if(iGeom.getGeomType() != geometryTypeOpt.value())
    {
      // This geometry's type is not the same as the other geometries' types
      return {MakeErrorResult<OutputActions>(
          to_underlying(CombineNodeBasedGeometries::ErrorCodes::DifferingGeometryTypes),
          fmt::format("The geometry at data path '{}' has geometry type '{}', which differs from other geometries that have geometry type '{}'.  All geometries MUST have the same geometry type.",
                      inputGeometryPath.toString(), GeometryTypeToString(iGeom.getGeomType()), GeometryTypeToString(geometryTypeOpt.value())))};
    }

    // Determine if the vertex data array and attribute matrix exist or not for this geometry, and update the boolean vectors to record that.  Also update the std::map
    // to record which vertex data arrays are found.  This allows us to keep track of what vertex data exists in which geometries and later check for inconsistencies.
    RecordElementPresence(FindVertexElements, iGeomPtr, i, inputGeometryPaths.size(), vertexArraysExist, vertexAttrMatricesExist, vertexDataArraysExistMap);

    // Determine if the edge data array and attribute matrix exist or not for this geometry, and update the boolean vectors to record that.  Also update the std::map
    // to record which edge data arrays are found.  This allows us to keep track of what edge data exists in which geometries and later check for inconsistencies.
    RecordElementPresence(FindEdgeElements, iGeomPtr, i, inputGeometryPaths.size(), edgeArraysExist, edgeAttrMatricesExist, edgeDataArraysExistMap);

    // Determine if the face data array and attribute matrix exist or not for this geometry, and update the boolean vectors to record that.  Also update the std::map
    // to record which face data arrays are found.  This allows us to keep track of what face data exists in which geometries and later check for inconsistencies.
    RecordElementPresence(FindFaceElements, iGeomPtr, i, inputGeometryPaths.size(), faceArraysExist, faceAttrMatricesExist, faceDataArraysExistMap);

    // Determine if the polyhedron data array and attribute matrix exist or not for this geometry, and update the boolean vectors to record that.  Also update the std::map
    // to record which polyhedron data arrays are found.  This allows us to keep track of what polyhedron data exists in which geometries and later check for inconsistencies.
    RecordElementPresence(FindPolyElements, iGeomPtr, i, inputGeometryPaths.size(), polyArraysExist, polyAttrMatricesExist, polyDataArraysExistMap);
  }

  // Determine whether the vertex array exists in all geometries
  auto boolResult = DoesGeometryElementExist(vertexArraysExist, inputGeometryPaths, "a vertices array");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }

  // Determine whether the vertex attribute matrix exists in all geometries
  boolResult = DoesGeometryElementExist(vertexAttrMatricesExist, inputGeometryPaths, "a vertex attribute matrix");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }

  // Determine whether the vertex data arrays are consistent across all geometries
  std::vector<GeometryArrayInfo> vertexDataArraysInfo;
  for(const auto& [arrayName, arrayInfoOpts] : vertexDataArraysExistMap)
  {
    // Check that the vertex data array exists across all geometries
    std::string arrayDesc = fmt::format("vertex data array '{}'", arrayName);
    boolResult = DoesGeometryElementExist(arrayInfoOpts, inputGeometryPaths, arrayDesc);
    if(boolResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
    }

    // If it does not exist, move on to validating the next vertex data array
    if(!boolResult.value())
    {
      continue;
    }

    std::vector<GeometryArrayInfo> arrayInfos(arrayInfoOpts.size());
    std::transform(arrayInfoOpts.begin(), arrayInfoOpts.end(), arrayInfos.begin(), [](const std::optional<GeometryArrayInfo>& opt) -> GeometryArrayInfo { return opt.value(); });

    // Validate that the vertex data array has the same ArrayType across all geometries
    auto result = ValidateDataArrayTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the vertex data array has the same component dimensions across all geometries
    result = ValidateDataArrayCompDimensions(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the vertex data array has the same DataType across all geometries
    result = ValidateDataArrayDataTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    vertexDataArraysInfo.push_back(arrayInfos[0]);
  }

  // Determine whether the edge array exists in all geometries
  boolResult = DoesGeometryElementExist(edgeArraysExist, inputGeometryPaths, "an edges array");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }
  bool edgesArrayExists = boolResult.value();

  // Determine whether the edge attribute matrix exists in all geometries
  boolResult = DoesGeometryElementExist(edgeAttrMatricesExist, inputGeometryPaths, "an edge attribute matrix");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }
  bool edgeAttrMatrixExists = boolResult.value();

  // Determine whether the edge data arrays are consistent across all geometries
  std::vector<GeometryArrayInfo> edgeDataArraysInfo;
  for(const auto& [arrayName, arrayInfoOpts] : edgeDataArraysExistMap)
  {
    // Check that the edge data array exists across all geometries
    std::string arrayDesc = fmt::format("edge data array '{}'", arrayName);
    boolResult = DoesGeometryElementExist(arrayInfoOpts, inputGeometryPaths, arrayDesc);
    if(boolResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
    }

    // If it does not exist, move on to validating the next edge data array
    if(!boolResult.value())
    {
      continue;
    }

    std::vector<GeometryArrayInfo> arrayInfos(arrayInfoOpts.size());
    std::transform(arrayInfoOpts.begin(), arrayInfoOpts.end(), arrayInfos.begin(), [](const std::optional<GeometryArrayInfo>& opt) -> GeometryArrayInfo { return opt.value(); });

    // Validate that the edge data array has the same ArrayType across all geometries
    auto result = ValidateDataArrayTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the edge data array has the same component dimensions across all geometries
    result = ValidateDataArrayCompDimensions(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the edge data array has the same DataType across all geometries
    result = ValidateDataArrayDataTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    edgeDataArraysInfo.push_back(arrayInfos[0]);
  }

  // Determine whether the face array exists in all geometries
  boolResult = DoesGeometryElementExist(faceArraysExist, inputGeometryPaths, "a faces array");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }
  bool facesArrayExists = boolResult.value();

  // Determine whether the face attribute matrix exists in all geometries
  boolResult = DoesGeometryElementExist(faceAttrMatricesExist, inputGeometryPaths, "a face attribute matrix");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }
  bool faceAttrMatrixExists = boolResult.value();

  // Determine whether the face data arrays are consistent across all geometries
  std::vector<GeometryArrayInfo> faceDataArraysInfo;
  for(const auto& [arrayName, arrayInfoOpts] : faceDataArraysExistMap)
  {
    // Check that the face data array exists across all geometries
    std::string arrayDesc = fmt::format("face data array '{}'", arrayName);
    boolResult = DoesGeometryElementExist(arrayInfoOpts, inputGeometryPaths, arrayDesc);
    if(boolResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
    }

    // If it does not exist, move on to validating the next face data array
    if(!boolResult.value())
    {
      continue;
    }

    std::vector<GeometryArrayInfo> arrayInfos(arrayInfoOpts.size());
    std::transform(arrayInfoOpts.begin(), arrayInfoOpts.end(), arrayInfos.begin(), [](const std::optional<GeometryArrayInfo>& opt) -> GeometryArrayInfo { return opt.value(); });

    // Validate that the face data array has the same ArrayType across all geometries
    auto result = ValidateDataArrayTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the face data array has the same component dimensions across all geometries
    result = ValidateDataArrayCompDimensions(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the face data array has the same DataType across all geometries
    result = ValidateDataArrayDataTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    faceDataArraysInfo.push_back(arrayInfos[0]);
  }

  // Determine whether the polyhedra array exists in all geometries
  boolResult = DoesGeometryElementExist(polyArraysExist, inputGeometryPaths, "a polyhedra array");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }

  // Determine whether the polyhedra attribute matrix exists in all geometries
  boolResult = DoesGeometryElementExist(polyAttrMatricesExist, inputGeometryPaths, "a polyhedra attribute matrix");
  if(boolResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
  }

  // Determine whether the polyhedra data arrays are consistent across all geometries
  std::vector<GeometryArrayInfo> polyDataArraysInfo;
  for(const auto& [arrayName, arrayInfoOpts] : polyDataArraysExistMap)
  {
    // Check that the polyhedra data array exists across all geometries
    std::string arrayDesc = fmt::format("polyhedra data array '{}'", arrayName);
    boolResult = DoesGeometryElementExist(arrayInfoOpts, inputGeometryPaths, arrayDesc);
    if(boolResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(boolResult))), {})};
    }

    // If it does not exist, move on to validating the next polyhedra data array
    if(!boolResult.value())
    {
      continue;
    }

    std::vector<GeometryArrayInfo> arrayInfos(arrayInfoOpts.size());
    std::transform(arrayInfoOpts.begin(), arrayInfoOpts.end(), arrayInfos.begin(), [](const std::optional<GeometryArrayInfo>& opt) -> GeometryArrayInfo { return opt.value(); });

    // Validate that the polyhedra data array has the same ArrayType across all geometries
    auto result = ValidateDataArrayTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the polyhedra data array has the same component dimensions across all geometries
    result = ValidateDataArrayCompDimensions(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    // Validate that the polyhedra data array has the same DataType across all geometries
    result = ValidateDataArrayDataTypes(arrayInfos);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(std::move(result)), {})};
    }
    polyDataArraysInfo.push_back(arrayInfos[0]);
  }

  // Create the output geometry with all its vertex, edge, face, and polyhedra attribute matrices and arrays
  OutputActions actions;
  IGeometry::Type geometryType = geometryTypeOpt.value();
  switch(geometryType)
  {
  case IGeometry::Type::Vertex: {
    actions.appendAction(std::make_unique<CreateVertexGeometryAction>(outputGeometryPath, 1, VertexGeom::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<VertexGeom, INodeGeometry0D>(outputGeometryPath, vertexDataArraysInfo, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo,
                                                                                        facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, polyDataArraysInfo, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Edge: {
    actions.appendAction(std::make_unique<CreateEdgeGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, EdgeGeom::k_EdgeAttributeMatrixName,
                                                                    VertexGeom::k_SharedVertexListName, EdgeGeom::k_SharedEdgeListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<EdgeGeom, INodeGeometry1D>(outputGeometryPath, vertexDataArraysInfo, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo,
                                                                                      facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, polyDataArraysInfo, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Triangle: {
    actions.appendAction(std::make_unique<CreateTriangleGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, TriangleGeom::k_FaceAttributeMatrixName,
                                                                        VertexGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<TriangleGeom, INodeGeometry2D>(outputGeometryPath, vertexDataArraysInfo, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo,
                                                                                          facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, polyDataArraysInfo, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Quad: {
    actions.appendAction(std::make_unique<CreateQuadGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, QuadGeom::k_FaceAttributeMatrixName,
                                                                    VertexGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<QuadGeom, INodeGeometry2D>(outputGeometryPath, vertexDataArraysInfo, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo,
                                                                                      facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, polyDataArraysInfo, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Tetrahedral: {
    actions.appendAction(std::make_unique<CreateTetrahedralGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, TetrahedralGeom::k_PolyhedronDataName,
                                                                           VertexGeom::k_SharedVertexListName, TetrahedralGeom::k_SharedPolyhedronListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<TetrahedralGeom, INodeGeometry3D>(outputGeometryPath, vertexDataArraysInfo, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo,
                                                                                             facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, polyDataArraysInfo, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Hexahedral: {
    actions.appendAction(std::make_unique<CreateHexahedralGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, HexahedralGeom::k_PolyhedronDataName,
                                                                          VertexGeom::k_SharedVertexListName, HexahedralGeom::k_SharedPolyhedronListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<HexahedralGeom, INodeGeometry3D>(outputGeometryPath, vertexDataArraysInfo, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraysInfo,
                                                                                            facesArrayExists, faceAttrMatrixExists, faceDataArraysInfo, polyDataArraysInfo, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Image: {
    [[fallthrough]];
  }
  case IGeometry::Type::RectGrid: {
    return {MakeErrorResult<OutputActions>(
        to_underlying(CombineNodeBasedGeometries::ErrorCodes::ObjectNotANodeGeometry),
        fmt::format("All the chosen geometries have type '{}' and are NOT node geometries.  Only node geometries are supported by this filter.", GeometryTypeToString(geometryType)))};
  }
  default:
    return {MakeErrorResult<OutputActions>(
        to_underlying(CombineNodeBasedGeometries::ErrorCodes::ObjectNotANodeGeometry),
        fmt::format("All the chosen geometries have type '{}' and this type is NOT supported by this filter.  Please contact the developers.", GeometryTypeToString(geometryType)))};
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CombineNodeBasedGeometriesFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  CombineNodeBasedGeometriesInputValues inputValues;

  inputValues.InputGeometryPaths = args.value<MultiPathSelectionParameter::ValueType>(k_InputGeometries_Key);
  inputValues.OutputGeometryPath = args.value<DataGroupCreationParameter::ValueType>(k_OutputGeometryPath_Key);

  return CombineNodeBasedGeometries(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
