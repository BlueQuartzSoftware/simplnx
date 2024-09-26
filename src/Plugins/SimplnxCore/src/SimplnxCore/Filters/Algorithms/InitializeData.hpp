#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

namespace nx::core
{
constexpr char k_DelimiterChar = ';';

enum InitializeType : uint64
{
  FillValue,
  Incremental,
  Random,
  RangedRandom
};

enum StepType : uint64
{
  Addition,
  Subtraction
};

/**
 * @brief Creates a formatted string representation of component values.
 *
 * This function generates a string by concatenating component values separated by commas.
 * If the number of components exceeds a visibility threshold (10 by default), it displays
 * only the first and last few values separated by ellipses. If a single component value is
 * provided, it replicates that value for the specified number of components.
 *
 * @param componentValues A vector of integer component values.
 * @param numComps The number of components to represent in the output string.
 * @return A formatted string representing the component values.
 */
std::string CreateCompValsStr(const std::vector<int64>& componentValues, usize numComps);

/**
 * @brief Creates a formatted string representation of component values from string inputs.
 *
 * This function converts a vector of string component values to integers and then generates
 * a formatted string representation using the integer version of CreateCompValsStr.
 * If a single string component value is provided, it replicates that value for the specified
 * number of components.
 *
 * @param componentValuesStrs A vector of string component values.
 * @param numComps The number of components to represent in the output string.
 * @return A formatted string representing the component values.
 */
std::string CreateCompValsStr(const std::vector<std::string>& componentValuesStrs, usize numComps);

/**
 * @brief Generates preflight values for fill operations based on initial fill values.
 *
 * This function constructs a descriptive string detailing how tuples will be filled with
 * component values. If a single initial fill value is provided, it applies the same value
 * to all components; otherwise, it uses different values for each component.
 * The generated description is appended to the provided preflightUpdatedValues vector.
 *
 * @param initFillValueStr A semicolon-separated string of initial fill values.
 * @param numComps The number of components in each tuple.
 * @param preflightUpdatedValues A reference to a vector where the generated preflight
 *        values will be stored.
 */
void CreateFillPreflightVals(const std::string& initFillValueStr, usize numComps, std::vector<IFilter::PreflightValue>& preflightUpdatedValues);

/**
 * @brief Generates preflight values for incremental fill operations on tuples.
 *
 * This function creates a descriptive summary of how tuples will be initialized and incremented
 * or decremented based on the provided step operation and step values. It includes a preview of
 * the first few tuples and issues a warning if any step value is zero, indicating that the corresponding
 * component values will remain unchanged.
 *
 * @param initFillValueStr A semicolon-separated string of initial fill values.
 * @param stepOperation The operation type for stepping (e.g., addition or subtraction).
 * @param stepValueStr A semicolon-separated string of step values for each component.
 * @param numTuples The total number of tuples to generate.
 * @param numComps The number of components in each tuple.
 * @param preflightUpdatedValues A reference to a vector where the generated preflight
 *        values will be stored.
 */
void CreateIncrementalPreflightVals(const std::string& initFillValueStr, usize stepOperation, const std::string& stepValueStr, usize numTuples, usize numComps,
                                    std::vector<IFilter::PreflightValue>& preflightUpdatedValues);

/**
 * @brief Generates preflight values for random fill operations on tuples.
 *
 * This function constructs a descriptive summary of how tuples will be filled with random
 * values, either within a specified range or without. It accounts for the number of components
 * and tuples, and whether the random seed is standardized across components. For multiple
 * components, it details whether values are generated independently or based on a single seed.
 *
 * @param standardizeSeed Indicates whether to use a standardized seed for all components.
 * @param initType The type of initialization (e.g., random or ranged random).
 * @param initStartRange A semicolon-separated string representing the starting range for
 *        random values (used if initType is ranged random).
 * @param initEndRange A semicolon-separated string representing the ending range for
 *        random values (used if initType is ranged random).
 * @param numTuples The total number of tuples to generate.
 * @param numComps The number of components in each tuple.
 * @param preflightUpdatedValues A reference to a vector where the generated preflight
 *        values will be stored.
 */
void CreateRandomPreflightVals(bool standardizeSeed, InitializeType initType, const std::string& initStartRange, const std::string& initEndRange, usize numTuples, usize numComps,
                               std::vector<IFilter::PreflightValue>& preflightUpdatedValues);

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
 * @brief A functor for validating multi-component input strings.
 *
 * The `ValidateMultiInputFunctor` struct provides a templated functor to validate
 * delimited input strings containing multiple components. It ensures that the input
 * string:
 * - Contains the expected number of components or an alternative acceptable number.
 * - Does not contain empty values between delimiters.
 * - Each component can be successfully converted to the specified type `T`.
 *
 * If the validation fails, the functor returns an error with a specific error code
 * and descriptive message. Otherwise, it indicates successful validation.
 */
struct SIMPLNXCORE_EXPORT ValidateMultiInputFunctor
{
  // The single comp size validation defaults to off as size 0 is checked earlier in the function

  /**
   * @brief Validates a delimited input string for the correct number of components and value types.
   *
   * This templated `operator()` performs the following validations on the input string:
   * 1. Splits the input string `unfilteredStr` using the predefined delimiter `k_DelimiterChar`.
   * 2. Checks if the resulting vector `splitVals` is empty. If empty, returns an error with code `-11610`.
   * 3. Iterates through each split value to ensure:
   *    - No value is empty. If an empty value is found, returns an error with code `-11611`.
   *    - Each value can be converted to type `T`. If conversion fails, returns an error with code `-11612`.
   * 4. Verifies the number of split components:
   *    - If the number of components matches `expectedComp`, validation is successful.
   *    - If `singleCompSize` is non-zero and matches the number of components, validation is successful.
   *    - If there is one extra component and the input string ends with a delimiter, returns an error with code `-11613`.
   *    - Otherwise, returns an error with code `-11614` indicating the number of components does not match expectations.
   *
   * @tparam T The type to which each component of the input string should be convertible.
   * @param expectedComp The expected number of components in the input string.
   * @param unfilteredStr The input string containing delimited components to be validated.
   * @param singleCompSize An optional alternative number of components that is also considered valid. Defaults to `0`.
   *
   * @return An `IFilter::PreflightResult` indicating the success or failure of the validation.
   *         - Returns an empty result if validation is successful.
   *         - Returns an error result with a specific error code and message if validation fails.
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

      Result<T> result = ConvertTo<T>::convert(splitVals[comp]);

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
  }
};

class InitializeData
{
public:
  InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues);
  ~InitializeData() noexcept;

  InitializeData(const InitializeData&) = delete;
  InitializeData(InitializeData&&) noexcept = delete;
  InitializeData& operator=(const InitializeData&) = delete;
  InitializeData& operator=(InitializeData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InitializeDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
