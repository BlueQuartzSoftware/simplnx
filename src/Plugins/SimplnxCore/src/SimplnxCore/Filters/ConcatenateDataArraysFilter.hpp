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
  static inline constexpr StringLiteral k_OutputArray_Key = "output_array_path";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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
