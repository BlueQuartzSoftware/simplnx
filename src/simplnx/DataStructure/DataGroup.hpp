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
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return DataGroup*
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
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return DataGroup*
   */
  static DataGroup* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

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
  DataGroup(DataGroup&& other);

  ~DataGroup() override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Creates and returns a deep copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

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

protected:
  /**
   * @brief Creates the DataGroup for the target DataStructure and with the
   * specified name.
   * @param dataStructure
   * @param name
   */
  DataGroup(DataStructure& dataStructure, std::string name);

  /**
   * @brief Creates the DataGroup for the target DataStructure and with the
   * specified name.
   * @param dataStructure
   * @param name
   * @param importId
   */
  DataGroup(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * Returns true if the DataObject can be added to the container. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  Result<bool> canInsert(const DataObject* obj) const override;
};
} // namespace nx::core
