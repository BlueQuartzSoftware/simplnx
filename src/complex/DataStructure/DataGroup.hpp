#pragma once

#include "complex/DataStructure/BaseGroup.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class DataContainer
 * @brief The DataGroup class is instantiable implementation of BaseGroup.
 * The DataGroup class does not impose restrictions on which types of
 * DataObject.
 */
class COMPLEX_EXPORT DataGroup : public BaseGroup
{
public:
  static DataGroup* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor
   * @param other
   */
  DataGroup(const DataGroup& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataGroup(DataGroup&& other) noexcept;

  virtual ~DataGroup();

  /**
   * @brief Creates and returns a deep copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Creates and returns a shallow copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
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
   * @param targetId
   * @param parentId
   * @return H5::Error
  */
  H5::ErrorType readHdf5(H5::IdType targetId, H5::IdType parentId) override;

  /**
   * @brief Writes the DataObject to the target HDF5 group.
   * @param parentId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const override;

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

private:
};
} // namespace complex
