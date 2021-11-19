#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class COMPLEX_EXPORT EmptyAction : public IDataAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  EmptyAction() = default;

  ~EmptyAction() noexcept override;

  EmptyAction(const EmptyAction&) = delete;
  EmptyAction(EmptyAction&&) noexcept = delete;
  EmptyAction& operator=(const EmptyAction&) = delete;
  EmptyAction& operator=(EmptyAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

private:
  DataPath m_Path;
  DimensionType m_Dims;
  OriginType m_Origin;
  SpacingType m_Spacing;
};
} // namespace complex
