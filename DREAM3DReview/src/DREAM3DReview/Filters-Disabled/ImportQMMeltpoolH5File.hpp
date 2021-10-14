#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ImportQMMeltpoolH5File
 * @brief This filter will ....
 */
class COMPLEX_EXPORT ImportQMMeltpoolH5File : public IFilter
{
public:
  ImportQMMeltpoolH5File() = default;
  ~ImportQMMeltpoolH5File() noexcept override = default;

  ImportQMMeltpoolH5File(const ImportQMMeltpoolH5File&) = delete;
  ImportQMMeltpoolH5File(ImportQMMeltpoolH5File&&) noexcept = delete;

  ImportQMMeltpoolH5File& operator=(const ImportQMMeltpoolH5File&) = delete;
  ImportQMMeltpoolH5File& operator=(ImportQMMeltpoolH5File&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFiles_Key = "InputFiles";
  static inline constexpr StringLiteral k_PossibleIndices_Key = "PossibleIndices";
  static inline constexpr StringLiteral k_SliceRange_Key = "SliceRange";
  static inline constexpr StringLiteral k_DataContainerPath_Key = "DataContainerPath";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "VertexAttributeMatrixName";
  static inline constexpr StringLiteral k_Power_Key = "Power";

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
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ImportQMMeltpoolH5File, "23fd021e-b649-587a-89d3-a93f76e207d6");
