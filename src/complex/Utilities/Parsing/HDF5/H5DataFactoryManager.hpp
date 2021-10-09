#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
namespace H5
{
class IDataFactory;

class COMPLEX_EXPORT DataFactoryManager
{
public:
  /**
   * @brief Constructs an DataFactoryManager and registers core H5DataFactories.
   */
  DataFactoryManager();
  virtual ~DataFactoryManager();

  /**
   * @brief Adds the specified IDataFactory for use when reading HDF5 files.
   * @param factory
   */
  void addFactory(IDataFactory* factory);

  /**
   * @brief Returns a vector of available IH5DataFactories.
   * @return std::vector<IDataFactory*>
   */
  std::vector<IDataFactory*> getFactories() const;

  /**
   * @brief Returns a pointer to the factory used for creating a specific
   * DataObject subclass.
   * @param typeName
   * @return IDataFactory*
   */
  IDataFactory* getFactory(const std::string& typeName) const;

private:
  /**
   * @brief Adds factories for DataObject subclasses defined within complex itself.
   */
  void addCoreFactories();

  ////////////
  // Variables
  std::map<std::string, std::shared_ptr<IDataFactory>> m_Factories;
};
} // namespace H5
} // namespace complex
