#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingOperationTypes.hpp"

#include <optional>
#include <utility>

namespace nx::core::ImageProcessing::detail
{
namespace
{
std::optional<ConnectedComponentLabelLimitHandling>& lastLabelPolicy() noexcept
{
  static thread_local std::optional<ConnectedComponentLabelLimitHandling> lastPolicy;
  return lastPolicy;
}
} // namespace

void RecordConnectedComponentPolicyForTesting(ConnectedComponentLabelLimitHandling handling) noexcept
{
  lastLabelPolicy() = handling;
}

std::optional<ConnectedComponentLabelLimitHandling> TakeConnectedComponentPolicyForTesting() noexcept
{
  return std::exchange(lastLabelPolicy(), std::nullopt);
}
} // namespace nx::core::ImageProcessing::detail
