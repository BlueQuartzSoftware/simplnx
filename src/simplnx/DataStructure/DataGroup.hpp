#pragma once

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class DataGroup
 * @brief The DataGroup class is an instantiable implementation of BaseGroup.
 * The DataGroup class does not impose restrictions on which types of
 * DataObject can be inserted.
 */
class SIMPLNX_EXPORT DataGroup : public BaseGroup
{
public:
  static constexpr StringLiteral k_TypeName = "DataGroup";

  /**
   * @brief Attempts to construct and insert a DataGroup into the DataStructure.
   * If a parentId is provided, then the DataGroup is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStucture will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the DataGroup.
   *
   * Returns a pointer to the DataGroup if the process succeeds. Returns
   * nullptr otherwise.
   * @param dataStructure The DataStructure to add the DataGroup to
   * @param name The name for the DataGroup
   * @param parentId Optional parent ID for hierarchical organization
   * @return Pointer to the created DataGroup, or nullptr if creation failed
   */
  static DataGroup* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Attempts to construct and insert a DataGroup into the DataStructure.
   * If a parentId is provided, then the DataGroup is created with the
   * corresponding BaseGroup as its parent. Otherwise, the DataStucture will be
   * used as the parent object. In either case, the DataStructure will take
   * ownership of the DataGroup.
   *
   * Unlike Create, Import allows setting the DataObject ID for use in
   * importing data.
   *
   * Returns a pointer to the DataGroup if the process succeeds. Returns
   * nullptr otherwise.
   * @param dataStructure The DataStructure to add the DataGroup to
   * @param name The name for the DataGroup
   * @param importId The ID to assign to the imported DataGroup
   * @param parentId Optional parent ID for hierarchical organization
   * @return Pointer to the imported DataGroup, or nullptr if import failed
   */
  static DataGroup* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Constructs a shallow copy of the DataGroup. This copy is not added
   * to the DataStructure by default.
   * @param other The DataGroup to copy from
   */
  DataGroup(const DataGroup& other);

  /**
   * @brief Constructs a DataGroup and moves values from the specified target.
   * @param other The DataGroup to move from
   */
  DataGroup(DataGroup&& other);

  ~DataGroup() override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return The DataObject type enumeration value
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return The GroupType enumeration value
   */
  GroupType getGroupType() const override;

  /**
   * @brief Creates and returns a deep copy of the DataGroup at the specified path.
   * @param copyPath The DataPath where the deep copy will be placed
   * @return Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Creates and returns a shallow copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed
   * as a copy cannot be added to the DataStructure anywhere the original
   * exists without changing its name.
   * @return Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return String representation of the DataObject type name
   */
  std::string getTypeName() const override;

protected:
  /**
   * @brief Protected constructor creates the DataGroup for the target DataStructure and with the
   * specified name.
   * @param dataStructure The DataStructure that will own this DataGroup
   * @param name The name for the DataGroup
   */
  DataGroup(DataStructure& dataStructure, std::string name);

  /**
   * @brief Protected constructor creates the DataGroup for the target DataStructure with the
   * specified name and import ID.
   * @param dataStructure The DataStructure that will own this DataGroup
   * @param name The name for the DataGroup
   * @param importId The ID to assign to this DataGroup
   */
  DataGroup(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * Returns true if the DataObject can be added to the container. Otherwise,
   * returns false.
   * @param obj Pointer to the DataObject to check for insertion
   * @return True if the object can be inserted, false otherwise
   */
  bool canInsert(const DataObject* obj) const override;
};
} // namespace nx::core
