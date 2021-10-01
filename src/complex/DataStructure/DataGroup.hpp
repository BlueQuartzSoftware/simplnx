#pragma once

#include "complex/DataStructure/BaseGroup.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class DataGroup
 * @brief The DataGroup class is an instantiable implementation of BaseGroup.
 * The DataGroup class does not impose restrictions on which types of
 * DataObject can be inserted.
 */
class COMPLEX_EXPORT DataGroup : public BaseGroup
{
public:
  /**
   * @brief Attempts to construct and insert a DataGroup into the DataStructure.
   * If a parentId is provided, then the DataGroup is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStucture will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the DataGroup.
   *
   * Returns a pointer to the DataGroup if the process succeeds. Returns
   * nullptr otherwise.
   * @param ds
   * @param name
   * @param parentId = {}
   * @return DataGroup*
   */
  static DataGroup* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Constructs a shallow copy of the DataGroup. This copy is not added
   * to the DataStructure by default.
   * @param other
   */
  DataGroup(const DataGroup& other);

  /**
   * @brief Constructs a DataGroup and moves values from the specified target.
   * @param other
   */
  DataGroup(DataGroup&& other) noexcept;

  ~DataGroup() override;

  /**
   * @brief Creates and returns a deep copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Creates and returns a shallow copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed
   * as a copy cannot be added to the DataStructure anywhere the original
   * exists without changing its name.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief Reads the DataStructure group from a target HDF5 group.
   * @param groupReader
   * @return H5::Error
   */
  H5::ErrorType readHdf5(const H5::GroupReader& groupReader) override;

protected:
  /**
   * @brief Creates the DataGroup for the target DataStructure and with the
   * specified name.
   * @param ds
   * @param name
   */
  DataGroup(DataStructure& ds, const std::string& name);

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * Returns true if the DataObject can be added to the container. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

  /**
   * @brief Writes the DataObject to the target HDF5 group.
   * @param parentGroupWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::GroupWriter& parentGroupWriter) const override;
};
} // namespace complex
