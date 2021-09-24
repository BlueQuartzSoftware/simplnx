#pragma once

#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

namespace complex
{
class COMPLEX_EXPORT HexahedralGeomFactory : public IH5DataFactory
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
   * @param ds DataStructure to add the created geometry to.
   * @param targetId ID for the target HDF5 object.
   * @param groupId ID for the parent HDF5 group.
   * @param parentId Optional DataObject ID describing which parent object to
   * create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId = {}) override;

  /**
   * @brief Reads an HDF5 Dataset that makes up a DataStructure node.
   * @param ds The DataStructure object
   * @param h5LocationId The HDF5 location id
   * @param h5DatasetName  The name of the dataset
   * @param parentId The HDF5 ID of the parent object.
   * @return
   */
  H5::ErrorType readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId = {}) override;
};
} // namespace complex
