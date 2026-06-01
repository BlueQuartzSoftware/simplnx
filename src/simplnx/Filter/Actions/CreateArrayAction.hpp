#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Output.hpp"

#include <string>
#include <vector>

namespace nx::core
{
/**
 * @brief Action for creating DataArrays in a DataStructure
 */
class SIMPLNX_EXPORT CreateArrayAction : public IDataCreationAction
{
public:
  CreateArrayAction() = delete;

  /**
   * @brief Constructs a CreateArrayAction.
   * @param type The data type of the array
   * @param tDims The tuple dimensions
   * @param cDims The component dimensions
   * @param path The path where the DataArray will be created
   * @param dataFormat The data store format override. Empty string means "Automatic"
   *                   (let the format resolver decide). A non-empty value bypasses the
   *                   resolver and uses the specified format directly.
   * @param fillValue The fill value for the array
   */
  CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat = "", std::string fillValue = "");

  ~CreateArrayAction() noexcept override;

  CreateArrayAction(const CreateArrayAction&) = delete;
  CreateArrayAction(CreateArrayAction&&) noexcept = delete;
  CreateArrayAction& operator=(const CreateArrayAction&) = delete;
  CreateArrayAction& operator=(CreateArrayAction&&) noexcept = delete;

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
   * @brief Returns the DataType of the DataArray to be created.
   * @return DataType
   */
  DataType type() const;

  /**
   * @brief Returns the dimensions of the DataArray to be created.
   * @return const std::vector<usize>&
   */
  const std::vector<usize>& dims() const;

  /**
   * @brief Returns the component dimensions of the DataArray to be created.
   * @return const std::vector<usize>&
   */
  const ShapeType& componentDims() const;

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

  /**
   * @brief Returns the fill value of the DataArray to be created.
   * @return std::string
   */
  std::string fillValue() const;

  /**
   * @brief Returns the data store format override for this action.
   *
   * Empty string means "Automatic" -- the format resolver decides. A non-empty
   * value bypasses the resolver and uses the specified format directly, allowing
   * individual filters to override the global format policy.
   *
   * @return The data format string
   */
  std::string dataFormat() const;

private:
  DataType m_Type;
  std::vector<usize> m_Dims;
  std::vector<usize> m_CDims;
  std::string m_DataFormat = "";
  std::string m_FillValue = "";
};
} // namespace nx::core
