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
namespace Keys
{
constexpr StringLiteral k_DataType = "data_type";
constexpr StringLiteral k_TupleShape = "tuple_shape";
constexpr StringLiteral k_ComponentShape = "component_shape";
constexpr StringLiteral k_NumTuples = "num_tuples";
constexpr StringLiteral k_NumComponents = "num_components";
constexpr StringLiteral k_StoreType = "store_type";
constexpr StringLiteral k_Dimensions = "dimensions";
constexpr StringLiteral k_CellDataPath = "cell_data_path";
constexpr StringLiteral k_Origin = "origin";
constexpr StringLiteral k_Spacing = "spacing";
constexpr StringLiteral k_NumVertices = "num_vertices";
constexpr StringLiteral k_VertexDataPath = "vertex_data_path";
constexpr StringLiteral k_NumEdges = "num_edges";
constexpr StringLiteral k_EdgeDataPath = "edge_data_path";
constexpr StringLiteral k_NumFaces = "num_faces";
constexpr StringLiteral k_FaceDataPath = "face_data_path";
constexpr StringLiteral k_NumPolyhedra = "num_polyhedra";
constexpr StringLiteral k_PolyhedronDataPath = "polyhedron_data_path";
constexpr StringLiteral k_GeometryType = "geometry_type";
constexpr StringLiteral k_UnitDimensionality = "unit_dimensionality";
constexpr StringLiteral k_LengthUnits = "length_units";
constexpr StringLiteral k_NumCells = "num_cells";
constexpr StringLiteral k_Geometry = "geometry";
constexpr StringLiteral k_Name = "name";
constexpr StringLiteral k_Path = "path";
constexpr StringLiteral k_Id = "id";
constexpr StringLiteral k_Type = "type";
constexpr StringLiteral k_Children = "children";
constexpr StringLiteral k_SchemaVersion = "schema_version";
constexpr StringLiteral k_Objects = "objects";
} // namespace Keys

constexpr int32 k_HierarchyJsonSchemaVersion = 1;

/**
 * @brief Converts shape dimensions to a JSON array.
 * @param shape Shape dimensions to convert.
 * @return JSON array that contains the dimensions in their original order.
 */
nlohmann::json shapeToJson(const std::vector<usize>& shape)
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
std::string_view storeTypeToString(IDataStore::StoreType storeType)
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
void appendDataArrayFields(nlohmann::json& node, const IDataArray& dataArray)
{
  node[Keys::k_DataType.view()] = std::string(DataTypeToString(dataArray.getDataType()).view());
  node[Keys::k_TupleShape.view()] = shapeToJson(dataArray.getTupleShape());
  node[Keys::k_ComponentShape.view()] = shapeToJson(dataArray.getComponentShape());
  node[Keys::k_NumTuples.view()] = dataArray.getNumberOfTuples();
  node[Keys::k_NumComponents.view()] = dataArray.getNumberOfComponents();
  node[Keys::k_StoreType.view()] = storeTypeToString(dataArray.getIDataStoreRef().getStoreType());
}

/**
 * @brief Adds string-array metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param stringArray Array to describe.
 */
void appendStringArrayFields(nlohmann::json& node, const StringArray& stringArray)
{
  node[Keys::k_DataType.view()] = "string";
  node[Keys::k_NumTuples.view()] = stringArray.getNumberOfTuples();
}

/**
 * @brief Adds neighbor-list metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param neighborList Neighbor list to describe.
 */
void appendNeighborListFields(nlohmann::json& node, const INeighborList& neighborList)
{
  node[Keys::k_DataType.view()] = std::string(DataTypeToString(neighborList.getDataType()).view());
  node[Keys::k_NumTuples.view()] = neighborList.getNumberOfTuples();
}

/**
 * @brief Adds attribute-matrix metadata to a JSON object.
 * @param node JSON object that receives the metadata.
 * @param attributeMatrix Attribute matrix to describe.
 */
void appendAttributeMatrixFields(nlohmann::json& node, const AttributeMatrix& attributeMatrix)
{
  node[Keys::k_TupleShape.view()] = shapeToJson(attributeMatrix.getShape());
}

/**
 * @brief Converts a three-component vector to a JSON array.
 * @tparam T Vector component type.
 * @param vector Vector to convert.
 * @return JSON array that contains the vector in X, Y, Z order.
 */
template <typename T>
nlohmann::json vec3ToJson(const Vec3<T>& vector)
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
bool hasReadableBounds(const RectGridGeom& rectGridGeom)
{
  const auto* xBoundsPtr = rectGridGeom.getXBounds();
  const auto* yBoundsPtr = rectGridGeom.getYBounds();
  const auto* zBoundsPtr = rectGridGeom.getZBounds();

  const auto hasReadableStore = [](const Float32Array* boundsPtr) {
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
void appendGridGeometryFields(nlohmann::json& geometryNode, const IGridGeometry& gridGeometry)
{
  geometryNode[Keys::k_Dimensions.view()] = vec3ToJson(gridGeometry.getDimensions());
  if(gridGeometry.getCellDataId().has_value())
  {
    geometryNode[Keys::k_CellDataPath.view()] = gridGeometry.getCellDataPath().toString();
  }

  if(const auto* imageGeomPtr = dynamic_cast<const ImageGeom*>(&gridGeometry); imageGeomPtr != nullptr)
  {
    geometryNode[Keys::k_Origin.view()] = vec3ToJson(imageGeomPtr->getOrigin());
    geometryNode[Keys::k_Spacing.view()] = vec3ToJson(imageGeomPtr->getSpacing());
  }
  else if(const auto* rectGridGeomPtr = dynamic_cast<const RectGridGeom*>(&gridGeometry); rectGridGeomPtr != nullptr && hasReadableBounds(*rectGridGeomPtr))
  {
    const Result<FloatVec3> originResult = rectGridGeomPtr->getOrigin();
    if(originResult.valid())
    {
      geometryNode[Keys::k_Origin.view()] = vec3ToJson(originResult.value());
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
void appendNodeGeometryFields(nlohmann::json& geometryNode, const INodeGeometry0D& nodeGeometry)
{
  geometryNode[Keys::k_NumVertices.view()] = nodeGeometry.getNumberOfVertices();
  if(nodeGeometry.getVertexAttributeMatrixId().has_value())
  {
    geometryNode[Keys::k_VertexDataPath.view()] = nodeGeometry.getVertexAttributeMatrixDataPath().toString();
  }

  if(const auto* geom1DPtr = dynamic_cast<const INodeGeometry1D*>(&nodeGeometry); geom1DPtr != nullptr)
  {
    geometryNode[Keys::k_NumEdges.view()] = geom1DPtr->getNumberOfEdges();
    if(geom1DPtr->getEdgeAttributeMatrixId().has_value())
    {
      geometryNode[Keys::k_EdgeDataPath.view()] = geom1DPtr->getEdgeAttributeMatrixDataPath().toString();
    }
  }

  if(const auto* geom2DPtr = dynamic_cast<const INodeGeometry2D*>(&nodeGeometry); geom2DPtr != nullptr)
  {
    geometryNode[Keys::k_NumFaces.view()] = geom2DPtr->getNumberOfFaces();
    if(geom2DPtr->getFaceAttributeMatrixId().has_value())
    {
      geometryNode[Keys::k_FaceDataPath.view()] = geom2DPtr->getFaceAttributeMatrixDataPath().toString();
    }
  }

  if(const auto* geom3DPtr = dynamic_cast<const INodeGeometry3D*>(&nodeGeometry); geom3DPtr != nullptr)
  {
    geometryNode[Keys::k_NumPolyhedra.view()] = geom3DPtr->getNumberOfPolyhedra();
    if(geom3DPtr->getPolyhedraAttributeMatrixId().has_value())
    {
      geometryNode[Keys::k_PolyhedronDataPath.view()] = geom3DPtr->getPolyhedronAttributeMatrixDataPath().toString();
    }
  }
}

/**
 * @brief Adds geometry metadata to a hierarchy node.
 * @param node JSON object that receives the geometry block.
 * @param geometry Geometry to describe.
 */
void appendGeometryFields(nlohmann::json& node, const IGeometry& geometry)
{
  nlohmann::json geometryNode;
  geometryNode[Keys::k_GeometryType.view()] = IGeometry::GeomTypeToString(geometry.getGeomType());
  geometryNode[Keys::k_UnitDimensionality.view()] = geometry.getUnitDimensionality();
  geometryNode[Keys::k_LengthUnits.view()] = IGeometry::LengthUnitToString(geometry.getUnits());
  geometryNode[Keys::k_NumCells.view()] = geometry.getNumberOfCells();

  if(const auto* gridGeometryPtr = dynamic_cast<const IGridGeometry*>(&geometry); gridGeometryPtr != nullptr)
  {
    appendGridGeometryFields(geometryNode, *gridGeometryPtr);
  }
  else if(const auto* nodeGeometryPtr = dynamic_cast<const INodeGeometry0D*>(&geometry); nodeGeometryPtr != nullptr)
  {
    appendNodeGeometryFields(geometryNode, *nodeGeometryPtr);
  }

  node[Keys::k_Geometry.view()] = std::move(geometryNode);
}

/**
 * @brief Creates one hierarchy node without its children.
 * @param object Data object to describe.
 * @param path Path of the object in the data structure.
 * @return JSON object that contains common and type-specific metadata.
 */
nlohmann::json makeObjectNode(const DataObject& object, const DataPath& path)
{
  nlohmann::json node;
  node[Keys::k_Name.view()] = object.getName();
  node[Keys::k_Path.view()] = path.toString();
  node[Keys::k_Id.view()] = object.getId();
  node[Keys::k_Type.view()] = object.getTypeName();

  if(const auto* geometryPtr = dynamic_cast<const IGeometry*>(&object); geometryPtr != nullptr)
  {
    appendGeometryFields(node, *geometryPtr);
  }
  else if(const auto* attributeMatrixPtr = dynamic_cast<const AttributeMatrix*>(&object); attributeMatrixPtr != nullptr)
  {
    appendAttributeMatrixFields(node, *attributeMatrixPtr);
  }
  else if(const auto* dataArrayPtr = dynamic_cast<const IDataArray*>(&object); dataArrayPtr != nullptr)
  {
    appendDataArrayFields(node, *dataArrayPtr);
  }
  else if(const auto* stringArrayPtr = dynamic_cast<const StringArray*>(&object); stringArrayPtr != nullptr)
  {
    appendStringArrayFields(node, *stringArrayPtr);
  }
  else if(const auto* neighborListPtr = dynamic_cast<const INeighborList*>(&object); neighborListPtr != nullptr)
  {
    appendNeighborListFields(node, *neighborListPtr);
  }

  node[Keys::k_Children.view()] = nlohmann::json::array();
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
nlohmann::json makeHierarchyNode(const DataObject& object, const DataPath& path)
{
  nlohmann::json hierarchyNode = makeObjectNode(object, path);
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

    const std::vector<const DataObject*> sortedChildPtrs = DataStructureExportUtilities::GetSortedObjectPointers(groupPtr->getDataMap());
    auto& children = pendingNode.getJson().at(Keys::k_Children.view()).get_ref<nlohmann::json::array_t&>();
    // Reserve before pendingNodes stores pointers to child JSON values.
    children.reserve(sortedChildPtrs.size());
    for(const DataObject* childPtr : sortedChildPtrs)
    {
      DataPath childPath = pendingNode.getPath().createChildPath(childPtr->getName());
      children.emplace_back(makeObjectNode(*childPtr, childPath));
      pendingNodes.emplace_back(childPtr, std::move(childPath), &children.back());
    }
  }
  return hierarchyNode;
}
} // namespace

nlohmann::json DataStructure::exportHierarchyAsJson() const
{
  nlohmann::json root;
  root[Keys::k_SchemaVersion.view()] = k_HierarchyJsonSchemaVersion;
  root[Keys::k_Objects.view()] = nlohmann::json::array();

  auto& objects = root[Keys::k_Objects.view()].get_ref<nlohmann::json::array_t&>();
  const std::vector<const DataObject*> rootObjectPtrs = DataStructureExportUtilities::GetSortedObjectPointers(m_RootGroup);
  objects.reserve(rootObjectPtrs.size());
  for(const DataObject* objectPtr : rootObjectPtrs)
  {
    objects.emplace_back(makeHierarchyNode(*objectPtr, DataPath({objectPtr->getName()})));
  }
  return root;
}
} // namespace nx::core
