#pragma once

#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace nx::core
{
class GridMontage;

namespace HDF5
{
/**
 * @brief The GridMontageIO class serves as a reader and writer for GridMontages using HDF5.
 */
class SIMPLNX_EXPORT GridMontageIO : public BaseGroupIO
{
public:
  using data_type = GridMontage;

  /**
   * @brief Default constructor.
   */
  GridMontageIO();
  ~GridMontageIO() noexcept override;

  /**
   * @brief Attempts to read the GridMontage from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param montageName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& montageName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  /**
   * @brief Attempts to write the GridMontage to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureWriter
   * @param montage
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const GridMontage& montage, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a GridMontage.
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

  GridMontageIO(const GridMontageIO& other) = delete;
  GridMontageIO(GridMontageIO&& other) = delete;
  GridMontageIO& operator=(const GridMontageIO& rhs) = delete;
  GridMontageIO& operator=(GridMontageIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
