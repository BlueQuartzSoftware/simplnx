#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

namespace complex
{
class COMPLEX_EXPORT DataArrayFactory : public IH5DataFactory
{
public:
  DataArrayFactory()
  : IH5DataFactory()
  {
  }

  virtual ~DataArrayFactory() = default;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return "DataArray";
  }

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID
   * @param ds DataStructure to add the created DataObject to.
   * @param targetId ID for the target HDF5 object.
   * @param parentId Optional DataObject ID describing which parent object to
   * create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType createFromHdf5(DataStructure& ds, H5::IdType targetId, const std::optional<DataObject::IdType>& parentId = {}) override
  {
    throw std::runtime_error("Not implemented");
  }
};
} // namespace complex
