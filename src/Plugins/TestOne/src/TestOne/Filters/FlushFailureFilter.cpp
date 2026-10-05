#include "FlushFailureFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"

#include <fmt/format.h>

#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

using namespace nx::core;

namespace
{
/**
 * @class CliCompletionStore
 * @brief Returns a selected checked error without requiring private storage code.
 */
class CliCompletionStore : public DataStore<int32>
{
public:
  /**
   * @brief Creates the literal two-value carrier.
   * @param mode Selects ordinary or allocation-category checked failure.
   */
  explicit CliCompletionStore(uint64 mode)
  : DataStore<int32>(ShapeType{2}, ShapeType{1}, int32{0})
  , m_Mode(mode)
  {
    setValue(0, 11);
    setValue(1, 13);
  }

  /**
   * @brief Reports a checked-only completion fault while leaving legacy flush harmless.
   * @return The selected checked result with a distinct warning.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    if(m_Mode == 1 || m_Mode == 2 || m_Mode == 3)
    {
      auto result = MakeErrorResult(m_Mode == 3 ? -272 : -76402, "C2-final-flush-error: cli-backing.h5:/Values");
      result.warnings().push_back({76403, "C2-final-flush-warning"});
      return result;
    }
    return {};
  }

private:
  uint64 m_Mode;
};

/**
 * @struct CliSnapshotControl
 * @brief Retains the End connection without a cycle to its owning data objects.
 */
struct CliSnapshotControl
{
  bool armed = false;
  bool allocation = false;
  nod::scoped_connection endConnection;
};

/**
 * @class CliSnapshotGroup
 * @brief Throws only during an armed shallow-copy operation.
 */
class CliSnapshotGroup : public DataGroup
{
public:
  /**
   * @brief Creates an insertable group with shared fault state.
   * @param dataStructure Owns the group after insertion.
   * @param control Arms an ordinary or simulated allocation copy error.
   */
  CliSnapshotGroup(DataStructure& dataStructure, std::shared_ptr<CliSnapshotControl> control)
  : DataGroup(dataStructure, "CompletionSnapshot")
  , m_Control(std::move(control))
  {
  }

  /**
   * @brief Preserves the control in an unarmed shallow copy.
   * @param other Supplies identity and the shared control.
   */
  CliSnapshotGroup(const CliSnapshotGroup& other)
  : DataGroup(other)
  , m_Control(other.m_Control)
  {
  }

  /**
   * @brief Copies the group or raises the selected snapshot exception.
   * @return A caller-owned shallow copy when unarmed.
   * @throws std::bad_alloc If an allocation fault is armed.
   * @throws std::runtime_error If an ordinary fault is armed.
   */
  DataObject* shallowCopy() override
  {
    if(m_Control->armed)
    {
      if(m_Control->allocation)
      {
        throw std::bad_alloc{};
      }
      throw std::runtime_error("C2-copy-error: CLI completion snapshot");
    }
    return new CliSnapshotGroup(*this);
  }

private:
  std::shared_ptr<CliSnapshotControl> m_Control;
};
} // namespace

std::string FlushFailureFilter::name() const
{
  return FilterTraits<FlushFailureFilter>::name;
}
std::string FlushFailureFilter::className() const
{
  return FilterTraits<FlushFailureFilter>::className;
}
Uuid FlushFailureFilter::uuid() const
{
  return FilterTraits<FlushFailureFilter>::uuid;
}
std::string FlushFailureFilter::humanName() const
{
  return "Completion Failure Test Fixture";
}

//------------------------------------------------------------------------------
std::vector<std::string> FlushFailureFilter::defaultTags() const
{
  return {className(), "Example", "Test"};
}

IFilter::VersionType FlushFailureFilter::parametersVersion() const
{
  return 1;
}

Parameters FlushFailureFilter::parameters() const
{
  Parameters result;
  result.insert(std::make_unique<UInt64Parameter>(
      k_Mode_Key, "Completion Test Mode", "0: unarmed; 1: flush error; 2: execute and flush errors; 3: flush allocation; 4: repeated snapshot; 5: root snapshot; 6: snapshot allocation.", uint64{0}));
  return result;
}

IFilter::UniquePointer FlushFailureFilter::clone() const
{
  return std::make_unique<FlushFailureFilter>();
}

IFilter::PreflightResult FlushFailureFilter::preflightImpl([[maybe_unused]] const DataStructure& dataStructure, const Arguments& filterArgs, [[maybe_unused]] const MessageHandler& messageHandler,
                                                           [[maybe_unused]] const std::atomic_bool& shouldCancel, [[maybe_unused]] const ExecutionContext& executionContext) const
{
  const auto mode = filterArgs.value<uint64>(k_Mode_Key);
  if(mode > 6)
  {
    return MakePreflightErrorResult(-76409, fmt::format("Completion Test Mode {} is outside the fixture range [0, 6].", mode));
  }
  PreflightResult result{};
  result.outputValues.push_back({"Completion value", "preserved"});
  return result;
}

Result<> FlushFailureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         [[maybe_unused]] const std::atomic_bool& shouldCancel, [[maybe_unused]] const ExecutionContext& executionContext) const
{
  const auto mode = filterArgs.value<uint64>(k_Mode_Key);
  messageHandler.sendInfoMessage("C2 fixture UUID: 2a98fc79-c485-45d5-8fa9-1f298f3d6729");
  auto store = std::make_shared<CliCompletionStore>(mode);
  if(DataArray<int32>::Create(dataStructure, "Values", store) == nullptr)
  {
    return MakeErrorResult(-76409, "Cannot insert CLI completion fixture array 'Values'.");
  }
  if(mode >= 4)
  {
    auto control = std::make_shared<CliSnapshotControl>();
    control->allocation = mode == 6;
    control->armed = mode != 5;
    if(!dataStructure.insert(std::make_shared<CliSnapshotGroup>(dataStructure, control), DataPath{}))
    {
      return MakeErrorResult(-76409, "Cannot insert CLI completion fixture group 'CompletionSnapshot'.");
    }
    if(mode == 5)
    {
      auto* parent = pipelineNode == nullptr ? nullptr : pipelineNode->getParentPipeline();
      bool connected = false;
      if(parent != nullptr)
      {
        for(const auto& node : *parent)
        {
          if(node.get() == pipelineNode)
          {
            const std::weak_ptr<CliSnapshotControl> weakControl = control;
            control->endConnection = node->getFilterUpdateSignal().connect([weakControl](AbstractPipelineNode*, int32, const std::string& message) {
              if(message == "End")
              {
                if(auto state = weakControl.lock())
                {
                  state->armed = true;
                }
              }
            });
            connected = true;
            break;
          }
        }
      }
      if(!connected)
      {
        return MakeErrorResult(-76409, "Root snapshot fixture requires its actual parent PipelineFilter.");
      }
    }
  }
  auto result = mode == 2 ? MakeErrorResult(-76401, "C2-original-execute-error") : Result<>{};
  result.warnings().push_back({76400, "C2-original-execute-warning"});
  return result;
}
