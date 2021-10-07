#pragma once

#include <map>
#include <memory>
#include <vector>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class IH5DataFactory;

namespace H5
{
class COMPLEX_EXPORT DataFactoryManager
{
public:
  /**
   * @brief Constructs an DataFactoryManager and registers core H5DataFactories.
   */
  DataFactoryManager();
  virtual ~DataFactoryManager();

  /**
   * @brief Adds the specified IH5DataFactory for use when reading HDF5 files.
   * @param factory
   */
  void addFactory(IH5DataFactory* factory);

  /**
   * @brief Returns a vector of available IH5DataFactories.
   * @return std::vector<IH5DataFactory*>
   */
  std::vector<IH5DataFactory*> getFactories() const;

  /**
   * @brief Returns a pointer to the factory used for creating a specific
   * DataObject subclass.
   * @param typeName
   * @return IH5DataFactory*
   */
  IH5DataFactory* getFactory(const std::string& typeName) const;

private:
  /**
   * @brief Adds factories for DataObject subclasses defined within complex itself.
   */
  void addCoreFactories();

  ////////////
  // Variables
  std::map<std::string, std::shared_ptr<IH5DataFactory>> m_Factories;
};
} // namespace H5
} // namespace complex
