#pragma once

#include "complex/complex_export.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace complex
{
class IDataFactory;

class COMPLEX_EXPORT IDataFactoryManager
{
public:
  using factory_ptr = std::shared_ptr<IDataFactory>;
  using factory_collection = std::vector<factory_ptr>;

  virtual ~IDataFactoryManager() noexcept;

  virtual std::string formatName() const = 0;

  /**
   * @brief Returns a collection of available data factories.
   * @return factory_collection
   */
  virtual factory_collection getFactories() const = 0;

  /**
   * @brief Returns a pointer to the factory used for creating a specific
   * DataObject subclass.
   * @param typeName
   * @return IDataFactory*
   */
  virtual factory_ptr getFactory(const std::string_view& typeName) const = 0;

protected:
  IDataFactoryManager();

private:
};
} // namespace complex
