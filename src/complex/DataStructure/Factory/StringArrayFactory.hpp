#pragma once

#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

namespace complex
{
namespace H5
{
class StringArrayFactory : public IDataFactory
{
public:
  StringArrayFactory();
  ~StringArrayFactory() override;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override;

  /**
   * @brief Creates and adds an HexahedralGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Active DataStructureReader
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                            const std::optional<DataObject::IdType>& parentId = {}, bool preflight = false) override;

  /**
   * @brief Reads an HDF5 Dataset that makes up a DataStructure node.
   * @param dataStructureReader Active DataStructureReader
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param datasetReader Wrapper around the HDF5 Dataset.
   * @param parentId The HDF5 ID of the parent object.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                              const std::optional<DataObject::IdType>& parentId = {}, bool preflight = false) override;
};
} // namespace H5
} // namespace complex
