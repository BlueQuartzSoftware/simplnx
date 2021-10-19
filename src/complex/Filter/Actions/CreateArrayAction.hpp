#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating DataArrays in a DataStructure
 */
class COMPLEX_EXPORT CreateArrayAction : public IDataAction
{
public:
  CreateArrayAction() = delete;

  CreateArrayAction(NumericType type, const std::vector<usize>& dims, uint64 nComp, const DataPath& path);

  ~CreateArrayAction() noexcept override;

  CreateArrayAction(const CreateArrayAction&) = delete;
  CreateArrayAction(CreateArrayAction&&) noexcept = delete;
  CreateArrayAction& operator=(const CreateArrayAction&) = delete;
  CreateArrayAction& operator=(CreateArrayAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the NumericType of the DataArray to be created.
   * @return
   */
  NumericType type() const;

  /**
   * @brief Returns the dimensions of the DataArray to be created.
   * @return
   */
  std::vector<usize> dims() const;

  /**
   * @brief Returns the number of components of the DataArray to be created.
   * @return
   */
  uint64 numComponents() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return
   */
  DataPath path() const;

private:
  NumericType m_Type;
  std::vector<usize> m_Dims;
  uint64 m_NComp;
  DataPath m_Path;
};
} // namespace complex
