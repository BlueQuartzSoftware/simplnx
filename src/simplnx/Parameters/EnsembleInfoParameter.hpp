#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/FilePathGenerator.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT EnsembleInfoParameter : public ValueParameter
{

public:
  /**
   * @brief This struct holds all of the data necessary to generate a list of file paths.
   */
  using RowType = std::array<std::string, 3>;
  using ValueType = std::vector<RowType>;

  // The lists here are IN ORDER of the enumerations from the EBSDLib. DO NOT CHANGE THE ORDER.
  static inline const std::vector<std::string> k_CrystalStructures = {"Hexagonal-High 6/mmm",  "Cubic-High m-3m", "Hexagonal-Low 6/m", "Cubic-Low m-3 (Tetrahedral)",
                                                                      "Triclinic -1",          "Monoclinic 2/m",  "Orthorhombic mmm",  "Tetragonal-Low 4/m",
                                                                      "Tetragonal-High 4/mmm", "Trigonal-Low -3", "Trigonal-High -3m"};
  static inline const std::vector<std::string> k_PhaseTypes = {"Primary", "Precipitate", "Transformation", "Matrix", "Boundary"};

  static inline constexpr StringLiteral k_DefaultPhaseName = "_PHASE_NAME_";

  EnsembleInfoParameter() = delete;

  EnsembleInfoParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);

  ~EnsembleInfoParameter() override = default;

  EnsembleInfoParameter(const EnsembleInfoParameter&) = delete;
  EnsembleInfoParameter(EnsembleInfoParameter&&) noexcept = delete;

  EnsembleInfoParameter& operator=(const EnsembleInfoParameter&) = delete;
  EnsembleInfoParameter& operator=(EnsembleInfoParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief Returns version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

protected:
  /**
   * @brief
   * @param value
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT EnsembleInfoFilterParameterConverter
{
  using ParameterType = EnsembleInfoParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::EnsembleInfoParameter, "10d3924f-b4c9-4e06-9225-ce11ec8dff89");
