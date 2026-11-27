#pragma once

#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Deep-copies a subtree and removes copies of the excluded object by matching its source ID.
 * The source links stay intact. An unresolved exclusion path gives a plain copy.
 * @pre The source path is outside the excluded subtree and allCreatedPaths omits each excluded placement and its descendants.
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
  UniquePointerType clone() const override;

private:
  DataPath m_ExcludedPath;
};
} // namespace nx::core
