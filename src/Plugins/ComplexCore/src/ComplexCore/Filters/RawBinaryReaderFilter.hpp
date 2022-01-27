#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class RawBinaryReaderFilter
 * @brief This filter reads data stored in files on the user's system
 * in binary form. The user should know exactly how the data is stored
 * in the file and properly define this in the user interface. Not
 * correctly identifying the type of data can cause serious issues since
 * this filter is simply reading the data into a pre-allocated array
 * interpreted as the user defines.
 */
class COMPLEXCORE_EXPORT RawBinaryReaderFilter : public IFilter
{
public:
  RawBinaryReaderFilter() = default;
  ~RawBinaryReaderFilter() noexcept override = default;

  RawBinaryReaderFilter(const RawBinaryReaderFilter&) = delete;
  RawBinaryReaderFilter(RawBinaryReaderFilter&&) noexcept = delete;

  RawBinaryReaderFilter& operator=(const RawBinaryReaderFilter&) = delete;
  RawBinaryReaderFilter& operator=(RawBinaryReaderFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "InputFile";
  static inline constexpr StringLiteral k_ScalarType_Key = "ScalarType";
  static inline constexpr StringLiteral k_TupleDims_Key = "TupleDimensions";
  static inline constexpr StringLiteral k_NumberOfComponents_Key = "NumberOfComponents";
  static inline constexpr StringLiteral k_Endian_Key = "Endian";
  static inline constexpr StringLiteral k_SkipHeaderBytes_Key = "SkipHeaderBytes";
  static inline constexpr StringLiteral k_CreatedAttributeArrayPath_Key = "CreatedAttributeArrayPath";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, RawBinaryReaderFilter, "0791f556-3d73-5b1e-b275-db3f7bb6850d");
