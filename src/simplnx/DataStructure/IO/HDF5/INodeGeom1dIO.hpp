#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/INodeGeom0dIO.hpp"

namespace nx::core
{
class INodeGeometry1D;

namespace HDF5
{
class SIMPLNX_EXPORT INodeGeom1dIO : public INodeGeom0dIO
{
public:
  /**
   * @brief Protected constructor.
   */
  INodeGeom1dIO();
  ~INodeGeom1dIO() noexcept override;

  INodeGeom1dIO(const INodeGeom1dIO& other) = delete;
  INodeGeom1dIO(INodeGeom1dIO&& other) = delete;
  INodeGeom1dIO& operator=(const INodeGeom1dIO& rhs) = delete;
  INodeGeom1dIO& operator=(INodeGeom1dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the INodeGeometry1D data from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param geometry
   * @param parentGroup
   * @param geomName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  static Result<> ReadNodeGeom1dData(DataStructureReader& dataStructureReader, INodeGeometry1D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the INodeGeometry1D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom1dData(DataStructureWriter& dataStructureWriter, const INodeGeometry1D& geometry, group_writer_type& parentGroup, bool importable);

  /**
   * @brief Writes or removes one connectivity ID attribute of a geometry group.
   * @param dataStructureWriter Writer that knows which connectivity caches it omits.
   * @param groupWriter The geometry's HDF5 group.
   * @param objectId The geometry's vertex-incidence or element-neighbors association.
   * @param tag Attribute name for that association.
   * @return Attribute write or removal errors.
   *
   * A set association to an object that is written stores its ID. Unset and omitted associations leave no
   * attribute. An existing attribute of that name, for example in a group written earlier, is removed. A reload
   * then cannot restore a stale reference.
   */
  static Result<> WriteConnectivityId(const DataStructureWriter& dataStructureWriter, group_writer_type& groupWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag);

  /**
   * @brief Finishes importing 1D node geometry data after the geometry has been created.
   * @param dataStructure The DataStructure containing the imported geometry
   * @param dataPath The path to the imported geometry
   * @param dataStructureGroup The HDF5 group containing the geometry data
   * @return Result<> Result with any errors or warnings
   */
  static Result<> FinishImportingNodeGeom1dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
