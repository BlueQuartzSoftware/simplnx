#include "CombineNodeBasedGeometriesFilter.hpp"

#include "simplnx/Common/TypeTraits.hpp"
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
// struct ElementCounts
//{
//   usize verticesCount;
//   std::optional<usize> edgeCount;
//   std::optional<usize> faceCount;
//   std::optional<usize> polyhedraCount;
// };
//
// template <class NodeGeom>
// ElementCounts GetElementCounts(const NodeGeom& nodeGeom)
//{
//   std::optional<usize> edgeCount;
//   std::optional<usize> faceCount;
//   std::optional<usize> polyhedraCount;
//   if constexpr(std::is_base_of_v<INodeGeometry3D, NodeGeom>)
//   {
//     polyhedraCount = nodeGeom.getNumberOfPolyhedra();
//   }
//   if constexpr(std::is_base_of_v<INodeGeometry2D, NodeGeom>)
//   {
//     faceCount = nodeGeom.getNumberOfFaces();
//   }
//   if constexpr(std::is_base_of_v<INodeGeometry1D, NodeGeom>)
//   {
//     edgeCount = nodeGeom.getNumberOfEdges();
//   }
//
//   return CellCounts(nodeGeom.getNumberOfVertices(), edgeCount, faceCount, polyhedraCount);
// }

struct DataArraySpec
{
  std::string name;
  std::vector<usize> compDims;
  IArray::ArrayType arrayType;
  std::optional<DataType> dataType;
};

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
std::tuple<bool, bool, std::vector<DataArraySpec>> FindGeometryElements(const IGeometry* geom, GetArrayFunc getArray, GetAttrMatrixFunc getAttrMatrix)
{
  bool arrayExists = false;
  bool attrMatrixExists = false;
  std::vector<DataArraySpec> dataArraySpecs;

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
        auto* iArray = dynamic_cast<IArray*>(item.second.get());
        if(iDataArray != nullptr)
        {
          dataArraySpecs.push_back({iDataArray->getName(), iDataArray->getComponentShape(), iDataArray->getArrayType(), iDataArray->getDataType()});
        }
        else if(iArray != nullptr)
        {
          if(iArray != nullptr)
          {
            dataArraySpecs.push_back({iArray->getName(), iArray->getComponentShape(), iArray->getArrayType(), {}});
          }
        }
      }
    }
  }

  return std::make_tuple(arrayExists, attrMatrixExists, dataArraySpecs);
}

std::tuple<bool, bool, std::vector<DataArraySpec>> FindVertexElements(const IGeometry* geom)
{
  auto getVerticesArrayFunc = [](const INodeGeometry0D* ptr) -> auto { return ptr->getVertices(); };
  auto getVertexAttrMatrixFunc = [](const INodeGeometry0D* ptr) -> auto { return ptr->getVertexAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry0D>(geom, getVerticesArrayFunc, getVertexAttrMatrixFunc);
}

std::tuple<bool, bool, std::vector<DataArraySpec>> FindEdgeElements(const IGeometry* geom)
{
  auto getEdgesArrayFunc = [](const INodeGeometry1D* ptr) -> auto { return ptr->getEdges(); };
  auto getEdgeAttrMatrixFunc = [](const INodeGeometry1D* ptr) -> auto { return ptr->getEdgeAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry1D>(geom, getEdgesArrayFunc, getEdgeAttrMatrixFunc);
}

std::tuple<bool, bool, std::vector<DataArraySpec>> FindFaceElements(const IGeometry* geom)
{
  auto getFacesArrayFunc = [](const INodeGeometry2D* ptr) -> auto { return ptr->getFaces(); };
  auto getFaceAttrMatrixFunc = [](const INodeGeometry2D* ptr) -> auto { return ptr->getFaceAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry2D>(geom, getFacesArrayFunc, getFaceAttrMatrixFunc);
}

std::tuple<bool, bool, std::vector<DataArraySpec>> FindPolyElements(const IGeometry* geom)
{
  auto getPolyArrayFunc = [](const INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedra(); };
  auto getPolyAttrMatrixFunc = [](const INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedraAttributeMatrix(); };
  return FindGeometryElements<INodeGeometry3D>(geom, getPolyArrayFunc, getPolyAttrMatrixFunc);
}

/**
 * Validates that the given geometry element exists across all input geometries.  Returns an error if the element exists in some geometries but not others.
 * @param elementsExist The vector of optional DataArraySpec instances.  If the optional does not have a value, then that data array does not exist for the input geometry at that index.
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

Result<bool> ValidateGeometryElementExists(const std::vector<bool>& elementsExist, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
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

Result<bool> ValidateGeometryElementExists(const std::vector<std::optional<DataArraySpec>>& elementsSpecs, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
{
  std::vector<bool> bools(elementsSpecs.size());
  std::transform(elementsSpecs.begin(), elementsSpecs.end(), bools.begin(), [](const std::optional<DataArraySpec>& opt) -> bool { return opt.has_value(); });
  return ValidateGeometryElementExists(bools, inputGeometryPaths, arrayDescription);
}

Result<bool> ValidateGeometryElementArrayTypes(const std::vector<std::optional<DataArraySpec>>& elementsSpecs, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
{
  bool allSameArrayType = std::all_of(elementsSpecs.begin(), elementsSpecs.end(), [elementsSpecs](const std::optional<DataArraySpec>& spec) {
    if(spec.has_value())
    {
      return (spec.value().arrayType == elementsSpecs[0].value().arrayType);
    }
    return false;
  });
  return {allSameArrayType};
}

Result<bool> ValidateGeometryElementCompDimensions(const std::vector<std::optional<DataArraySpec>>& elementsSpecs, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
{
  bool allSameCompDims = std::all_of(elementsSpecs.begin(), elementsSpecs.end(), [elementsSpecs](const std::optional<DataArraySpec>& spec) {
    if(spec.has_value())
    {
      return (spec.value().compDims == elementsSpecs[0].value().compDims);
    }
    return false;
  });
  return {allSameCompDims};
}

Result<bool> ValidateGeometryElementDataTypes(const std::vector<std::optional<DataArraySpec>>& elementsSpecs, const std::vector<DataPath>& inputGeometryPaths, const std::string& arrayDescription)
{
  bool allSameDataType = std::all_of(elementsSpecs.begin(), elementsSpecs.end(), [elementsSpecs](const std::optional<DataArraySpec>& spec) {
    if(spec.has_value())
    {
      return (spec.value().dataType == elementsSpecs[0].value().dataType);
    }
    return false;
  });
  return {allSameDataType};
}

Result<> AddSpecActions(const DataArraySpec& spec, const DataPath& outputGeomPath, const std::string attrMatrixName, OutputActions& actions)
{
  switch(spec.arrayType)
  {
  case IArray::ArrayType::DataArray: {
    actions.appendAction(std::make_unique<CreateArrayAction>(spec.dataType.value(), std::vector<usize>{1}, spec.compDims, outputGeomPath.createChildPath(attrMatrixName).createChildPath(spec.name)));
    break;
  }
  case IArray::ArrayType::StringArray: {
    actions.appendAction(std::make_unique<CreateStringArrayAction>(std::vector<usize>{1}, outputGeomPath.createChildPath(attrMatrixName).createChildPath(spec.name)));
    break;
  }
  case IArray::ArrayType::NeighborListArray: {
    actions.appendAction(std::make_unique<CreateNeighborListAction>(spec.dataType.value(), 1, outputGeomPath.createChildPath(attrMatrixName).createChildPath(spec.name)));
    break;
  }
  case IArray::ArrayType::Any: {
    return MakeErrorResult(
        -56, fmt::format("Geometry at path '{}' has array with name '{}' that has array type 'Any'.  This should NEVER happen.  Please contact the developers.", spec.name, outputGeomPath.toString()));
  }
  }

  return {};
}

template <class NodeGeom>
Result<> CreateOtherAttrMatricesAndArrays(const DataPath& outputGeomPath, const std::vector<DataArraySpec>& vertexDataArraySpecs, bool edgesArrayExists, bool edgeAttrMatrixExists,
                                          const std::vector<DataArraySpec>& edgeDataArraySpecs, bool facesArrayExists, bool faceAttrMatrixExists, const std::vector<DataArraySpec>& faceDataArraySpecs,
                                          const std::vector<DataArraySpec>& polyDataArraySpecs, OutputActions& actions)
{
  if constexpr(std::is_base_of_v<INodeGeometry0D, NodeGeom>)
  {
    for(const auto& vertexDataArraySpec : vertexDataArraySpecs)
    {
      auto result = AddSpecActions(vertexDataArraySpec, outputGeomPath, INodeGeometry0D::k_VertexAttributeMatrixName, actions);
      if(result.invalid())
      {
        return result;
      }
    }
  }
  if constexpr(std::is_base_of_v<INodeGeometry1D, NodeGeom>)
  {
    for(const auto& edgeDataArraySpec : edgeDataArraySpecs)
    {
      auto result = AddSpecActions(edgeDataArraySpec, outputGeomPath, INodeGeometry1D::k_EdgeAttributeMatrixName, actions);
      if(result.invalid())
      {
        return result;
      }
    }
  }
  if constexpr(std::is_base_of_v<INodeGeometry2D, NodeGeom>)
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

    for(const auto& faceDataArraySpec : faceDataArraySpecs)
    {
      auto result = AddSpecActions(faceDataArraySpec, outputGeomPath, INodeGeometry2D::k_FaceAttributeMatrixName, actions);
      if(result.invalid())
      {
        return result;
      }
    }
  }
  if constexpr(std::is_base_of_v<INodeGeometry3D, NodeGeom>)
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

    for(const auto& polyDataArraySpec : polyDataArraySpecs)
    {
      auto result = AddSpecActions(polyDataArraySpec, outputGeomPath, INodeGeometry3D::k_PolyhedronDataName, actions);
      if(result.invalid())
      {
        return result;
      }
    }
  }

  return {};
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

  std::optional<IGeometry::Type> geometryTypeOpt;
  std::vector<bool> vertexArraysExist(inputGeometryPaths.size());
  std::vector<bool> vertexAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<DataArraySpec>>> vertexDataArraysExistMap;
  std::vector<bool> edgeArraysExist(inputGeometryPaths.size());
  std::vector<bool> edgeAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<DataArraySpec>>> edgeDataArraysExistMap;
  std::vector<bool> faceArraysExist(inputGeometryPaths.size());
  std::vector<bool> faceAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<DataArraySpec>>> faceDataArraysExistMap;
  std::vector<bool> polyArraysExist(inputGeometryPaths.size());
  std::vector<bool> polyAttrMatricesExist(inputGeometryPaths.size());
  std::map<std::string, std::vector<std::optional<DataArraySpec>>> polyDataArraysExistMap;
  for(usize i = 0; i < inputGeometryPaths.size(); ++i)
  {
    const auto& inputGeometryPath = inputGeometryPaths[i];
    const auto* iGeomPtr = dataStructure.getDataAs<IGeometry>(inputGeometryPath);
    if(iGeomPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(
          to_underlying(CombineNodeBasedGeometries::ErrorCodes::ObjectNotAGeometry),
          fmt::format("The data object at data path '{}' is not a geometry.  All data objects MUST be geometries with the same geometry type.", inputGeometryPath.toString()))};
    }
    auto& iGeom = *iGeomPtr;

    if(!geometryTypeOpt.has_value())
    {
      geometryTypeOpt = iGeom.getGeomType();
    }

    if(iGeom.getGeomType() != geometryTypeOpt.value())
    {
      return {MakeErrorResult<OutputActions>(
          to_underlying(CombineNodeBasedGeometries::ErrorCodes::DifferingGeometryTypes),
          fmt::format("The geometry at data path '{}' has geometry type '{}', which differs from other geometries that have geometry type '{}'.  All geometries MUST have the same geometry type.",
                      inputGeometryPath.toString(), GeometryTypeToString(iGeom.getGeomType()), GeometryTypeToString(geometryTypeOpt.value())))};
    }

    auto [vertexArrayExists, vertexAttrMatrixExists, vertexDataArraysSpec] = FindVertexElements(iGeomPtr);
    vertexArraysExist[i] = vertexArrayExists;
    vertexAttrMatricesExist[i] = vertexAttrMatrixExists;
    for(const auto& daSpec : vertexDataArraysSpec)
    {
      const std::string vertexDataArrayName = daSpec.name;
      auto& specs = vertexDataArraysExistMap[vertexDataArrayName];
      if(specs.empty())
      {
        specs.resize(inputGeometryPaths.size());
      }
      specs[i] = daSpec;
    }

    auto [edgeArrayExists, edgeAttrMatrixExists, edgeDataArraysSpec] = FindEdgeElements(iGeomPtr);
    edgeArraysExist[i] = edgeArrayExists;
    edgeAttrMatricesExist[i] = edgeAttrMatrixExists;
    for(const auto& daSpec : edgeDataArraysSpec)
    {
      const std::string edgeDataArrayName = daSpec.name;
      auto& specs = edgeDataArraysExistMap[edgeDataArrayName];
      if(specs.empty())
      {
        specs.resize(inputGeometryPaths.size());
      }
      specs[i] = daSpec;
    }

    auto [faceArrayExists, faceAttrMatrixExists, faceDataArraysSpec] = FindFaceElements(iGeomPtr);
    faceArraysExist[i] = faceArrayExists;
    faceAttrMatricesExist[i] = faceAttrMatrixExists;
    for(const auto& daSpec : faceDataArraysSpec)
    {
      const std::string faceDataArrayName = daSpec.name;
      auto& specs = faceDataArraysExistMap[faceDataArrayName];
      if(specs.empty())
      {
        specs.resize(inputGeometryPaths.size());
      }
      specs[i] = daSpec;
    }

    auto [polyArrayExists, polyAttrMatrixExists, polyDataArraysSpec] = FindPolyElements(iGeomPtr);
    polyArraysExist[i] = polyArrayExists;
    polyAttrMatricesExist[i] = polyAttrMatrixExists;
    for(const auto& daSpec : polyDataArraysSpec)
    {
      const std::string polyDataArrayName = daSpec.name;
      auto& specs = polyDataArraysExistMap[polyDataArrayName];
      if(specs.empty())
      {
        specs.resize(inputGeometryPaths.size());
      }
      specs[i] = daSpec;
    }
  }

  auto result = ValidateGeometryElementExists(vertexArraysExist, inputGeometryPaths, "a vertices array");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }
  bool vertexArrayExists = result.value();

  result = ValidateGeometryElementExists(vertexAttrMatricesExist, inputGeometryPaths, "a vertex attribute matrix");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }
  bool vertexAttrMatrixExists = result.value();

  std::vector<DataArraySpec> vertexDataArraySpecs;
  for(const auto& [arrayName, arrayExists] : vertexDataArraysExistMap)
  {
    std::string arrayDesc = fmt::format("vertex data array '{}'", arrayName);
    result = ValidateGeometryElementExists(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementArrayTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementCompDimensions(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementDataTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    vertexDataArraySpecs.push_back(arrayExists[0].value());
  }

  result = ValidateGeometryElementExists(edgeArraysExist, inputGeometryPaths, "an edges array");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }
  bool edgesArrayExists = result.value();

  result = ValidateGeometryElementExists(edgeAttrMatricesExist, inputGeometryPaths, "an edge attribute matrix");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }
  bool edgeAttrMatrixExists = result.value();

  std::vector<DataArraySpec> edgeDataArraySpecs;
  for(const auto& [arrayName, arrayExists] : edgeDataArraysExistMap)
  {
    std::string arrayDesc = fmt::format("edge data array '{}'", arrayName);
    result = ValidateGeometryElementExists(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementArrayTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementCompDimensions(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementDataTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    edgeDataArraySpecs.push_back(arrayExists[0].value());
  }

  result = ValidateGeometryElementExists(faceArraysExist, inputGeometryPaths, "a faces array");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }
  bool facesArrayExists = result.value();

  result = ValidateGeometryElementExists(faceAttrMatricesExist, inputGeometryPaths, "a face attribute matrix");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }
  bool faceAttrMatrixExists = result.value();

  std::vector<DataArraySpec> faceDataArraySpecs;
  for(const auto& [arrayName, arrayExists] : faceDataArraysExistMap)
  {
    std::string arrayDesc = fmt::format("face data array '{}'", arrayName);
    result = ValidateGeometryElementExists(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementArrayTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementCompDimensions(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementDataTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    faceDataArraySpecs.push_back(arrayExists[0].value());
  }

  result = ValidateGeometryElementExists(polyArraysExist, inputGeometryPaths, "a polyhedra array");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }

  result = ValidateGeometryElementExists(polyAttrMatricesExist, inputGeometryPaths, "a polyhedra attribute matrix");
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }

  std::vector<DataArraySpec> polyDataArraySpecs;
  for(const auto& [arrayName, arrayExists] : polyDataArraysExistMap)
  {
    std::string arrayDesc = fmt::format("polyhedra data array '{}'", arrayName);
    result = ValidateGeometryElementExists(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementArrayTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementCompDimensions(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    result = ValidateGeometryElementDataTypes(arrayExists, inputGeometryPaths, arrayDesc);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
    }
    polyDataArraySpecs.push_back(arrayExists[0].value());
  }

  OutputActions actions;
  IGeometry::Type geometryType = geometryTypeOpt.value();
  switch(geometryType)
  {
  case IGeometry::Type::Vertex: {
    actions.appendAction(std::make_unique<CreateVertexGeometryAction>(outputGeometryPath, 1, VertexGeom::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<VertexGeom>(outputGeometryPath, vertexDataArraySpecs, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraySpecs, facesArrayExists,
                                                                       faceAttrMatrixExists, faceDataArraySpecs, polyDataArraySpecs, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Edge: {
    actions.appendAction(std::make_unique<CreateEdgeGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, EdgeGeom::k_EdgeAttributeMatrixName,
                                                                    VertexGeom::k_SharedVertexListName, EdgeGeom::k_SharedEdgeListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<EdgeGeom>(outputGeometryPath, vertexDataArraySpecs, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraySpecs, facesArrayExists,
                                                                     faceAttrMatrixExists, faceDataArraySpecs, polyDataArraySpecs, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Triangle: {
    actions.appendAction(std::make_unique<CreateTriangleGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, TriangleGeom::k_FaceAttributeMatrixName,
                                                                        VertexGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<TriangleGeom>(outputGeometryPath, vertexDataArraySpecs, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraySpecs, facesArrayExists,
                                                                         faceAttrMatrixExists, faceDataArraySpecs, polyDataArraySpecs, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Quad: {
    actions.appendAction(std::make_unique<CreateQuadGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, QuadGeom::k_FaceAttributeMatrixName,
                                                                    VertexGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<QuadGeom>(outputGeometryPath, vertexDataArraySpecs, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraySpecs, facesArrayExists,
                                                                     faceAttrMatrixExists, faceDataArraySpecs, polyDataArraySpecs, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Tetrahedral: {
    actions.appendAction(std::make_unique<CreateTetrahedralGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, TetrahedralGeom::k_PolyhedronDataName,
                                                                           VertexGeom::k_SharedVertexListName, TetrahedralGeom::k_SharedPolyhedronListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<TetrahedralGeom>(outputGeometryPath, vertexDataArraySpecs, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraySpecs, facesArrayExists,
                                                                            faceAttrMatrixExists, faceDataArraySpecs, polyDataArraySpecs, actions);
    if(creationResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(creationResult), {})};
    }
    break;
  }
  case IGeometry::Type::Hexahedral: {
    actions.appendAction(std::make_unique<CreateHexahedralGeometryAction>(outputGeometryPath, 1, 1, VertexGeom::k_VertexAttributeMatrixName, HexahedralGeom::k_PolyhedronDataName,
                                                                          VertexGeom::k_SharedVertexListName, HexahedralGeom::k_SharedPolyhedronListName));
    auto creationResult = CreateOtherAttrMatricesAndArrays<HexahedralGeom>(outputGeometryPath, vertexDataArraySpecs, edgesArrayExists, edgeAttrMatrixExists, edgeDataArraySpecs, facesArrayExists,
                                                                           faceAttrMatrixExists, faceDataArraySpecs, polyDataArraySpecs, actions);
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
