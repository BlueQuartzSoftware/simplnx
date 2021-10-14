#pragma once

#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Parameters.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ExportH5DataFilter
 * @brief The ExportH5DataFilter is an IFilter class designed to export the
 * DataStructure to a target HDF5 file.
 */
class COMPLEX_EXPORT ExportH5DataFilter : public IFilter
{
public:
  ExportH5DataFilter() = default;
  ~ExportH5DataFilter() noexcept override = default;

  ExportH5DataFilter(const ExportH5DataFilter&) = delete;
  ExportH5DataFilter(ExportH5DataFilter&&) noexcept = delete;

  ExportH5DataFilter& operator=(const ExportH5DataFilter&) = delete;
  ExportH5DataFilter& operator=(ExportH5DataFilter&&) noexcept = delete;

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
   * @brief Returns the ExportH5DataFilter class's UUID.
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
  Result<OutputActions> preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief Classes that implement IFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ExportH5DataFilter, "b3a95784-2ced-11ec-8d3d-0242ac130003");
