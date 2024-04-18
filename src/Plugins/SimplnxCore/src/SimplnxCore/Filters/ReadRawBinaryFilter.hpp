#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReadRawBinaryFilter
 * @brief This filter reads data stored in files on the user's system
 * in binary form. The user should know exactly how the data is stored
 * in the file and properly define this in the user interface. Not
 * correctly identifying the type of data can cause serious issues since
 * this filter is simply reading the data into a pre-allocated array
 * interpreted as the user defines.
 */
class SIMPLNXCORE_EXPORT ReadRawBinaryFilter : public IFilter
{
public:
  ReadRawBinaryFilter() = default;
  ~ReadRawBinaryFilter() noexcept override = default;

  ReadRawBinaryFilter(const ReadRawBinaryFilter&) = delete;
  ReadRawBinaryFilter(ReadRawBinaryFilter&&) noexcept = delete;

  ReadRawBinaryFilter& operator=(const ReadRawBinaryFilter&) = delete;
  ReadRawBinaryFilter& operator=(ReadRawBinaryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "input_file";
  static inline constexpr StringLiteral k_ScalarType_Key = "scalar_type_index";
  static inline constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";
  static inline constexpr StringLiteral k_NumberOfComponents_Key = "number_of_components";
  static inline constexpr StringLiteral k_Endian_Key = "endian_index";
  static inline constexpr StringLiteral k_SkipHeaderBytes_Key = "skip_header_bytes";
  static inline constexpr StringLiteral k_CreatedAttributeArrayPath_Key = "created_attribute_array_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadRawBinaryFilter, "dd159366-5f12-42db-af6d-a33592ae8a89");
