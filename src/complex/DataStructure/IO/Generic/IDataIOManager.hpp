#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"
#include "complex/complex_export.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace complex
{
class IDataFactory;

class COMPLEX_EXPORT IDataIOManager
{
public:
  using factory_id_type = std::string;
  using factory_ptr = std::shared_ptr<IDataFactory>;
  using factory_collection = std::map<factory_id_type, factory_ptr>;

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
   * @return IDataFactory*
   */
  factory_ptr getFactory(factory_id_type typeName) const;

  template <typename T>
  std::shared_ptr<T> getFactoryAs(factory_id_type typeName) const
  {
    return std::dynamic_pointer_cast<T>(getFactory(typeName));
  }

  template <typename T>
  void addFactory()
  {
    auto sharedIO = std::make_shared<T>();
    const auto key = sharedIO->getTypeName();
    m_FactoryCollection[key] = sharedIO;
  }

protected:
  IDataIOManager();

private:
  factory_collection m_FactoryCollection;
};
} // namespace complex
