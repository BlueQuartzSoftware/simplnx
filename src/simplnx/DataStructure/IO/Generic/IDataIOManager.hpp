#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataFactory.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nx::core
{
class IDataFactory;

/**
 * @brief The IDataIOManager class serves as a base point for reading and writing DataStructures to specific file formats.
 * To support a new file format, create a derived class and provide subclasses of IDataFactory for all concrete DataObject types.
 */
class SIMPLNX_EXPORT IDataIOManager
{
public:
  using factory_id_type = std::string;
  using factory_ptr = std::shared_ptr<IDataFactory>;
  using factory_collection = std::map<factory_id_type, factory_ptr>;
  using DataStoreCreateFnc =
      std::function<std::unique_ptr<IDataStore>(DataType, const typename IDataStore::ShapeType&, const typename IDataStore::ShapeType&, const std::optional<IDataStore::ShapeType>&)>;
  using DataStoreCreationMap = std::map<std::string, DataStoreCreateFnc>;

  virtual ~IDataIOManager() noexcept;

  virtual std::string formatName() const = 0;

  /**
   * @brief Returns a collection of available data factories.
   * @return factory_collection
   */
  factory_collection getFactories() const;

  /**
   * @brief Returns a pointer to the factory used for creating a specific
   * DataObject subclass.
   * @param typeName
   * @return factory_ptr
   */
  factory_ptr getFactory(factory_id_type typeName) const;

  /**
   * @brief Returns a pointer to the factory used for creating a specific DataObject subclass.
   * @param typeName
   * @return std::shared_ptr<T>
   */
  template <typename T>
  std::shared_ptr<T> getFactoryAs(factory_id_type typeName) const
  {
    return std::dynamic_pointer_cast<T>(getFactory(typeName));
  }

  /**
   * Adds a factory of the specified type. T should be the IDataFactory subclass type.
   */
  template <typename T>
  void addFactory()
  {
    auto sharedIO = std::make_shared<T>();
    const auto key = sharedIO->getTypeName();
    m_FactoryCollection[key] = sharedIO;
  }

  DataStoreCreationMap getDataStoreCreationFunctions();

  bool hasDataStoreCreationFnc(const std::string& type) const;

  DataStoreCreateFnc dataStoreCreationFnc(const std::string& type) const;

protected:
  IDataIOManager();

  void addDataStoreCreationFnc(const std::string& type, DataStoreCreateFnc creationFnc);

private:
  factory_collection m_FactoryCollection;
  DataStoreCreationMap m_DataStoreCreationMap;
};
} // namespace nx::core
