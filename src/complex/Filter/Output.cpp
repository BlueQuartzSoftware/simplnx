#include "Output.hpp"

using namespace complex;

namespace complex
{
Result<> OutputActions::ApplyActions(nonstd::span<const AnyDataAction> actions, DataStructure& dataStructure, IDataAction::Mode mode)
{
  std::vector<Error> errors;
  std::vector<Warning> warnings;
  for(const auto& action : actions)
  {
    Result<> actionResult = action->apply(dataStructure, mode);
    if(!ExtractResult(std::move(actionResult), errors, warnings))
    {
      break;
    }
  }
  Result<> result = errors.empty() ? Result<>{} : Result<>{nonstd::make_unexpected(std::move(errors))};
  result.warnings() = std::move(warnings);
  return result;
}

Result<> OutputActions::applyRegular(DataStructure& dataStructure, IDataAction::Mode mode) const
{
  return ApplyActions(actions, dataStructure, mode);
}

Result<> OutputActions::applyDeferred(DataStructure& dataStructure, IDataAction::Mode mode) const
{
  return ApplyActions(deferredActions, dataStructure, mode);
}

Result<> OutputActions::applyAll(DataStructure& dataStructure, IDataAction::Mode mode) const
{
  Result<> regularActionsResult = applyRegular(dataStructure, mode);
  if(regularActionsResult.invalid())
  {
    return regularActionsResult;
  }
  Result<> deferredActionsResult = applyDeferred(dataStructure, mode);
  return MergeResults(std::move(regularActionsResult), std::move(deferredActionsResult));
}
} // namespace complex
