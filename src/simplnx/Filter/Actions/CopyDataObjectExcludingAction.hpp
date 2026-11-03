#pragma once

#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Copies an object without any links to a selected object. Source links are restored even if copying throws.
 * If excludedPath does not resolve, this is a plain CopyDataObjectAction.
 * @pre allCreatedPaths omits the aliases of the excluded object and their descendants.
 */
class SIMPLNX_EXPORT CopyDataObjectExcludingAction : public CopyDataObjectAction
{
public:
  CopyDataObjectExcludingAction() = delete;
  CopyDataObjectExcludingAction(const DataPath& source, const DataPath& destination, const std::vector<DataPath>& allCreatedPaths, DataPath excludedPath);
  ~CopyDataObjectExcludingAction() noexcept override;
  CopyDataObjectExcludingAction(const CopyDataObjectExcludingAction&) = delete;
  CopyDataObjectExcludingAction(CopyDataObjectExcludingAction&&) noexcept = delete;
  CopyDataObjectExcludingAction& operator=(const CopyDataObjectExcludingAction&) = delete;
  CopyDataObjectExcludingAction& operator=(CopyDataObjectExcludingAction&&) noexcept = delete;

  Result<> apply(DataStructure& dataStructure, Mode mode) const override;
  UniquePointer clone() const override;

private:
  DataPath m_ExcludedPath;
};
} // namespace nx::core
