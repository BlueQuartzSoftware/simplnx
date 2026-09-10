#include "DataStructure.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataReparentedMessage.hpp"
#include "simplnx/DataStructure/Observers/AbstractDataStructureObserver.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <numeric>
#include <sstream>
#include <stdexcept>

namespace
{
const std::string k_Delimiter = "|--";

/**
 * @brief Returns the lazy process-wide storage-format resolver.
 * @return Mutable reference to the default resolver pointer.
 *
 * In-memory storage keeps DataStructure usable before application startup sets
 * the resolver.
 */
std::shared_ptr<const nx::core::IDataStoreFormatResolver>& DefaultFormatResolverRef()
{
  static std::shared_ptr<const nx::core::IDataStoreFormatResolver> s_Default = std::make_shared<nx::core::InMemoryFormatResolver>();
  return s_Default;
}
} // namespace

namespace nx::core
{
namespace
{
constexpr int32 k_HierarchyJsonSchemaVersion = 1;

/**
 * @brief Converts a shape vector to a JSON array of integers.
 */
nlohmann::json ShapeToJson(const std::vector<usize>& shape)
{
  nlohmann::json array = nlohmann::json::array();
  for(usize dim : shape)
  {
    array.push_back(dim);
  }
  return array;
}

/**
 * @brief Returns the human readable name of a data store type.
 */
std::string StoreTypeToString(IDataStore::StoreType storeType)
{
  switch(storeType)
  {
  case IDataStore::StoreType::InMemory:
    return "InMemory";
  case IDataStore::StoreType::OutOfCore:
    return "OutOfCore";
  case IDataStore::StoreType::Empty:
    return "Empty";
  case IDataStore::StoreType::EmptyOutOfCore:
    return "EmptyOutOfCore";
  }
  return "Unknown";
}

/**
 * @brief Adds the array-specific keys for an IDataArray.
 */
void AppendDataArrayFields(nlohmann::json& node, const IDataArray& dataArray)
{
  node["data_type"] = std::string(DataTypeToString(dataArray.getDataType()).view());
  node["tuple_shape"] = ShapeToJson(dataArray.getTupleShape());
  node["component_shape"] = ShapeToJson(dataArray.getComponentShape());
  node["num_tuples"] = dataArray.getNumberOfTuples();
  node["num_components"] = dataArray.getNumberOfComponents();
  node["store_type"] = StoreTypeToString(dataArray.getIDataStoreRef().getStoreType());
}

/**
 * @brief Adds the keys for a StringArray.
 */
void AppendStringArrayFields(nlohmann::json& node, const StringArray& stringArray)
{
  node["data_type"] = "string";
  node["num_tuples"] = stringArray.getNumberOfTuples();
}

/**
 * @brief Adds the keys for an INeighborList.
 */
void AppendNeighborListFields(nlohmann::json& node, const INeighborList& neighborList)
{
  node["data_type"] = std::string(DataTypeToString(neighborList.getDataType()).view());
  node["num_tuples"] = neighborList.getNumberOfTuples();
}

/**
 * @brief Adds the keys for an AttributeMatrix.
 */
void AppendAttributeMatrixFields(nlohmann::json& node, const AttributeMatrix& attributeMatrix)
{
  node["tuple_shape"] = ShapeToJson(attributeMatrix.getShape());
}

/**
 * @brief Converts a 3-vector to a JSON array.
 */
template <typename T>
nlohmann::json Vec3ToJson(const Vec3<T>& vec)
{
  return nlohmann::json::array({vec[0], vec[1], vec[2]});
}

/**
 * @brief Adds the grid-specific keys (dimensions, cell data path, origin, spacing).
 * RectGrid origin is derived from its bounds arrays and is omitted when those are unavailable.
 */
void AppendGridGeometryFields(nlohmann::json& geometryNode, const IGridGeometry& gridGeometry)
{
  geometryNode["dimensions"] = Vec3ToJson(gridGeometry.getDimensions());
  if(gridGeometry.getCellDataId().has_value())
  {
    geometryNode["cell_data_path"] = gridGeometry.getCellDataPath().toString();
  }

  if(const auto* imageGeom = dynamic_cast<const ImageGeom*>(&gridGeometry); imageGeom != nullptr)
  {
    geometryNode["origin"] = Vec3ToJson(imageGeom->getOrigin());
    geometryNode["spacing"] = Vec3ToJson(imageGeom->getSpacing());
  }
  else if(const auto* rectGridGeom = dynamic_cast<const RectGridGeom*>(&gridGeometry); rectGridGeom != nullptr)
  {
    Result<FloatVec3> originResult = rectGridGeom->getOrigin();
    if(originResult.valid())
    {
      geometryNode["origin"] = Vec3ToJson(originResult.value());
    }
  }
}

/**
 * @brief Adds the node-based keys. Each dimensionality contributes only the
 * counts and attribute-matrix paths it owns. A *_data_path key is omitted
 * when that attribute matrix has not been assigned.
 */
void AppendNodeGeometryFields(nlohmann::json& geometryNode, const INodeGeometry0D& nodeGeometry)
{
  geometryNode["num_vertices"] = nodeGeometry.getNumberOfVertices();
  if(nodeGeometry.getVertexAttributeMatrixId().has_value())
  {
    geometryNode["vertex_data_path"] = nodeGeometry.getVertexAttributeMatrixDataPath().toString();
  }

  if(const auto* geom1D = dynamic_cast<const INodeGeometry1D*>(&nodeGeometry); geom1D != nullptr)
  {
    geometryNode["num_edges"] = geom1D->getNumberOfEdges();
    if(geom1D->getEdgeAttributeMatrixId().has_value())
    {
      geometryNode["edge_data_path"] = geom1D->getEdgeAttributeMatrixDataPath().toString();
    }
  }

  if(const auto* geom2D = dynamic_cast<const INodeGeometry2D*>(&nodeGeometry); geom2D != nullptr)
  {
    geometryNode["num_faces"] = geom2D->getNumberOfFaces();
    if(geom2D->getFaceAttributeMatrixId().has_value())
    {
      geometryNode["face_data_path"] = geom2D->getFaceAttributeMatrixDataPath().toString();
    }
  }

  if(const auto* geom3D = dynamic_cast<const INodeGeometry3D*>(&nodeGeometry); geom3D != nullptr)
  {
    geometryNode["num_polyhedra"] = geom3D->getNumberOfPolyhedra();
    if(geom3D->getPolyhedraAttributeMatrixId().has_value())
    {
      geometryNode["polyhedron_data_path"] = geom3D->getPolyhedronAttributeMatrixDataPath().toString();
    }
  }
}

/**
 * @brief Builds and attaches the "geometry" block for any IGeometry.
 */
void AppendGeometryFields(nlohmann::json& node, const IGeometry& geometry)
{
  nlohmann::json geometryNode;
  geometryNode["geometry_type"] = IGeometry::GeomTypeToString(geometry.getGeomType());
  geometryNode["unit_dimensionality"] = geometry.getUnitDimensionality();
  geometryNode["length_units"] = IGeometry::LengthUnitToString(geometry.getUnits());
  geometryNode["num_cells"] = geometry.getNumberOfCells();

  if(const auto* gridGeometry = dynamic_cast<const IGridGeometry*>(&geometry); gridGeometry != nullptr)
  {
    AppendGridGeometryFields(geometryNode, *gridGeometry);
  }
  else if(const auto* nodeGeometry = dynamic_cast<const INodeGeometry0D*>(&geometry); nodeGeometry != nullptr)
  {
    AppendNodeGeometryFields(geometryNode, *nodeGeometry);
  }

  node["geometry"] = std::move(geometryNode);
}

/**
 * @brief Builds the JSON node for one DataObject, without its children.
 * Type-specific keys are added by dynamic type; geometries get a nested "geometry" block.
 */
nlohmann::json MakeObjectNode(const DataObject& object, const DataPath& path)
{
  nlohmann::json node;
  node["name"] = object.getName();
  node["path"] = path.toString();
  node["id"] = object.getId();
  node["type"] = object.getTypeName();

  if(const auto* geometry = dynamic_cast<const IGeometry*>(&object); geometry != nullptr)
  {
    AppendGeometryFields(node, *geometry);
  }
  else if(const auto* attributeMatrix = dynamic_cast<const AttributeMatrix*>(&object); attributeMatrix != nullptr)
  {
    AppendAttributeMatrixFields(node, *attributeMatrix);
  }
  else if(const auto* dataArray = dynamic_cast<const IDataArray*>(&object); dataArray != nullptr)
  {
    AppendDataArrayFields(node, *dataArray);
  }
  else if(const auto* stringArray = dynamic_cast<const StringArray*>(&object); stringArray != nullptr)
  {
    AppendStringArrayFields(node, *stringArray);
  }
  else if(const auto* neighborList = dynamic_cast<const INeighborList*>(&object); neighborList != nullptr)
  {
    AppendNeighborListFields(node, *neighborList);
  }

  node["children"] = nlohmann::json::array();
  return node;
}
} // namespace

DataStructure::DataStructure()
: m_IsValid(true)
{
}

DataStructure::DataStructure(const DataStructure& dataStructure)
: m_DataObjects(dataStructure.m_DataObjects)
, m_RootGroup(dataStructure.m_RootGroup)
, m_IsValid(dataStructure.m_IsValid)
, m_NextId(dataStructure.m_NextId)
, m_FormatResolver(dataStructure.m_FormatResolver)
{
  // Keep copied objects alive while the root map rebinds their DataStructure.
  std::map<DataObject::IdType, std::shared_ptr<DataObject>> sharedData;
  for(const auto& [identifier, dataWkPtr] : dataStructure.m_DataObjects)
  {
    auto dataPtr = dataWkPtr.lock();
    if(dataPtr != nullptr)
    {
      auto copy = std::shared_ptr<DataObject>(dataPtr->shallowCopy());
      sharedData[identifier] = copy;
      m_DataObjects[identifier] = copy;
    }
  }
  m_RootGroup.setDataStructure(this);
}

DataStructure::DataStructure(DataStructure&& dataStructure) noexcept
: m_DataObjects(std::move(dataStructure.m_DataObjects))
, m_RootGroup(std::move(dataStructure.m_RootGroup))
, m_IsValid(dataStructure.m_IsValid)
, m_NextId(dataStructure.m_NextId)
, m_FormatResolver(std::move(dataStructure.m_FormatResolver))
{
  m_RootGroup.setDataStructure(this);
}

DataStructure::~DataStructure()
{
  m_IsValid = false;
  for(auto& [identifier, weakDataPtr] : m_DataObjects)
  {
    if(auto sharedDataPtr = weakDataPtr.lock())
    {
      if(sharedDataPtr->getDataStructure() == this)
      {
        sharedDataPtr->setDataStructure(nullptr);
      }
    }
  }
}

void DataStructure::setFormatResolver(std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  m_FormatResolver = std::move(resolver);
}

const IDataStoreFormatResolver& DataStructure::formatResolver() const
{
  if(m_FormatResolver != nullptr)
  {
    return *m_FormatResolver;
  }
  return *DefaultFormatResolverRef();
}

void DataStructure::setDefaultFormatResolver(std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  if(resolver != nullptr)
  {
    DefaultFormatResolverRef() = std::move(resolver);
  }
}

DataObject::IdType DataStructure::generateId()
{
  return m_NextId++;
}

void DataStructure::setNextId(DataObject::IdType nextDataId)
{
  m_NextId = nextDataId;
}

size_t DataStructure::getSize() const
{
  return m_DataObjects.size();
}

void DataStructure::clear()
{
  auto topDataIds = m_RootGroup.getKeys();
  for(auto dataId : topDataIds)
  {
    removeData(dataId);
  }
  m_DataObjects.clear();
}

std::optional<DataObject::IdType> DataStructure::getId(const DataPath& path) const
{
  if(path.empty())
  {
    return {0};
  }
  const DataObject* dataObject = getData(path);
  if(nullptr == dataObject)
  {
    return std::nullopt;
  }
  return dataObject->getId();
}

LinkedPath DataStructure::getLinkedPath(const DataPath& path) const
{
  try
  {
    std::vector<DataObject::IdType> pathIds;
    const DataObject* data = m_RootGroup[path[0]];
    const BaseGroup* parent = dynamic_cast<const BaseGroup*>(data);
    pathIds.push_back(data->getId());

    for(usize i = 1; i < path.getLength(); i++)
    {
      std::string name = path[i];
      data = (*parent)[name];
      pathIds.push_back(data->getId());

      parent = dynamic_cast<const BaseGroup*>(data);
    }

    return LinkedPath(this, pathIds);
  } catch(const std::exception& e)
  {
    return LinkedPath();
  }
}

bool DataStructure::containsData(DataObject::IdType identifier) const
{
  return getData(identifier) != nullptr;
}

bool DataStructure::containsData(const DataPath& path) const
{
  return getData(path) != nullptr;
}

Result<LinkedPath> DataStructure::makePath(const DataPath& path)
{
  std::vector<DataObject::IdType> createdIds;

  try
  {
    std::vector<DataObject::IdType> pathIds;
    std::string name = path[0];
    const DataObject* data = m_RootGroup[name];
    if(data == nullptr)
    {
      data = nx::core::DataGroup::Create(*this, name);
      createdIds.push_back(data->getId());
    }
    const BaseGroup* parent = dynamic_cast<const BaseGroup*>(data);
    pathIds.push_back(data->getId());

    for(usize i = 1; i < path.getLength(); i++)
    {
      if(parent == nullptr)
      {
        return nx::core::MakeErrorResult<LinkedPath>(-3, fmt::format("Target parent object '{}' in path '{}' is not derived from BaseGroup.", name, path.toString()));
      }

      name = path[i];
      data = (*parent)[name];
      if(data == nullptr)
      {
        data = DataGroup::Create(*this, name, pathIds.back());
        createdIds.push_back(data->getId());
      }
      pathIds.push_back(data->getId());

      parent = dynamic_cast<const BaseGroup*>(data);
    }

    return {LinkedPath(this, pathIds)};
  } catch(const std::exception& e)
  {
    for(const auto& id : createdIds)
    {
      removeData(id);
    }

    return nx::core::MakeErrorResult<LinkedPath>(-2, fmt::format("Exception thrown when attempting to create a path '{}' in the DataStructure: '{}'", path.toString(), e.what()));
  }
}

std::vector<DataPath> DataStructure::getDataPathsForId(DataObject::IdType identifier) const
{
  auto* dataObject = getData(identifier);
  if(dataObject == nullptr)
  {
    return {};
  }
  return dataObject->getDataPaths();
}

std::vector<DataPath> DataStructure::getAllDataPaths() const
{
  std::vector<DataPath> dataPaths;
  for(const auto& [identifier, weakPtr] : m_DataObjects)
  {
    auto sharedPtr = weakPtr.lock();
    if(sharedPtr == nullptr)
    {
      continue;
    }

    auto localPaths = sharedPtr->getDataPaths();
    dataPaths.insert(dataPaths.end(), localPaths.begin(), localPaths.end());
  }
  return dataPaths;
}

std::vector<DataObject::IdType> DataStructure::getAllDataObjectIds() const
{
  std::vector<DataObject::IdType> dataIds;
  dataIds.reserve(m_DataObjects.size());
  for(const auto& [identifier, weakPtr] : m_DataObjects)
  {
    dataIds.push_back(identifier);
  }
  return dataIds;
}

DataObject* DataStructure::getData(DataObject::IdType identifier)
{
  auto iter = m_DataObjects.find(identifier);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

DataObject* DataStructure::getData(const std::optional<DataObject::IdType>& identifier)
{
  if(!identifier)
  {
    return nullptr;
  }

  auto iter = m_DataObjects.find(identifier.value());
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

DataObject* DataStructure::getData(const DataPath& path)
{
  if(path.empty())
  {
    return nullptr;
  }
  DataObject* targetObject = m_RootGroup[path[0]];
  for(usize index = 1; index < path.getLength(); index++)
  {
    if(targetObject == nullptr)
    {
      return nullptr;
    }
    if(!targetObject->isGroup())
    {
      return nullptr;
    }
    auto* groupObject = static_cast<BaseGroup*>(targetObject);
    DataObject* childObject = (*groupObject)[path[index]];
    if(childObject == nullptr)
    {
      return nullptr;
    }
    targetObject = childObject;
  }

  return targetObject;
}

DataObject& DataStructure::getDataRef(const DataPath& path)
{
  DataObject* object = getData(path);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Input Path '{}' does not exist", path.toString()));
  }
  return *object;
}

DataObject& DataStructure::getDataRef(DataObject::IdType identifier)
{
  DataObject* object = getData(identifier);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Id '{}' does not exist", identifier));
  }
  return *object;
}

DataObject* DataStructure::getData(const LinkedPath& path)
{
  return getData(path.getId());
}

const DataObject* DataStructure::getData(DataObject::IdType identifier) const
{
  auto iter = m_DataObjects.find(identifier);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

const DataObject* DataStructure::getData(const std::optional<DataObject::IdType>& identifier) const
{
  if(!identifier)
  {
    return nullptr;
  }

  auto iter = m_DataObjects.find(identifier.value());
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

const DataObject* DataStructure::getData(const DataPath& path) const
{
  if(path.empty())
  {
    return nullptr;
  }
  const DataObject* targetObject = m_RootGroup[path[0]];
  for(usize index = 1; index < path.getLength(); index++)
  {
    if(targetObject == nullptr)
    {
      return nullptr;
    }
    if(!targetObject->isGroup())
    {
      return nullptr;
    }
    const auto* groupObject = static_cast<const BaseGroup*>(targetObject);
    const DataObject* childObject = (*groupObject)[path[index]];
    if(childObject == nullptr)
    {
      return nullptr;
    }
    targetObject = childObject;
  }

  return targetObject;
}

const DataObject& DataStructure::getDataRef(const DataPath& path) const
{
  const DataObject* object = getData(path);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Input Path '{}' does not exist", path.toString()));
  }
  return *object;
}

const DataObject& DataStructure::getDataRef(DataObject::IdType identifier) const
{
  const DataObject* object = getData(identifier);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Id '{}' does not exist", identifier));
  }
  return *object;
}

const DataObject* DataStructure::getData(const LinkedPath& path) const
{
  return path.getData();
}

std::shared_ptr<DataObject> DataStructure::getSharedData(DataObject::IdType id)
{
  if(m_DataObjects.find(id) == m_DataObjects.end())
  {
    return nullptr;
  }
  return m_DataObjects.at(id).lock();
}

std::shared_ptr<const DataObject> DataStructure::getSharedData(DataObject::IdType id) const
{
  if(m_DataObjects.find(id) == m_DataObjects.end())
  {
    return nullptr;
  }
  return m_DataObjects.at(id).lock();
}

std::shared_ptr<DataObject> DataStructure::getSharedData(const DataPath& path)
{
  auto dataObject = getData(path);
  if(dataObject == nullptr)
  {
    return nullptr;
  }
  return m_DataObjects.at(dataObject->getId()).lock();
}

std::shared_ptr<const DataObject> DataStructure::getSharedData(const DataPath& path) const
{
  auto dataObject = getData(path);
  if(dataObject == nullptr)
  {
    return nullptr;
  }
  return m_DataObjects.at(dataObject->getId()).lock();
}

bool DataStructure::removeData(DataObject::IdType identifier)
{
  DataObject* data = getData(identifier);
  return removeData(data);
}

void DataStructure::setData(DataObject::IdType identifier, std::shared_ptr<DataObject> dataObject)
{
  if(dataObject == nullptr)
  {
    removeData(identifier);
    return;
  }

  m_DataObjects[identifier] = dataObject;
}

bool DataStructure::removeData(const std::optional<DataObject::IdType>& identifier)
{
  if(!identifier)
  {
    return false;
  }
  else
  {
    return removeData(identifier.value());
  }
}

bool DataStructure::removeData(const DataPath& path)
{
  DataObject* data = getData(path);
  return removeData(data);
}

bool DataStructure::removeData(DataObject* data)
{
  if(data == nullptr)
  {
    return false;
  }

  auto pathsToData = data->getDataPaths();
  auto parentIds = data->getParentIds();
  if(parentIds.size() == 0)
  {
    return removeTopLevel(data);
  }
  for(DataObject::IdType parentId : parentIds)
  {
    auto parent = getDataAs<BaseGroup>(parentId);
    if(!parent->remove(data))
    {
      return false;
    }
  }

  return true;
}

void DataStructure::dataDeleted(DataObject::IdType identifier, const std::string& name)
{
  if(!m_IsValid)
  {
    return;
  }

  auto msg = std::make_shared<DataRemovedMessage>(this, identifier, name);
  notify(msg);
}

std::vector<DataObject*> DataStructure::getTopLevelData() const
{
  std::vector<DataObject*> topLevel(m_RootGroup.getSize(), nullptr);
  usize index = 0;
  for(auto& iter : m_RootGroup)
  {
    auto obj = iter.second;
    topLevel[index++] = (obj.get());
  }
  return topLevel;
}

const DataMap& DataStructure::getDataMap() const
{
  return m_RootGroup;
}

DataMap& DataStructure::getRootGroup()
{
  return m_RootGroup;
}

bool DataStructure::insertTopLevel(const std::shared_ptr<DataObject>& obj)
{
  if(obj == nullptr)
  {
    return false;
  }

  if(m_RootGroup.contains(obj.get()) || m_RootGroup.contains(obj->getName()))
  {
    return false;
  }

  return m_RootGroup.insert(obj);
}

bool DataStructure::removeTopLevel(DataObject* data)
{
  std::string name = data->getName();
  if(!m_RootGroup.remove(data))
  {
    return false;
  }

  DataPath path({name});
  std::vector<DataPath> paths({path});
  return true;
}

bool DataStructure::finishAddingObject(const std::shared_ptr<DataObject>& dataObject, const std::optional<DataObject::IdType>& parent)
{
  if(parent.has_value() && containsData(*parent))
  {
    auto* parentContainer = dynamic_cast<BaseGroup*>(getData(*parent));
    if(parentContainer == nullptr)
    {
      return false;
    }
    if(!parentContainer->insert(dataObject))
    {
      return false;
    }
  }
  else if(!insertTopLevel(dataObject))
  {
    return false;
  }

  trackDataObject(dataObject);
  auto msg = std::make_shared<DataAddedMessage>(this, dataObject->getId());
  notify(msg);
  return true;
}

DataStructure::Iterator DataStructure::begin()
{
  return m_RootGroup.begin();
}

DataStructure::Iterator DataStructure::end()
{
  return m_RootGroup.end();
}

DataStructure::ConstIterator DataStructure::begin() const
{
  return m_RootGroup.begin();
}

DataStructure::ConstIterator DataStructure::end() const
{
  return m_RootGroup.end();
}

bool DataStructure::insert(const std::shared_ptr<DataObject>& dataObject, const DataPath& dataPath)
{
  if(dataObject == nullptr)
  {
    return false;
  }

  if(dataObject.get() == getData(dataObject->getId()))
  {
    return false;
  }

  if(getData(dataObject->getId()) != nullptr)
  {
    dataObject->setId(generateId());
  }

  // A new insertion starts with one requested placement and no inherited parents.
  dataObject->clearParents();

  if(dataPath.empty())
  {
    return insertIntoRoot(dataObject);
  }

  auto parentGroup = getDataAs<BaseGroup>(dataPath);
  return insertIntoParent(dataObject, parentGroup);
}

DataObject::IdType DataStructure::getNextId() const
{
  return m_NextId;
}

bool DataStructure::insertIntoRoot(const std::shared_ptr<DataObject>& dataObject)
{
  if(dataObject == nullptr)
  {
    return false;
  }

  if(!m_RootGroup.insert(dataObject))
  {
    return false;
  }
  trackDataObject(dataObject);
  return true;
}
bool DataStructure::insertIntoParent(const std::shared_ptr<DataObject>& dataObject, BaseGroup* parentGroup)
{
  if(parentGroup == nullptr)
  {
    return false;
  }

  if(!parentGroup->insert(dataObject))
  {
    return false;
  }
  trackDataObject(dataObject);
  return true;
}

void DataStructure::trackDataObject(const std::shared_ptr<DataObject>& dataObject)
{
  if(dataObject == nullptr)
  {
    return;
  }
  if(m_DataObjects.find(dataObject->getId()) == m_DataObjects.end())
  {
    m_DataObjects[dataObject->getId()] = dataObject;
    if(m_NextId <= dataObject->getId())
    {
      m_NextId = dataObject->getId() + 1;
    }
  }
  dataObject->setDataStructure(this);
}

bool DataStructure::setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParentId)
{
  auto& target = m_DataObjects[targetId];
  auto newParent = dynamic_cast<BaseGroup*>(getData(newParentId));
  if(newParent == nullptr)
  {
    return false;
  }

  if(!newParent->insert(target))
  {
    return false;
  }

  notify(std::make_shared<DataReparentedMessage>(this, targetId, newParentId, true));
  return true;
}

bool DataStructure::removeParent(DataObject::IdType targetId, DataObject::IdType parentId)
{
  const auto& target = m_DataObjects[targetId];
  const auto parent = dynamic_cast<BaseGroup*>(getData(parentId));
  const auto targetPtr = target.lock();
  if(targetPtr == nullptr)
  {
    return false;
  }
  if(parentId == 0)
  {
    return removeTopLevel(targetPtr.get());
  }
  return parent->remove(targetPtr.get());
}

DataStructure::SignalType& DataStructure::getSignal()
{
  return m_Signal;
}

void DataStructure::notify(const std::shared_ptr<AbstractDataStructureMessage>& msg)
{
  if(!m_IsValid || msg == nullptr)
  {
    return;
  }
  m_Signal(this, msg);
}

DataStructure& DataStructure::operator=(const DataStructure& rhs)
{
  m_DataObjects = rhs.m_DataObjects;
  m_RootGroup = rhs.m_RootGroup;
  m_IsValid = rhs.m_IsValid;
  m_NextId = rhs.m_NextId;
  m_FormatResolver = rhs.m_FormatResolver;

  // Keep copied objects alive while the root map rebinds their DataStructure.
  std::map<DataObject::IdType, std::shared_ptr<DataObject>> sharedData;
  for(auto& [identifier, dataWkPtr] : rhs.m_DataObjects)
  {
    auto dataPtr = dataWkPtr.lock();
    if(dataPtr != nullptr)
    {
      auto copy = std::shared_ptr<DataObject>(dataPtr->shallowCopy());
      sharedData[identifier] = copy;
      m_DataObjects[identifier] = copy;
    }
  }
  applyAllDataStructure();
  return *this;
}

DataStructure& DataStructure::operator=(DataStructure&& rhs) noexcept
{
  m_DataObjects = std::move(rhs.m_DataObjects);
  m_RootGroup = std::move(rhs.m_RootGroup);
  m_IsValid = std::move(rhs.m_IsValid);
  m_NextId = std::move(rhs.m_NextId);
  m_FormatResolver = std::move(rhs.m_FormatResolver);

  applyAllDataStructure();
  return *this;
}

void DataStructure::applyAllDataStructure()
{
  m_RootGroup.setDataStructure(this);
}

nonstd::expected<void, std::string> DataStructure::validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const
{
  if(dataPaths.empty())
  {
    return {};
  }

  std::stringstream message;

  usize tupleCount = std::numeric_limits<usize>::max();
  for(const auto& dataPath : dataPaths)
  {
    auto* dataObject = getData(dataPath);

    const DataObject::Type dataObjectType = dataObject->getDataObjectType();
    size_t numTuples = 0;
    if(dataObjectType == DataObject::Type::NeighborList || dataObjectType == DataObject::Type::StringArray || dataObjectType == DataObject::Type::DataArray)
    {
      const auto* dataArrayPtr = getDataAs<IArray>(dataPath);
      numTuples = dataArrayPtr->getNumberOfTuples();
    }
    else
    {
      message << "Only NeighborList, StringArray and DataArray can be validated for tuple counts\n";
      return {nonstd::make_unexpected(message.str())};
    }

    auto parentPaths = dataObject->getDataPaths();
    for(const auto& path : parentPaths)
    {
      message << "DataPath: " << path.toString() << "    | Tuple Count: " << numTuples << "\n";
    }

    // The first array establishes the tuple count for all later comparisons.
    if(tupleCount == std::numeric_limits<usize>::max())
    {
      tupleCount = numTuples;
    }
    else if(tupleCount != numTuples)
    {
      return {nonstd::make_unexpected(message.str())};
    }
  }
  return {};
}

void DataStructure::resetIds(DataObject::IdType startingId)
{
  // Zero is the root identifier and cannot identify an inserted object.
  if(startingId == 0)
  {
    startingId = 1;
  }

  m_NextId = startingId;

  // Build an old-to-new map before updating hierarchy references.
  WeakCollectionType newCollection;
  std::unordered_map<DataObject::IdType, DataObject::IdType> updatedIdsMap;
  for(auto& dataObjectIter : m_DataObjects)
  {
    auto dataObjectPtr = dataObjectIter.second.lock();
    if(dataObjectPtr == nullptr)
    {
      continue;
    }

    auto oldId = dataObjectIter.first;
    auto newId = generateId();

    dataObjectPtr->setId(newId);
    updatedIdsMap[oldId] = newId;

    newCollection.insert({newId, dataObjectPtr});
  }

  m_DataObjects = newCollection;

  for(auto& dataObjectIter : m_DataObjects)
  {
    auto dataObjectPtr = dataObjectIter.second.lock();
    if(dataObjectPtr != nullptr)
    {
      dataObjectPtr->checkUpdatedIds(updatedIdsMap);
    }
  }
  m_RootGroup.updateIds(updatedIdsMap);
}

void DataStructure::exportHierarchyAsGraphViz(std::ostream& outputStream) const
{
  outputStream << "digraph DataGraph {\n"
               << "\tlabelloc =\"t\"\n"
               << "\trankdir=LR;\n"
               << "\tlabel=\"DataStructure Hierarchy\"\n"
               << "\tlabelloc=\"t\"\n"
               << "\tfontcolor=\"#FFFFFA\"\n"
               << "\tfontsize=12\n"
               << "\tgraph [splines=true bgcolor=\"#242627\"]\n"
               << "\tnode [shape=record style=\"filled\" fillcolor=\"#1D7ECD\" fontsize=12 fontcolor=\"#FFFFFA\"]\n"
               << "\tedge [dir=front arrowtail=empty style=\"\" color=\"#FFFFFA\"]\n\n";
  for(const auto* object : getTopLevelData())
  {
    auto topLevelPath = DataPath::FromString(object->getDataPaths()[0].getTargetName()).value();
    auto optionalDataPaths = GetAllChildDataPaths(*this, topLevelPath);
    outputStream << "\n/* Top level DataObject: " << topLevelPath.getTargetName() << " */\n\"" << topLevelPath.getTargetName() << "\";\n";

    if(optionalDataPaths.has_value() && !optionalDataPaths.value().empty())
    {
      recurseHierarchyToGraphViz(outputStream, optionalDataPaths.value(), topLevelPath.getTargetName());
    }
  }

  outputStream << "}\n";
}

void DataStructure::exportHierarchyAsText(std::ostream& outputStream) const
{
  for(const auto* object : getTopLevelData())
  {
    auto topLevelPath = DataPath::FromString(object->getDataPaths()[0].getTargetName()).value();
    outputStream << k_Delimiter << topLevelPath.getTargetName() << "\n";
    auto optionalDataPaths = GetAllChildDataPaths(*this, topLevelPath);

    if(optionalDataPaths.has_value() && !optionalDataPaths.value().empty())
    {
      recurseHierarchyToText(outputStream, optionalDataPaths.value(), "");
    }
  }
}

void DataStructure::recurseHierarchyToGraphViz(std::ostream& outputStream, const std::vector<DataPath> paths, const std::string& parent) const
{
  for(const auto& path : paths)
  {
    outputStream << "\"" << parent << "\" -> \"" << path.getTargetName() << "\"\n";

    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(!optionalChildPaths.has_value() || optionalChildPaths.value().empty())
    {
      continue;
    }

    recurseHierarchyToGraphViz(outputStream, optionalChildPaths.value(), path.getTargetName());
  }
  // outputStream << "\n"; // for readability
}

void DataStructure::recurseHierarchyToText(std::ostream& outputStream, const std::vector<DataPath> paths, std::string indent) const
{
  indent += "  ";

  for(const auto& path : paths)
  {
    outputStream << indent << k_Delimiter << path.getTargetName() << "\n";

    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(!optionalChildPaths.has_value() || optionalChildPaths.value().empty())
    {
      continue;
    }

    recurseHierarchyToText(outputStream, optionalChildPaths.value(), indent);
  }
}

nlohmann::json DataStructure::exportHierarchyAsJson() const
{
  nlohmann::json root;
  root["schema_version"] = k_HierarchyJsonSchemaVersion;
  root["objects"] = nlohmann::json::array();

  for(const auto* object : getTopLevelData())
  {
    if(object == nullptr)
    {
      continue;
    }
    const DataPath topLevelPath({object->getName()});
    nlohmann::json node = MakeObjectNode(*object, topLevelPath);

    auto optionalChildPaths = GetAllChildDataPaths(*this, topLevelPath);
    if(optionalChildPaths.has_value() && !optionalChildPaths.value().empty())
    {
      recurseHierarchyToJson(node["children"], optionalChildPaths.value());
    }
    root["objects"].push_back(std::move(node));
  }

  return root;
}

void DataStructure::recurseHierarchyToJson(nlohmann::json& nodes, const std::vector<DataPath>& paths) const
{
  for(const auto& path : paths)
  {
    const DataObject* object = getData(path);
    if(object == nullptr)
    {
      continue;
    }
    nlohmann::json node = MakeObjectNode(*object, path);

    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(optionalChildPaths.has_value() && !optionalChildPaths.value().empty())
    {
      recurseHierarchyToJson(node["children"], optionalChildPaths.value());
    }
    nodes.push_back(std::move(node));
  }
}

void DataStructure::flush() const
{
  for(const auto& weakPtr : m_DataObjects)
  {
    std::shared_ptr<DataObject> sharedObj = weakPtr.second.lock();
    if(sharedObj == nullptr)
    {
      continue;
    }
    sharedObj->flush();
  }
}

uint64 DataStructure::memoryUsage() const
{
  uint64 memory = 0;
  for(const auto& dataIter : m_DataObjects)
  {
    auto dataPtr = dataIter.second.lock();
    if(dataPtr == nullptr)
    {
      continue;
    }
    memory += dataPtr->memoryUsage();
  }
  return memory;
}

Result<> DataStructure::transferDataArraysOoc()
{
  Result<> result;

  // Resolve each array with the creation-time format resolver.
  // The resolver keeps core independent of concrete out-of-core formats.
  for(const auto& dataIter : m_DataObjects)
  {
    auto dataPtr = dataIter.second.lock();
    auto dataArrayPtr = std::dynamic_pointer_cast<IDataArray>(dataPtr);
    if(dataArrayPtr == nullptr)
    {
      continue;
    }

    // The first path supplies the geometry context. Multiple geometry links are
    // not a supported conversion configuration.
    const std::vector<DataPath> paths = getDataPathsForId(dataIter.first);
    if(paths.empty())
    {
      continue;
    }

    const std::string resolvedFormat = ArrayCreationUtilities::ResolveStorageFormat(*this, paths.front(), dataArrayPtr->getDataType(), dataArrayPtr->memoryUsage(), "");

    // An empty format selects in-core storage, so conversion is not required.
    if(resolvedFormat.empty())
    {
      continue;
    }

    if(!ConvertIDataArray(dataArrayPtr, resolvedFormat))
    {
      result.warnings().emplace_back(Warning{-3570, fmt::format("Cannot convert DataArray: '{}' to out-of-core", dataArrayPtr->getName())});
    }
  }

  return result;
}

Result<> DataStructure::validateGeometries() const
{
  // Geometry DataObject::Type values form a contiguous range. Unit tests assert
  // this range so a changed enumeration cannot silently skip validation.
  Result<> result;
  for(const auto& dataObject : m_RootGroup)
  {
    auto dataObjectType = dataObject.second->getDataObjectType();
    if(dataObjectType >= DataObject::Type::IGeometry && dataObjectType <= DataObject::Type::TetrahedralGeom)
    {
      auto* geomPtr = dynamic_cast<IGeometry*>(dataObject.second.get());
      result = MergeResults(geomPtr->validate(), result);
    }
  }
  return result;
}

Result<> DataStructure::validateAttributeMatrices() const
{
  Result<> result;
  for(const auto& dataObject : m_DataObjects)
  {
    auto dataObjectSharedPtr = dataObject.second.lock();
    if(dataObjectSharedPtr.get() != nullptr)
    {
      auto dataObjectType = dataObjectSharedPtr->getDataObjectType();
      if(dataObjectType == DataObject::Type::AttributeMatrix)
      {
        auto* attrMatPtr = dynamic_cast<AttributeMatrix*>(dataObject.second.lock().get());
        if(nullptr != attrMatPtr)
        {
          result = MergeResults(attrMatPtr->validate(), result);
        }
      }
    }
  }
  return result;
}

} // namespace nx::core
