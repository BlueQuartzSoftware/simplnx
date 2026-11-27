/**
 * @file DataStructureJson.cpp
 * @brief Implements JSON hierarchy export for DataStructure.
 */

#include "DataStructure.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructureExportUtilities.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nx::core
{
namespace
{
/**
 * @namespace Keys
 * @brief Defines object keys for the hierarchy JSON schema.
 */
namespace keys
{
constexpr StringLiteralType k_DataType = "data_type";
constexpr StringLiteralType k_TupleShape = "tuple_shape";
constexpr StringLiteralType k_ComponentShape = "component_shape";
constexpr StringLiteralType k_NumTuples = "num_tuples";
constexpr StringLiteralType k_NumComponents = "num_components";
constexpr StringLiteralType k_StoreType = "store_type";
constexpr StringLiteralType k_Dimensions = "dimensions";
constexpr StringLiteralType k_CellDataPath = "cell_data_path";
constexpr StringLiteralType k_Origin = "origin";
constexpr StringLiteralType k_Spacing = "spacing";
constexpr StringLiteralType k_NumVertices = "num_vertices";
constexpr StringLiteralType k_VertexDataPath = "vertex_data_path";
constexpr StringLiteralType k_NumEdges = "num_edges";
constexpr StringLiteralType k_EdgeDataPath = "edge_data_path";
constexpr StringLiteralType k_NumFaces = "num_faces";
constexpr StringLiteralType k_FaceDataPath = "face_data_path";
constexpr StringLiteralType k_NumPolyhedra = "num_polyhedra";
constexpr StringLiteralType k_PolyhedronDataPath = "polyhedron_data_path";
constexpr StringLiteralType k_GeometryType = "geometry_type";
constexpr StringLiteralType k_UnitDimensionality = "unit_dimensionality";
constexpr StringLiteralType k_LengthUnits = "length_units";
constexpr StringLiteralType k_NumCells = "num_cells";
constexpr StringLiteralType k_Geometry = "geometry";
constexpr StringLiteralType k_Name = "name";
constexpr StringLiteralType k_Path = "path";
constexpr StringLiteralType k_Id = "id";
constexpr StringLiteralType k_Type = "type";
constexpr StringLiteralType k_Children = "children";
constexpr StringLiteralType k_SchemaVersion = "schema_version";
constexpr StringLiteralType k_Objects = "objects";
} // namespace keys

constexpr int32 k_HierarchyJsonSchemaVersion = 1;

/**
 * @brief Converts shape dimensions to a JSON array.
 * @param shape Shape dimensions to convert.
 * @return JSON array that contains the dimensions in their original order.
 */
nlohmann::json ShapeToJson(const std::vector<usize>& shape)
{
  nlohmann::json::array_t values;
  values.reserve(shape.size());
  for(const usize dimension : shape)
  {
    values.emplace_back(dimension);
  }
  return values;
}

/**
 * @brief Gets the readable name of a data-store type.
 * @param storeType Data-store type to describe.
 * @return Static name of the data-store type.
 */
std::string_view StoreTypeToString(IDataStore::StoreType storeType)
{
  switch(storeType)
  {
  case IDataStore::StoreType::InMemory:
    return "InMemory";
  case IDataStore::StoreType::OutOfCore:
    return "OutOfCore";
  case IDataStore::StoreType::Empty:
    return "Empty";
  }
  return "Unknown";
}

/**
 * @brief Adds numeric-array metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param dataArray Array to describe.
 */
void AppendDataArrayFields(nlohmann::json& node, const IDataArray& dataArray)
{
  node[keys::k_DataType.view()] = std::string(DataTypeToString(dataArray.getDataType()).view());
  node[keys::k_TupleShape.view()] = ShapeToJson(dataArray.getTupleShape());
  node[keys::k_ComponentShape.view()] = ShapeToJson(dataArray.getComponentShape());
  node[keys::k_NumTuples.view()] = dataArray.getNumberOfTuples();
  node[keys::k_NumComponents.view()] = dataArray.getNumberOfComponents();
  node[keys::k_StoreType.view()] = StoreTypeToString(dataArray.getIDataStoreRef().getStoreType());
}

/**
 * @brief Adds string-array metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param stringArray Array to describe.
 */
void AppendStringArrayFields(nlohmann::json& node, const StringArray& stringArray)
{
  node[keys::k_DataType.view()] = "string";
  node[keys::k_NumTuples.view()] = stringArray.getNumberOfTuples();
}

/**
 * @brief Adds neighbor-list metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param neighborList Neighbor list to describe.
 */
void AppendNeighborListFields(nlohmann::json& node, const INeighborList& neighborList)
{
  node[keys::k_DataType.view()] = std::string(DataTypeToString(neighborList.getDataType()).view());
  node[keys::k_NumTuples.view()] = neighborList.getNumberOfTuples();
}

/**
 * @brief Adds attribute-matrix metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param attributeMatrix Attribute matrix to describe.
 */
void AppendAttributeMatrixFields(nlohmann::json& node, const AttributeMatrix& attributeMatrix)
{
  node[keys::k_TupleShape.view()] = ShapeToJson(attributeMatrix.getShape());
}

/**
 * @brief Converts a three-component vector to a JSON array.
 * @tparam T Vector component type.
 * @param vector Vector to convert.
 * @return JSON array that contains the vector in X, Y, Z order.
 */
template <typename T>
nlohmann::json Vec3ToJson(const Vec3<T>& vector)
{
  return nlohmann::json::array({vector[0], vector[1], vector[2]});
}

/**
 * @brief Checks if all bounds arrays contain readable values.
 * @param rectGridGeom Rectilinear grid geometry to inspect.
 * @return True if all bounds arrays have stores that permit value access.
 *
 * Preflight stores contain only metadata and do not permit value access.
 */
bool HasReadableBounds(const RectGridGeom& rectGridGeom)
{
  const auto* xBoundsPtr = rectGridGeom.getXBounds();
  const auto* yBoundsPtr = rectGridGeom.getYBounds();
  const auto* zBoundsPtr = rectGridGeom.getZBounds();

  const auto hasReadableStore = [](const Float32ArrayType* boundsPtr) {
    if(boundsPtr == nullptr)
    {
      return false;
    }

    const IDataStore::StoreType storeType = boundsPtr->getIDataStoreRef().getStoreType();
    return storeType != IDataStore::StoreType::Empty;
  };

  return hasReadableStore(xBoundsPtr) && hasReadableStore(yBoundsPtr) && hasReadableStore(zBoundsPtr);
}

/**
 * @brief Adds grid-geometry metadata to a JSON object.
 * @param geometryNode JSON object that receives the metadata.
 * @param gridGeometry Grid geometry to describe.
 *
 * A RectGrid origin is omitted when its bounds values are not readable.
 */
void AppendGridGeometryFields(nlohmann::json& geometryNode, const IGridGeometry& gridGeometry)
{
  geometryNode[keys::k_Dimensions.view()] = Vec3ToJson(gridGeometry.getDimensions());
  if(gridGeometry.getCellDataId().has_value())
  {
    geometryNode[keys::k_CellDataPath.view()] = gridGeometry.getCellDataPath().toString();
  }

  if(const auto* imageGeomPtr = dynamic_cast<const ImageGeom*>(&gridGeometry); imageGeomPtr != nullptr)
  {
    geometryNode[keys::k_Origin.view()] = Vec3ToJson(imageGeomPtr->getOrigin());
    geometryNode[keys::k_Spacing.view()] = Vec3ToJson(imageGeomPtr->getSpacing());
  }
  else if(const auto* rectGridGeomPtr = dynamic_cast<const RectGridGeom*>(&gridGeometry); rectGridGeomPtr != nullptr && HasReadableBounds(*rectGridGeomPtr))
  {
    const Result<FloatVec3Type> originResult = rectGridGeomPtr->getOrigin();
    if(originResult.valid())
    {
      geometryNode[keys::k_Origin.view()] = Vec3ToJson(originResult.value());
    }
  }
}

/**
 * @brief Adds node-geometry metadata to a JSON object.
 * @param geometryNode JSON object that receives the metadata.
 * @param nodeGeometry Node geometry to describe.
 *
 * Each dimensionality contributes its counts and assigned attribute-matrix paths.
 */
void AppendNodeGeometryFields(nlohmann::json& geometryNode, const INodeGeometry0D& nodeGeometry)
{
  geometryNode[keys::k_NumVertices.view()] = nodeGeometry.getNumberOfVertices();
  if(nodeGeometry.getVertexAttributeMatrixId().has_value())
  {
    geometryNode[keys::k_VertexDataPath.view()] = nodeGeometry.getVertexAttributeMatrixDataPath().toString();
  }

  if(const auto* geom1DPtr = dynamic_cast<const INodeGeometry1D*>(&nodeGeometry); geom1DPtr != nullptr)
  {
    geometryNode[keys::k_NumEdges.view()] = geom1DPtr->getNumberOfEdges();
    if(geom1DPtr->getEdgeAttributeMatrixId().has_value())
    {
      geometryNode[keys::k_EdgeDataPath.view()] = geom1DPtr->getEdgeAttributeMatrixDataPath().toString();
    }
  }

  if(const auto* geom2DPtr = dynamic_cast<const INodeGeometry2D*>(&nodeGeometry); geom2DPtr != nullptr)
  {
    geometryNode[keys::k_NumFaces.view()] = geom2DPtr->getNumberOfFaces();
    if(geom2DPtr->getFaceAttributeMatrixId().has_value())
    {
      geometryNode[keys::k_FaceDataPath.view()] = geom2DPtr->getFaceAttributeMatrixDataPath().toString();
    }
  }

  if(const auto* geom3DPtr = dynamic_cast<const INodeGeometry3D*>(&nodeGeometry); geom3DPtr != nullptr)
  {
    geometryNode[keys::k_NumPolyhedra.view()] = geom3DPtr->getNumberOfPolyhedra();
    if(geom3DPtr->getPolyhedraAttributeMatrixId().has_value())
    {
      geometryNode[keys::k_PolyhedronDataPath.view()] = geom3DPtr->getPolyhedronAttributeMatrixDataPath().toString();
    }
  }
}

/**
 * @brief Adds geometry metadata to a hierarchy node.
 * @param node JSON object that receives the geometry block.
 * @param geometry Geometry to describe.
 */
void AppendGeometryFields(nlohmann::json& node, const IGeometry& geometry)
{
  nlohmann::json geometryNode;
  geometryNode[keys::k_GeometryType.view()] = IGeometry::GeomTypeToString(geometry.getGeomType());
  geometryNode[keys::k_UnitDimensionality.view()] = geometry.getUnitDimensionality();
  geometryNode[keys::k_LengthUnits.view()] = IGeometry::LengthUnitToString(geometry.getUnits());
  geometryNode[keys::k_NumCells.view()] = geometry.getNumberOfCells();

  if(const auto* gridGeometryPtr = dynamic_cast<const IGridGeometry*>(&geometry); gridGeometryPtr != nullptr)
  {
    AppendGridGeometryFields(geometryNode, *gridGeometryPtr);
  }
  else if(const auto* nodeGeometryPtr = dynamic_cast<const INodeGeometry0D*>(&geometry); nodeGeometryPtr != nullptr)
  {
    AppendNodeGeometryFields(geometryNode, *nodeGeometryPtr);
  }

  node[keys::k_Geometry.view()] = std::move(geometryNode);
}

/**
 * @brief Creates one hierarchy node without its children.
 * @param object Data object to describe.
 * @param path Path of the object in the data structure.
 * @return JSON object that contains common and type-specific metadata.
 */
nlohmann::json MakeObjectNode(const DataObject& object, const DataPath& path)
{
  nlohmann::json node;
  node[keys::k_Name.view()] = object.getName();
  node[keys::k_Path.view()] = path.toString();
  node[keys::k_Id.view()] = object.getId();
  node[keys::k_Type.view()] = object.getTypeName();

  if(const auto* geometryPtr = dynamic_cast<const IGeometry*>(&object); geometryPtr != nullptr)
  {
    AppendGeometryFields(node, *geometryPtr);
  }
  else if(const auto* attributeMatrixPtr = dynamic_cast<const AttributeMatrix*>(&object); attributeMatrixPtr != nullptr)
  {
    AppendAttributeMatrixFields(node, *attributeMatrixPtr);
  }
  else if(const auto* dataArrayPtr = dynamic_cast<const IDataArray*>(&object); dataArrayPtr != nullptr)
  {
    AppendDataArrayFields(node, *dataArrayPtr);
  }
  else if(const auto* stringArrayPtr = dynamic_cast<const StringArray*>(&object); stringArrayPtr != nullptr)
  {
    AppendStringArrayFields(node, *stringArrayPtr);
  }
  else if(const auto* neighborListPtr = dynamic_cast<const INeighborList*>(&object); neighborListPtr != nullptr)
  {
    AppendNeighborListFields(node, *neighborListPtr);
  }

  node[keys::k_Children.view()] = nlohmann::json::array();
  return node;
}

/**
 * @class PendingNode
 * @brief Stores one iterative hierarchy-expansion operation.
 */
class PendingNode
{
public:
  /**
   * @brief Creates one pending hierarchy node.
   * @param objectPtr Data object represented by the JSON node.
   * @param path Path of the data object.
   * @param jsonPtr JSON node that receives child nodes.
   */
  PendingNode(const DataObject* objectPtr, DataPath path, nlohmann::json* jsonPtr)
  : m_ObjectPtr(objectPtr)
  , m_Path(std::move(path))
  , m_JsonPtr(jsonPtr)
  {
  }

  /**
   * @brief Gets the data object represented by this pending node.
   * @return Non-owning pointer to the data object.
   */
  [[nodiscard]] const DataObject* getObjectPtr() const
  {
    return m_ObjectPtr;
  }

  /**
   * @brief Gets the hierarchy path of the data object.
   * @return Data object path.
   */
  [[nodiscard]] const DataPath& getPath() const
  {
    return m_Path;
  }

  /**
   * @brief Gets the JSON object that receives descendants.
   * @return Mutable JSON hierarchy node.
   */
  [[nodiscard]] nlohmann::json& getJson() const
  {
    return *m_JsonPtr;
  }

private:
  const DataObject* m_ObjectPtr = nullptr;
  DataPath m_Path;
  nlohmann::json* m_JsonPtr = nullptr;
};

/**
 * @brief Creates one hierarchy node and all descendant nodes.
 * @param object Data object to describe.
 * @param path Path of the object in the data structure.
 * @return JSON object that contains the complete hierarchy branch.
 */
nlohmann::json MakeHierarchyNode(const DataObject& object, const DataPath& path)
{
  nlohmann::json hierarchyNode = MakeObjectNode(object, path);
  std::vector<PendingNode> pendingNodes;
  pendingNodes.emplace_back(&object, path, &hierarchyNode);

  while(!pendingNodes.empty())
  {
    const PendingNode pendingNode = std::move(pendingNodes.back());
    pendingNodes.pop_back();

    const auto* groupPtr = dynamic_cast<const BaseGroup*>(pendingNode.getObjectPtr());
    if(groupPtr == nullptr)
    {
      continue;
    }

    const std::vector<const DataObject*> sortedChildPtrs = data_structure_export_utilities::GetSortedObjectPointers(groupPtr->getDataMap());
    auto& children = pendingNode.getJson().at(keys::k_Children.view()).get_ref<nlohmann::json::array_t&>();
    // Reserve before pendingNodes stores pointers to child JSON values.
    children.reserve(sortedChildPtrs.size());
    for(const DataObject* childPtr : sortedChildPtrs)
    {
      DataPath childPath = pendingNode.getPath().createChildPath(childPtr->getName());
      children.emplace_back(MakeObjectNode(*childPtr, childPath));
      pendingNodes.emplace_back(childPtr, std::move(childPath), &children.back());
    }
  }
  return hierarchyNode;
}
} // namespace

nlohmann::json DataStructure::exportHierarchyAsJson() const
{
  nlohmann::json root;
  root[keys::k_SchemaVersion.view()] = k_HierarchyJsonSchemaVersion;
  root[keys::k_Objects.view()] = nlohmann::json::array();

  auto& objects = root[keys::k_Objects.view()].get_ref<nlohmann::json::array_t&>();
  const std::vector<const DataObject*> rootObjectPtrs = data_structure_export_utilities::GetSortedObjectPointers(m_RootGroup);
  objects.reserve(rootObjectPtrs.size());
  for(const DataObject* objectPtr : rootObjectPtrs)
  {
    objects.emplace_back(MakeHierarchyNode(*objectPtr, DataPath({objectPtr->getName()})));
  }
  return root;
}
} // namespace nx::core
