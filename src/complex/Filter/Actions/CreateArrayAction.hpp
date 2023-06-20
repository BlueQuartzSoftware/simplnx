#pragma once

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

#include <string>
#include <vector>

namespace complex
{
/**
 * @brief Action for creating DataArrays in a DataStructure
 */
class COMPLEX_EXPORT CreateArrayAction : public IDataCreationAction
{
public:
  CreateArrayAction() = delete;

  CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat = "");

  ~CreateArrayAction() noexcept override;

  CreateArrayAction(const CreateArrayAction&) = delete;
  CreateArrayAction(CreateArrayAction&&) noexcept = delete;
  CreateArrayAction& operator=(const CreateArrayAction&) = delete;
  CreateArrayAction& operator=(CreateArrayAction&&) noexcept = delete;

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
  const std::vector<usize>& componentDims() const;

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
   * @brief Returns the data formatting name for use in creating the appropriate data store.
   * An empty string results in creating an in-memory DataStore.
   * Other formats must be defined in external plugins.
   * @return std::string
   */
  std::string dataFormat() const;

private:
  DataType m_Type;
  std::vector<usize> m_Dims;
  std::vector<usize> m_CDims;
  std::string m_DataFormat = "";
};
} // namespace complex
