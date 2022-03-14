#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action that will copy a Source DataArray to a Destination DataPath.
 */
class COMPLEX_EXPORT CopyArrayInstanceAction : public IDataCreationAction
{
public:
  CopyArrayInstanceAction() = delete;

  CopyArrayInstanceAction(const DataPath& selectedDataPath, const DataPath& createdDataPath);

  ~CopyArrayInstanceAction() noexcept override;

  CopyArrayInstanceAction(const CopyArrayInstanceAction&) = delete;
  CopyArrayInstanceAction(CopyArrayInstanceAction&&) noexcept = delete;
  CopyArrayInstanceAction& operator=(const CopyArrayInstanceAction&) = delete;
  CopyArrayInstanceAction& operator=(CopyArrayInstanceAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief The selecte data array path to be copied
   * @return DataPath
   */
  DataPath selectedDataPath() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return DataPath
   */
  DataPath createdDataPath() const;

private:
  DataPath m_SelectedDataPath;
};
} // namespace complex
