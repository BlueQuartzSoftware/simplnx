#pragma once

#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

namespace complex
{
class COMPLEX_EXPORT EdgeGeomFactory : public IH5DataFactory
{
public:
  EdgeGeomFactory();

  ~EdgeGeomFactory() override;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override;

  /**
   * @brief Creates and adds an HexahedralGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param ds DataStructure to add the created geometry to.
   * @param groupReader Wrapper around HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId = {}) override;

  /**
   * @brief Reads an HDF5 Dataset that makes up a DataStructure node.
   * @param ds The DataStructure object
   * @param datasetReader Wrapper around the HDF5 dataset.
   * @param parentId The HDF5 ID of the parent object.
   * @return
   */
  H5::ErrorType readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId = {}) override;
};
} // namespace complex
