#pragma once

#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT HexahedralGeomFactory : public IDataFactory
{
public:
  HexahedralGeomFactory();

  ~HexahedralGeomFactory() override;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override;

  /**
   * @brief Creates and adds an HexahedralGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Active DataStructureReader
   * @param groupReader Wrapped HDF5 group reader.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId = {}) override;

  /**
   * @brief Reads an HDF5 Dataset that makes up a DataStructure node.
   * @param dataStructureReader Active DataStructureReader
   * @param datasetReader Wrapped HDF5 dataset reader.
   * @param parentId = {} The HDF5 ID of the parent object.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId = {}) override;
};
} // namespace H5
} // namespace complex
