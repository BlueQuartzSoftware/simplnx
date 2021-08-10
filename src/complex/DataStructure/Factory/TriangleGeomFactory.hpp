#pragma once

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

namespace complex
{
class TriangleGeomFactory : public IH5DataFactory
{
public:
  TriangleGeomFactory()
  : IH5DataFactory()
  {
  }

  virtual ~TriangleGeomFactory() = default;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return "TriangleGeom";
  }

  /**
   * @brief Creates and adds an TriangleGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param ds DataStructure to add the created geometry to.
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