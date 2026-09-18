#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_ThrowMessage = "boom";
constexpr int32 k_ExecuteErrorCode = -9975;
constexpr StringLiteral k_ExecuteErrorMessage = "the store rejected the write";
constexpr int32 k_PreflightErrorCode = -9976;
constexpr int32 k_PreflightWarningCode = -9977;
constexpr int32 k_DeferredErrorCode = -9978;
constexpr int32 k_DeferredWarningCode = -9979;
constexpr StringLiteral k_PreflightValueName = "Preflight detail";
constexpr StringLiteral k_PreflightValueText = "preserved";

/**
 * @struct NonStandardException
 * @brief An exception type that does not derive from std::exception.
 *
 * HDF5's C++ API, and several third party libraries simplnx links, throw types outside the
 * std::exception hierarchy. This type stands in for them.
 */
struct NonStandardException
{
};

/**
 * @class BackstopFilterBase
 * @brief Supplies the IFilter boilerplate the execute() backstop tests do not exercise.
 * @tparam DerivedT The concrete filter. It must declare k_FilterName and k_FilterUuid.
 *
 * Each test filter differs only in its executeImpl, so the identity and parameter methods
 * live here instead of being repeated in every filter.
 */
template <class DerivedT>
class BackstopFilterBase : public IFilter
{
public:
  BackstopFilterBase() = default;
  ~BackstopFilterBase() noexcept override = default;

  BackstopFilterBase(const BackstopFilterBase&) = delete;
  BackstopFilterBase(BackstopFilterBase&&) noexcept = delete;
  BackstopFilterBase& operator=(const BackstopFilterBase&) = delete;
  BackstopFilterBase& operator=(BackstopFilterBase&&) noexcept = delete;

  /**
   * @brief Returns the filter name used in the backstop error message.
   * @return Filter name.
   */
  std::string name() const override
  {
    return DerivedT::k_FilterName.str();
  }

  /**
   * @brief Returns the C++ class name of this filter.
   * @return Class name.
   */
  std::string className() const override
  {
    return DerivedT::k_FilterName.str();
  }

  /**
   * @brief Returns the identifier of this filter.
   * @return A fixed test-only uuid.
   */
  Uuid uuid() const override
  {
    return *Uuid::FromString(DerivedT::k_FilterUuid.view());
  }

  /**
   * @brief Returns the human readable name of this filter.
   * @return Human name.
   */
  std::string humanName() const override
  {
    return DerivedT::k_FilterName.str();
  }

  /**
   * @brief Returns the parameters of this filter.
   * @return An empty parameter set because the backstop needs no inputs.
   */
  Parameters parameters() const override
  {
    return {};
  }

  /**
   * @brief Returns the version of this filter's parameter set.
   * @return The initial version.
   */
  VersionType parametersVersion() const override
  {
    return 1;
  }

  /**
   * @brief Returns a copy of this filter.
   * @return An owning pointer to the copy.
   */
  UniquePointer clone() const override
  {
    return std::make_unique<DerivedT>();
  }

protected:
  /**
   * @brief Reports a valid preflight so execute() reaches executeImpl.
   * @return An empty preflight result.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    return {};
  }
};

/**
 * @class ThrowingFilter
 * @brief A filter whose executeImpl always throws a std::runtime_error.
 *
 * IFilter::execute has to convert any exception that escapes a filter into an invalid result so a
 * pipeline reports the failure instead of terminating the process. This filter exercises that
 * backstop without depending on a plugin being loaded.
 */
class ThrowingFilter : public BackstopFilterBase<ThrowingFilter>
{
public:
  static inline constexpr StringLiteral k_FilterName = "ThrowingFilter";
  static inline constexpr StringLiteral k_FilterUuid = "1f0d5f4a-9f7d-4c67-9d1e-2a3b4c5d6e7f";

protected:
  /**
   * @brief Throws so the caller can observe how execute() handles an escaped exception.
   * @throws std::runtime_error Always.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    throw std::runtime_error(k_ThrowMessage.str());
  }
};

/**
 * @class NonStandardThrowingFilter
 * @brief A filter whose executeImpl throws a type outside the std::exception hierarchy.
 *
 * A catch(const std::exception&) handler alone lets such a throw reach std::terminate, which
 * kills the application without telling the user what failed.
 */
class NonStandardThrowingFilter : public BackstopFilterBase<NonStandardThrowingFilter>
{
public:
  static inline constexpr StringLiteral k_FilterName = "NonStandardThrowingFilter";
  static inline constexpr StringLiteral k_FilterUuid = "2c9e7b31-5a44-4f18-8c33-6d1e9b0a7f52";

protected:
  /**
   * @brief Throws a non-std::exception type.
   * @throws NonStandardException Always.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    throw NonStandardException{};
  }
};

/**
 * @class CancelAfterErrorFilter
 * @brief A filter that raises the cancellation flag and then reports a storage error.
 *
 * This is the shape of a real failure: the user presses Cancel while a store write is already
 * failing. The reported error has to stay the storage error, because "Filter cancelled" would
 * hide the cause.
 */
class CancelAfterErrorFilter : public BackstopFilterBase<CancelAfterErrorFilter>
{
public:
  static inline constexpr StringLiteral k_FilterName = "CancelAfterErrorFilter";
  static inline constexpr StringLiteral k_FilterUuid = "84b1c0de-3f27-4a6b-9e15-0c7d2f38ab41";

  /**
   * @brief Selects the flag executeImpl raises.
   * @param cancelFlag Supplies the same flag the caller passes to execute(). The caller must keep
   * it alive for the filter lifetime.
   */
  void setCancelFlag(std::atomic_bool* cancelFlag)
  {
    m_CancelFlag = cancelFlag;
  }

protected:
  /**
   * @brief Raises the cancellation flag and reports a storage error.
   * @return An error carrying k_ExecuteErrorCode.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    if(m_CancelFlag != nullptr)
    {
      m_CancelFlag->store(true);
    }
    return MakeErrorResult(k_ExecuteErrorCode, k_ExecuteErrorMessage.str());
  }

private:
  std::atomic_bool* m_CancelFlag = nullptr;
};

/**
 * @class CancellingPreflightFilter
 * @brief Sets cancellation while preflight returns controlled diagnostic data.
 *
 * The fixture verifies that cancellation does not hide an existing failure
 * or discard preflight metadata.
 */
class CancellingPreflightFilter : public BackstopFilterBase<CancellingPreflightFilter>
{
public:
  static inline constexpr StringLiteral k_FilterName = "CancellingPreflightFilter";
  static inline constexpr StringLiteral k_FilterUuid = "9fbd2765-2fab-4b51-b53a-3b940ea49df0";

  /**
   * @brief Selects the cancellation flag and preflight validity for the test.
   * @param cancelFlag Flag to set.
   * @param returnError Selects failure.
   * @param executionCount Execution count.
   */
  void configure(std::atomic_bool* cancelFlag, bool returnError, int32* executionCount)
  {
    m_CancelFlag = cancelFlag;
    m_ReturnError = returnError;
    m_ExecutionCount = executionCount;
  }

protected:
  /**
   * @brief Sets cancellation and returns controlled warnings, output values, and optional errors.
   * @param dataStructure Unused.
   * @param filterArgs Unused.
   * @param messageHandler Unused.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Unused.
   * @return Preflight data for cancellation-precedence tests.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    m_CancelFlag->store(true);

    Result<OutputActions> result;
    if(m_ReturnError)
    {
      result = MakeErrorResult<OutputActions>(k_PreflightErrorCode, "The controlled preflight operation failed");
    }
    result.warnings().push_back(Warning{k_PreflightWarningCode, "The controlled preflight warning"});
    return {std::move(result), {{k_PreflightValueName.str(), k_PreflightValueText.str()}}};
  }

  /**
   * @brief Records an unexpected call to executeImpl.
   * @param dataStructure Unused.
   * @param filterArgs Unused.
   * @param pipelineNode Unused.
   * @param messageHandler Unused.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Unused.
   * @return A valid result.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    (*m_ExecutionCount)++;
    return {};
  }

private:
  std::atomic_bool* m_CancelFlag = nullptr;
  bool m_ReturnError = false;
  int32* m_ExecutionCount = nullptr;
};

/**
 * @class FailingDeferredAction
 * @brief Returns a controlled error when a filter applies deferred actions.
 */
class FailingDeferredAction : public IDataAction
{
public:
  /**
   * @brief Constructs the action with a shared invocation counter.
   * @param invocationCount Counts calls across action clones.
   */
  explicit FailingDeferredAction(std::shared_ptr<int32> invocationCount)
  : m_InvocationCount(std::move(invocationCount))
  {
  }

  /**
   * @brief Returns the controlled deferred-action failure.
   * @param dataStructure Supplies the unused test data structure.
   * @param mode Supplies the action mode.
   * @return An error
   * and warning that identify this action.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    (*m_InvocationCount)++;
    Result<> result = MakeErrorResult(k_DeferredErrorCode, "The controlled deferred action failed");
    result.warnings().push_back(Warning{k_DeferredWarningCode, "The controlled deferred warning"});
    return result;
  }

  /**
   * @brief Copies the action while sharing the invocation counter.
   * @return A new action that reports through the same counter.
   */
  UniquePointer clone() const override
  {
    return std::make_unique<FailingDeferredAction>(m_InvocationCount);
  }

private:
  std::shared_ptr<int32> m_InvocationCount;
};

/**
 * @class RecordingDeferredAction
 * @brief Records whether deferred execution continues after a preceding error.
 */
class RecordingDeferredAction : public IDataAction
{
public:
  /**
   * @brief Constructs the action with a shared invocation counter.
   * @param invocationCount Counts calls across action clones.
   */
  explicit RecordingDeferredAction(std::shared_ptr<int32> invocationCount)
  : m_InvocationCount(std::move(invocationCount))
  {
  }

  /**
   * @brief Records one invocation.
   * @param dataStructure Supplies the unused test data structure.
   * @param mode Supplies the action mode.
   * @return A valid result.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    (*m_InvocationCount)++;
    return {};
  }

  /**
   * @brief Copies the action while sharing the invocation counter.
   * @return A new action that reports through the same counter.
   */
  UniquePointer clone() const override
  {
    return std::make_unique<RecordingDeferredAction>(m_InvocationCount);
  }

private:
  std::shared_ptr<int32> m_InvocationCount;
};

/**
 * @class DeferredFailureFilter
 * @brief Supplies one failing deferred action for execution-boundary testing.
 */
class DeferredFailureFilter : public BackstopFilterBase<DeferredFailureFilter>
{
public:
  static inline constexpr StringLiteral k_FilterName = "DeferredFailureFilter";
  static inline constexpr StringLiteral k_FilterUuid = "19b58b93-abd0-4f4e-91a4-e5202aac93f8";

  /**
   * @brief Constructs the filter with internal invocation counters.
   */
  DeferredFailureFilter()
  : DeferredFailureFilter(std::make_shared<int32>(0), std::make_shared<int32>(0))
  {
  }

  /**
   * @brief Constructs the filter with shared deferred-action invocation counters.
   * @param failingInvocationCount Failing action count.
   * @param followingInvocationCount Following action count.
   */
  DeferredFailureFilter(std::shared_ptr<int32> failingInvocationCount, std::shared_ptr<int32> followingInvocationCount)
  : m_FailingInvocationCount(std::move(failingInvocationCount))
  , m_FollowingInvocationCount(std::move(followingInvocationCount))
  {
  }

  /**
   * @brief Returns a copy that shares the deferred-action invocation counters.
   * @return An owning pointer to the copy.
   */
  UniquePointer clone() const override
  {
    return std::make_unique<DeferredFailureFilter>(m_FailingInvocationCount, m_FollowingInvocationCount);
  }

protected:
  /**
   * @brief Returns the failing deferred action and controlled preflight metadata.
   * @param dataStructure Unused.
   * @param filterArgs Unused.
   * @param messageHandler Unused.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Unused.
   * @return Deferred action, warning, and output value for the execution test.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    OutputActions actions;
    actions.appendDeferredAction(std::make_unique<FailingDeferredAction>(m_FailingInvocationCount));
    actions.appendDeferredAction(std::make_unique<RecordingDeferredAction>(m_FollowingInvocationCount));
    Result<OutputActions> result{std::move(actions)};
    result.warnings().push_back(Warning{k_PreflightWarningCode, "The controlled preflight warning"});
    return {std::move(result), {{k_PreflightValueName.str(), k_PreflightValueText.str()}}};
  }

  /**
   * @brief Allows execution to reach the deferred action.
   * @param dataStructure Unused.
   * @param filterArgs Unused.
   * @param pipelineNode Unused.
   * @param messageHandler Unused.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Unused.
   * @return A valid result.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    return {};
  }

private:
  std::shared_ptr<int32> m_FailingInvocationCount;
  std::shared_ptr<int32> m_FollowingInvocationCount;
};
} // namespace

TEST_CASE("IFilter: An exception thrown by executeImpl becomes an invalid result", "[simplnx][IFilter]")
{
  ThrowingFilter filter;
  DataStructure dataStructure;
  Arguments args;

  IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args);

  REQUIRE(executeResult.result.invalid());
  REQUIRE(!executeResult.result.errors().empty());

  const std::string& message = executeResult.result.errors().front().message;
  REQUIRE(StringUtilities::contains(message, filter.name()));
  REQUIRE(StringUtilities::contains(message, k_ThrowMessage.view()));
}

TEST_CASE("IFilter: A non-standard exception thrown by executeImpl becomes an invalid result", "[simplnx][IFilter]")
{
  NonStandardThrowingFilter filter;
  DataStructure dataStructure;
  Arguments args;

  IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args);

  REQUIRE(executeResult.result.invalid());
  REQUIRE(!executeResult.result.errors().empty());

  const Error& error = executeResult.result.errors().front();
  REQUIRE(error.code == -2);
  REQUIRE(StringUtilities::contains(error.message, filter.name()));
}

TEST_CASE("IFilter: A cancellation does not replace an execution error", "[simplnx][IFilter]")
{
  CancelAfterErrorFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::atomic_bool shouldCancel = false;
  filter.setCancelFlag(&shouldCancel);

  IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);

  REQUIRE(shouldCancel.load());
  REQUIRE(executeResult.result.invalid());
  REQUIRE(!executeResult.result.errors().empty());

  const Error& error = executeResult.result.errors().front();
  REQUIRE(error.code == k_ExecuteErrorCode);
  REQUIRE(StringUtilities::contains(error.message, k_ExecuteErrorMessage.view()));
}

TEST_CASE("IFilter: Preflight cancellation preserves diagnostics and output values", "[simplnx][IFilter]")
{
  CancellingPreflightFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insert("Unknown argument", std::make_any<int32>(1));
  std::atomic_bool shouldCancel = false;
  int32 executionCount = 0;

  SECTION("An existing preflight error takes precedence")
  {
    filter.configure(&shouldCancel, true, &executionCount);
    IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);

    REQUIRE(shouldCancel.load());
    REQUIRE(executionCount == 0);
    REQUIRE(executeResult.result.invalid());
    REQUIRE(executeResult.result.errors().size() == 1);
    REQUIRE(executeResult.result.errors().front().code == k_PreflightErrorCode);
    REQUIRE(executeResult.result.warnings().size() == 2);
    REQUIRE(executeResult.result.warnings().front().code == k_PreflightWarningCode);
    REQUIRE(executeResult.result.warnings()[1].code == -1);
    REQUIRE(executeResult.outputValues.size() == 1);
    REQUIRE(executeResult.outputValues.front().name == k_PreflightValueName.view());
    REQUIRE(executeResult.outputValues.front().value == k_PreflightValueText.view());
  }

  SECTION("A successful preflight reports cancellation")
  {
    filter.configure(&shouldCancel, false, &executionCount);
    IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);

    REQUIRE(shouldCancel.load());
    REQUIRE(executionCount == 0);
    REQUIRE(executeResult.result.invalid());
    REQUIRE(executeResult.result.errors().size() == 1);
    REQUIRE(executeResult.result.errors().front().code == -1);
    REQUIRE(executeResult.result.warnings().size() == 2);
    REQUIRE(executeResult.result.warnings().front().code == k_PreflightWarningCode);
    REQUIRE(executeResult.result.warnings()[1].code == -1);
    REQUIRE(executeResult.outputValues.size() == 1);
    REQUIRE(executeResult.outputValues.front().name == k_PreflightValueName.view());
    REQUIRE(executeResult.outputValues.front().value == k_PreflightValueText.view());
  }
}

TEST_CASE("IFilter: A deferred-action error precedes post-execution validation", "[simplnx][IFilter]")
{
  DataStructure dataStructure;
  auto* imageGeometry = ImageGeom::Create(dataStructure, "Invalid Image Geometry");
  REQUIRE(imageGeometry != nullptr);
  imageGeometry->setDimensions({2, 2, 2});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", {1}, imageGeometry->getId());
  REQUIRE(cellData != nullptr);
  imageGeometry->setCellData(*cellData);

  Result<> geometryValidationResult = dataStructure.validateGeometries();
  REQUIRE(geometryValidationResult.invalid());
  REQUIRE(geometryValidationResult.errors().front().code == -4501);

  auto failingInvocationCount = std::make_shared<int32>(0);
  auto followingInvocationCount = std::make_shared<int32>(0);
  DeferredFailureFilter filter(failingInvocationCount, followingInvocationCount);
  IFilter::ExecuteResult executeResult = filter.execute(dataStructure, {});

  REQUIRE(*failingInvocationCount == 1);
  REQUIRE(*followingInvocationCount == 0);
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors().front().code == k_DeferredErrorCode);
  REQUIRE(executeResult.result.warnings().size() == 2);
  REQUIRE(executeResult.result.warnings()[0].code == k_PreflightWarningCode);
  REQUIRE(executeResult.result.warnings()[1].code == k_DeferredWarningCode);
  REQUIRE(executeResult.outputValues.size() == 1);
  REQUIRE(executeResult.outputValues.front().name == k_PreflightValueName.view());
  REQUIRE(executeResult.outputValues.front().value == k_PreflightValueText.view());
}
