#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT ConcatenateDataArraysFilter : public IFilter
{
public:
  ConcatenateDataArraysFilter() = default;
  ~ConcatenateDataArraysFilter() noexcept override = default;

  ConcatenateDataArraysFilter(const ConcatenateDataArraysFilter&) = delete;
  ConcatenateDataArraysFilter(ConcatenateDataArraysFilter&&) noexcept = delete;

  ConcatenateDataArraysFilter& operator=(const ConcatenateDataArraysFilter&) = delete;
  ConcatenateDataArraysFilter& operator=(ConcatenateDataArraysFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputArrays_Key = "input_arrays";
  static inline constexpr StringLiteral k_OutputArray_Key = "output_array";
  static inline constexpr StringLiteral k_OutputTupleDims_Key = "output_tuple_dims";

  // Error Codes
  enum class ErrorCodes : int32
  {
    EmptyInputArrays = -2300,
    OneInputArray = -2301,
    NonPositiveTupleDimValue = -2302,
    TypeNameMismatch = -2303,
    ComponentShapeMismatch = -2304,
    TotalTuplesMismatch = -2305,
    InputArraysEqualAny = -2306,
    InputArraysUnsupported = -2307
  };

  // Warning Codes
  enum class WarningCodes : int32
  {
    MultipleTupleDimsNotSupported = -100
  };

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter's name.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter name as a human-readable string.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of parameters required to execute the filter.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates and returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ConcatenateDataArraysFilter, "e547e382-4928-4ec6-a311-d26c16e9f675");
