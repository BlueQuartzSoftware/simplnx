#pragma once

#include <optional>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

namespace complex
{
class IH5DataFactory
{
public:
  virtual ~IH5DataFactory() = default;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID
   * @param ds DataStructure to add the created DataObject to.
   * @param targetId ID for the target HDF5 object
   * @param parentId Optional DataObject ID describing which parent object to
   * create the generated DataObject under.
   * @return H5::ErrorType
   */
  virtual H5::ErrorType createFromHdf5(DataStructure& ds, H5::IdType targetId, const std::optional<DataObject::IdType>& parentId = {}) = 0;

  // Copy and move constuctors / operators deleted
  IH5DataFactory(const IH5DataFactory& other) = delete;
  IH5DataFactory(IH5DataFactory&& other) = delete;
  IH5DataFactory& operator=(const IH5DataFactory& rhs) = delete;
  IH5DataFactory& operator=(IH5DataFactory&& rhs) = delete;

protected:
  /**
   * @brief Default constructor
   */
  IH5DataFactory();
};
} // namespace complex
