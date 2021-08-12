#pragma once

#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

namespace complex
{
class COMPLEX_EXPORT GridMontageFactory : public IH5DataFactory
{
public:
  GridMontageFactory()
  : IH5DataFactory()
  {
  }

  virtual ~GridMontageFactory() = default;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return "GridMontage";
  }

  /**
   * @brief Creates and adds an GridMontage to the provided DataStructure from
   * the target HDF5 ID.
   * @param ds DataStructure to add the created montage to.
   * @param targetId ID for the target HDF5 object
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