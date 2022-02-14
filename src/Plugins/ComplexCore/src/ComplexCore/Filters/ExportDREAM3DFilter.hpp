#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Parameters.hpp"

namespace complex
{
/**
 * @class ExportDREAM3DFilter
 * @brief The ExportDREAM3DFilter is an IFilter class designed to export the
 * DataStructure to a target HDF5 file.
 */
class COMPLEXCORE_EXPORT ExportDREAM3DFilter : public IFilter
{
public:
  ExportDREAM3DFilter() = default;
  ~ExportDREAM3DFilter() noexcept override = default;

  ExportDREAM3DFilter(const ExportDREAM3DFilter&) = delete;
  ExportDREAM3DFilter(ExportDREAM3DFilter&&) noexcept = delete;

  ExportDREAM3DFilter& operator=(const ExportDREAM3DFilter&) = delete;
  ExportDREAM3DFilter& operator=(ExportDREAM3DFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ExportFilePath = "Export_File_Path";

  /**
   * @brief Returns the name of the filter class.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the ExportDREAM3DFilter class's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns a collection of the filter's parameters (i.e. its inputs)
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter as a std::unique_ptr.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Classes that implement IFilter must provide this function for preflight.
   * Runs after the filter runs the checks in its parameters.
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Classes that implement IFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ExportDREAM3DFilter, "b3a95784-2ced-11ec-8d3d-0242ac130003");
