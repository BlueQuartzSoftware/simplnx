#pragma once

#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for creating DataArrays in a DataStructure
 */
class SIMPLNX_EXPORT CreateStringArrayAction : public IDataCreationAction
{
public:
  CreateStringArrayAction() = delete;

  CreateStringArrayAction(const std::vector<usize>& tDims, const DataPath& path, const std::string& initializeValue = "");

  ~CreateStringArrayAction() noexcept override;

  CreateStringArrayAction(const CreateStringArrayAction&) = delete;
  CreateStringArrayAction(CreateStringArrayAction&&) noexcept = delete;
  CreateStringArrayAction& operator=(const CreateStringArrayAction&) = delete;
  CreateStringArrayAction& operator=(CreateStringArrayAction&&) noexcept = delete;

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
   * @brief Returns the dimensions of the DataArray to be created.
   * @return const std::vector<usize>&
   */
  const std::vector<usize>& dims() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return const DataPath&
   */
  DataPath path() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  std::vector<usize> m_Dims;
  std::string m_InitializeValue;
};
} // namespace nx::core
