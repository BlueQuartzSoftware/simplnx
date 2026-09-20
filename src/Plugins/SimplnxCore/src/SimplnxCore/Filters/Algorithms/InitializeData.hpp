#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

namespace nx::core
{
/**
 * @brief Separates component values in initialization parameters.
 */
constexpr char k_DelimiterChar = ';';

/**
 * @enum InitializeType
 * @brief Selects complete-array initialization behavior.
 */
enum InitializeType : uint64
{
  FillValue,   ///< Repeats configured component values.
  Incremental, ///< Adds or subtracts component steps after each tuple.
  Random,      ///< Uses the implementation's full-type distribution.
  RangedRandom ///< Uses configured component ranges.
};

/**
 * @enum StepType
 * @brief Selects the incremental operation.
 */
enum StepType : uint64
{
  Addition,   ///< Adds each step after generating a tuple.
  Subtraction ///< Subtracts each step after generating a tuple.
};

/**
 * @brief Formats integer component values for a preflight preview.
 * @param componentValues Supplies one or all component values.
 * @param numComps Number of represented components.
 * @return Comma-delimited preview. More than 10 components show first and last values.
 */
std::string CreateCompValsStr(const std::vector<int64>& componentValues, usize numComps);

/**
 * @brief Converts string component values and formats a preflight preview.
 * @param componentValuesStrs Supplies Boolean text or integer text.
 * @param numComps Number of represented components.
 * @return Comma-delimited preview.
 * @pre Every non-Boolean string converts through std::stoll().
 */
std::string CreateCompValsStr(const std::vector<std::string>& componentValuesStrs, usize numComps);

/**
 * @brief Appends a fill-value preflight preview for multi-component tuples.
 * @param initFillValueStr Semicolon-delimited fill values.
 * @param numComps Number of components per tuple.
 * @param preflightUpdatedValues Receives the preview.
 */
void CreateFillPreflightVals(const std::string& initFillValueStr, usize numComps, std::vector<IFilter::PreflightValue>& preflightUpdatedValues);

/**
 * @brief Appends an incremental preview and zero-step warning.
 * @param initFillValueStr Semicolon-delimited initial values.
 * @param stepOperation Addition or subtraction choice index.
 * @param stepValueStr Semicolon-delimited step values.
 * @param numTuples Number of output tuples.
 * @param numComps Number of components per tuple.
 * @param preflightUpdatedValues Receives the preview.
 */
void CreateIncrementalPreflightVals(const std::string& initFillValueStr, usize stepOperation, const std::string& stepValueStr, usize numTuples, usize numComps,
                                    std::vector<IFilter::PreflightValue>& preflightUpdatedValues);

/**
 * @brief Appends a random-generation preflight preview.
 * @param standardizeSeed True to seed all component engines identically.
 * @param initType Random or ranged-random mode.
 * @param initStartRange Semicolon-delimited lower bounds.
 * @param initEndRange Semicolon-delimited upper bounds.
 * @param numTuples Number of output tuples.
 * @param numComps Number of components per tuple.
 * @param preflightUpdatedValues Receives the preview.
 */
void CreateRandomPreflightVals(bool standardizeSeed, InitializeType initType, const std::string& initStartRange, const std::string& initEndRange, usize numTuples, usize numComps,
                               std::vector<IFilter::PreflightValue>& preflightUpdatedValues);

/**
 * @struct InitializeDataInputValues
 * @brief Stores target, mode, component values, ranges, steps, and seed behavior.
 */
struct SIMPLNXCORE_EXPORT InitializeDataInputValues
{
  DataPath InputArrayPath;
  InitializeType initType;
  StepType stepType;
  std::vector<std::string> stringValues;
  std::vector<std::string> startValues;
  std::vector<std::string> stepValues;
  uint64 seed;
  std::vector<std::string> randBegin;
  std::vector<std::string> randEnd;
  bool standardizeSeed;
};

/**
 * @struct ValidateMultiInputFunctor
 * @brief Validates delimited component counts and value conversion.
 */
struct SIMPLNXCORE_EXPORT ValidateMultiInputFunctor
{
  /**
   * @brief Validates one semicolon-delimited component string.
   * @tparam T Specifies the required conversion type.
   * @param expectedComp Required component count.
   * @param unfilteredStr Input component text.
   * @param singleCompSize Accepted alternate count, or zero to disable it.
   * @return Success, or a preflight error for empty, invalid, trailing, or mismatched values.
   */
  template <typename T>
  IFilter::PreflightResult operator()(const usize expectedComp, const std::string& unfilteredStr, const usize singleCompSize = 0)
  {
    std::vector<std::string> splitVals = StringUtilities::split(StringUtilities::trimmed(unfilteredStr), k_DelimiterChar);

    if(splitVals.empty())
    {
      return IFilter::MakePreflightErrorResult(-11610, fmt::format("A required parameter is unable to be processed with '{}' delimiter. Input: {}", k_DelimiterChar, unfilteredStr));
    }

    for(usize comp = 0; comp < splitVals.size(); comp++)
    {
      if(splitVals[comp].empty())
      {
        return IFilter::MakePreflightErrorResult(-11611, fmt::format("Empty value found after '{}' components were converted. Check for duplicate '{}' next to one another.", comp, k_DelimiterChar));
      }

      Result<T> result = StringInterpretationUtilities::Convert<T>(splitVals[comp]);
      if(result.invalid())
      {
        return IFilter::MakePreflightErrorResult(-11612, fmt::format("Unable to process '{}' into a {} value.", splitVals[comp], DataTypeToString(GetDataType<T>())));
      }
    }

    if(splitVals.size() == expectedComp)
    {
      return {}; // Valid
    }

    if(splitVals.size() == singleCompSize)
    {
      return {}; // Valid
    }

    if(splitVals.size() == expectedComp + 1)
    {
      if(unfilteredStr.back() == k_DelimiterChar)
      {
        return IFilter::MakePreflightErrorResult(-11613, fmt::format("Remove the extra delimiter '{}' at the end of your value sequence: {}.", k_DelimiterChar, unfilteredStr));
      }
    }

    return IFilter::MakePreflightErrorResult(-11614,
                                             fmt::format("Using '{}' as a delimiter we are unable to break '{}' into the required {} components.", k_DelimiterChar, unfilteredStr, expectedComp));
  } // namespace nx::core
};

/**
 * @class InitializeData
 * @brief Initializes a complete DataArray through generated tuple chunks.
 *
 * Fill mode repeats component values. Incremental mode generates one sequence
 * per component. Random modes use one mt19937_64 engine per component. Equal
 * seeds produce equal draws only when the component distributions are equal.
 *
 * The transfer targets 65,536 values but always retains one complete tuple. A
 * wider tuple creates a larger one-tuple buffer. All modes write completed chunks
 * immediately. Cancellation returns success and does not restore those chunks.
 * In-core and out-of-core telemetry labels use the same generator implementation.
 *
 * Preflight must validate all conversion strings. Runtime conversion Results are
 * dereferenced without an error check. Signed incremental arithmetic must remain
 * representable. Integral random distributions use int64 bounds and cannot
 * represent the full UInt64 range. Unranged floating generation uses global
 * rand() state for signs, so its output is not controlled only by seed.
 */
class InitializeData
{
public:
  /**
   * @brief Initializes the complete-array generator.
   * @param dataStructure Contains the target array.
   * @param mesgHandler Preserves the common algorithm constructor signature.
   * @param shouldCancel Signals cancellation between generated chunks.
   * @param inputValues Selects mode, values, ranges, steps, and seed behavior.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues);
  /**
   * @brief Destroys the complete-array generator.
   */
  ~InitializeData() noexcept;

  InitializeData(const InitializeData&) = delete;
  InitializeData(InitializeData&&) noexcept = delete;
  InitializeData& operator=(const InitializeData&) = delete;
  InitializeData& operator=(InitializeData&&) noexcept = delete;

  /**
   * @brief Generates and writes all target tuples.
   * @return Bulk-write or invalid-mode result.
   * @pre Component input lists contain one value or one value per component.
   * @pre All configured strings convert to the target value type.
   *
   * Cancellation returns success with completed chunks retained.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InitializeDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
