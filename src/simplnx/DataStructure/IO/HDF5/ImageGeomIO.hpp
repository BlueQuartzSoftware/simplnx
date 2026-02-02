#pragma once

#include "simplnx/DataStructure/IO/HDF5/IGridGeometryIO.hpp"

namespace nx::core
{
class ImageGeom;

namespace HDF5
{
class SIMPLNX_EXPORT ImageGeomIO : public IGridGeometryIO
{
public:
  using data_type = ImageGeom;

  /**
   * @brief Default constructor.
   */
  ImageGeomIO();
  ~ImageGeomIO() noexcept override;

  /**
   * @brief Attempts to read the ImageGeom from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param geomName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& geomName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  /**
   * @brief Attempts to write an ImageGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const ImageGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to an ImageGeom.
   * Otherwise, this method returns writeData(...)
   * @param dataStructureWriter
   * @param dataObject
   * @param parentWriter
   * @return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  /**
   * @brief Returns the DataObject::Type for this IO class.
   * @return DataObject::Type The type identifier
   */
  DataObject::Type getDataType() const override;

  /**
   * @brief Returns the DataObject type name as a string for this IO class.
   * @return std::string The type name
   */
  std::string getTypeName() const override;

  ImageGeomIO(const ImageGeomIO& other) = delete;
  ImageGeomIO(ImageGeomIO&& other) = delete;
  ImageGeomIO& operator=(const ImageGeomIO& rhs) = delete;
  ImageGeomIO& operator=(ImageGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
