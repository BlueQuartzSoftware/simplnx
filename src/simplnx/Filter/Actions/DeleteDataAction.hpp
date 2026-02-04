#pragma once

#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for deleting DataObjects in a DataStructure
 */
class SIMPLNX_EXPORT DeleteDataAction : public IDataAction
{
public:
  enum class DeleteType : uint64
  {
    JustObject = 0,
    // IndependentChildren = 1,
    // AllChildren = 2,
    // JustPath = 3
  };

  DeleteDataAction() = delete;

  /**
   * @brief Constructs a DeleteDataAction.
   * @param path The path to the DataObject to delete
   * @param type The deletion type
   */
  DeleteDataAction(const DataPath& path, DeleteType type = DeleteType::JustObject);

  ~DeleteDataAction() noexcept override;

  DeleteDataAction(const DeleteDataAction&) = delete;
  DeleteDataAction(DeleteDataAction&&) noexcept = delete;
  DeleteDataAction& operator=(const DeleteDataAction&) = delete;
  DeleteDataAction& operator=(DeleteDataAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns the type to be used
   * @return DeleteType
   */
  DeleteType type() const;

private:
  DataPath m_Path;
  DeleteType m_Type;
};
} // namespace nx::core
