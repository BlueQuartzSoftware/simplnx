#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Parameters.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT CreateAttributeMatrixFilter : public IFilter
{
public:
  CreateAttributeMatrixFilter() = default;
  ~CreateAttributeMatrixFilter() noexcept override = default;

  CreateAttributeMatrixFilter(const CreateAttributeMatrixFilter&) = delete;
  CreateAttributeMatrixFilter(CreateAttributeMatrixFilter&&) noexcept = delete;

  CreateAttributeMatrixFilter& operator=(const CreateAttributeMatrixFilter&) = delete;
  CreateAttributeMatrixFilter& operator=(CreateAttributeMatrixFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataObjectPath = "data_object_path";
  static inline constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";

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
   * @brief Classes that implement IFilter must provide this function for preflight.
   * Runs after the filter runs the checks in its parameters.
   * @param data
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Classes that implement IFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CreateAttributeMatrixFilter, "a6a28355-ee69-4874-bcac-76ed427423ed");
