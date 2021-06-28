#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/ScalarData.hpp"
#include "complex/DataStructure/LinkedPath.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class AbstractDataStructureMessage;
class AbstractDataStructureObserver;
class AbstractGeometry;
class AbstractMontage;
class DataGroup;
class DataPath;

/**
 * class DataStructure
 *
 */
class COMPLEX_EXPORT DataStructure
{
public:
  using Iterator = DataMap::Iterator;
  using ConstIterator = DataMap::ConstIterator;

  friend class DataMap;
  friend class DataObject;

  /**
   * @brief
   */
  DataStructure();

  /**
   * @brief
   * @param other
   */
  DataStructure(const DataStructure& other);

  /**
   * @brief
   * @param other
   */
  DataStructure(DataStructure&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataStructure();

  /**
   * @brief Returns the number of unique DataObjects in the DataStructure.
   * @return size_t
   */
  size_t size() const;

  /**
   * @brief Returns the IdType for the DataObject found at the specified DataPath. The
   * return type is optional<IdType> for cases where the DataPath does not point to a
   * DataObject. If no DataObject is found at the path, an empty optional object is
   * returned.
   * @param path
   * @return std::optional<IdType>
   */
  std::optional<DataObject::IdType> getId(const DataPath& path) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType. If no such object
   * exists, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  DataObject* getData(DataObject::IdType id);

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no DataObject is
   * found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  DataObject* getData(const DataPath& path);

  /**
   * @brief Returns a pointer to the DataObject found at the specified LinkedPath. If no
   * such DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  DataObject* getData(const LinkedPath& path);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType. If no such object
   * exists, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  const DataObject* getData(DataObject::IdType id) const;

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no DataObject is
   * found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  const DataObject* getData(const DataPath& path) const;

  /**
   * @brief Returns a pointer to the DataObject found at the specified LinkedPath. If no
   * such DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  const DataObject* getData(const LinkedPath& path) const;

  /**
   * @brief Removes the DataObject using the specified IdType. Returns true if an object was
   * found. Otherwise, returns false.
   * @param id
   * @return bool
   */
  bool removeData(DataObject::IdType id);

  /**
   * @brief Removes the DataObject using the specified DataPath. Returns true if an object
   * was found. Otherwise, returns false.
   * @param path
   * @return bool
   */
  bool removeData(const DataPath& path);

  /**
   * @brief Returns a LinkedPath based on the specified DataPath.
   * @param path
   * @return LinkedPath
   */
  LinkedPath getLinkedPath(const DataPath& path) const;

  /**
   * @brief Creates a ScalarData object for the target parent container specified by the
   * given IdType. A default value can be specified.
   * Returns an EditableScalar object that serves as an editable wrapper for the created ScalarData.
   * @param parent
   * @param defaultValue
   * @return EditableScalar<T>
   */
  template <typename T>
  ScalarData<T> createScalar(const std::string& name, T defaultValue, std::optional<DataObject::IdType> parent = {})
  {
  }

  /**
   * @brief Creates a DataArray
   * @param parent
   * @param tupleSize
   * @param numTuples
   * @param defaultValue
   * @return EditableArray<T>
   */
  template <typename T>
  DataArray<T> createDataArray(const std::string& name, size_t tupleSize, size_t numTuples, T defaultValue, std::optional<DataObject::IdType> parent = {})
  {
  }

  /**
   * @brief
   * @param parent
   * @return DataGroup*
   */
  DataGroup* createGroup(const std::string& name, std::optional<DataObject::IdType> parent = {});

  /**
   * @brief
   * @param parent
   * @return AbstractMontage*
   */
  template <class T>
  AbstractMontage* createMontage(const std::string& name, std::optional<DataObject::IdType> parent = {})
  {
  }

  /**
   * @brief
   * @param parent
   * @return AbstractGeometry*
   */
  template <class T>
  AbstractGeometry* createGeometry(const std::string& name, std::optional<DataObject::IdType> parent)
  {
  }

  /**
   * @brief
   * @return std::vector<DataObject*>
   */
  std::vector<DataObject*> getTopLevelData() const;

  /**
   * @brief
   * @param targetId
   * @param newParent
   * @return bool
   */
  bool setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParent);

  /**
   * @brief
   * @param targetId
   * @param parent
   * @return bool
   */
  bool removeParent(DataObject::IdType targetId, DataObject::IdType parent);

  /**
   * @brief
   * @return std::vector<AbstractDataStructureObserver*>
   */
  std::vector<AbstractDataStructureObserver*> getObservers() const;

  /**
   * @brief
   * @param obs
   */
  void addObserver(AbstractDataStructureObserver* obs);

  /**
   * @brief
   * @param obs
   */
  void removeObserver(AbstractDataStructureObserver* obs);

  /**
   * @brief
   * @return iterator
   */
  Iterator begin();

  /**
   * @brief
   * @return iterator
   */
  Iterator end();

  /**
   * @brief
   * @return
   */
  ConstIterator begin() const;

  /**
   * @brief
   * @return
   */
  ConstIterator end() const;

  /**
   * @brief Writes the DataStructure as an XDMF file at the specified filepath
   * @param hdfFilePath
   * @return H5::ErrorType
   */
  H5::ErrorType writeXdmfFile(const std::filesystem::path& hdfFilePath) const;

  /**
   * @brief
   * @param hdfFilePath
   * @return H5::ErrorType
   */
  H5::ErrorType readXdmfFile(const std::filesystem::path& hdfFilePath);

protected:
  /**
   * @brief
   * @param id
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> getSharedData(DataObject::IdType id) const;

private:
  /**
   * @brief
   * @param container
   * @return
   */
  bool insertTopLevel(const std::shared_ptr<BaseGroup>& container);

  /**
   * @brief
   * @param data
   * @return
   */
  bool removeTopLevel(DataObject* data);

  /**
   * @brief
   * @param data
   * @return
   */
  bool removeData(DataObject* data);

  /**
   * @brief
   * @param id
   * @param paths
   */
  void dataDeleted(DataObject::IdType id, const std::string& name);

  /**
   * @brief
   * @param msg
   */
  void notify(const std::shared_ptr<AbstractDataStructureMessage>& msg);

  ////////////
  // Variables
  std::map<DataObject::IdType, std::weak_ptr<DataObject>> m_DataObjects;
  DataMap m_RootGroup;
  std::set<AbstractDataStructureObserver*> m_Observers;
  bool m_IsValid = false;
};
} // namespace complex
