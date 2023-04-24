#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for deleting DataObjects in a DataStructure
 */
class COMPLEX_EXPORT DeleteDataAction : public IDataAction
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

  DeleteDataAction(const DataPath& path, DeleteType type = DeleteType::JustObject);

  ~DeleteDataAction() noexcept override;

  DeleteDataAction(const DeleteDataAction&) = delete;
  DeleteDataAction(DeleteDataAction&&) noexcept = delete;
  DeleteDataAction& operator=(const DeleteDataAction&) = delete;
  DeleteDataAction& operator=(DeleteDataAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
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
} // namespace complex
