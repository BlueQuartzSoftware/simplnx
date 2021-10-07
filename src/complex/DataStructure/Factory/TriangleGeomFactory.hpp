#pragma once

#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

namespace complex
{
class COMPLEX_EXPORT TriangleGeomFactory : public IH5DataFactory
{
public:
  TriangleGeomFactory();

  ~TriangleGeomFactory() override;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override;

  /**
   * @brief Creates and adds an HexahedralGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param groupReader Wrapper around the HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readDataStructureGroup(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId = {}) override;

  /**
   * @brief Reads an HDF5 Dataset that makes up a DataStructure node.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param datasetReader Wrapper around the HDF5 dataset.
   * @param parentId = {} The HDF5 ID of the parent object.
   * @return
   */
  H5::ErrorType readDataStructureDataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId = {}) override;
};
} // namespace complex
