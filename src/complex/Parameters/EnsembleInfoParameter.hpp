#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/Utilities/FilePathGenerator.hpp"
#include "complex/complex_export.hpp"

#include <string>

namespace complex
{
class COMPLEX_EXPORT EnsembleInfoParameter : public ValueParameter
{

public:
  /**
   * @brief This struct holds all of the data necessary to generate a list of file paths.
   */
  using RowType = std::array<std::string, 3>;
  using ValueType = std::vector<RowType>;

  // The lists here are IN ORDER of the enumerations from the EBSDLib. DO NOT CHANGE THE ORDER.
  static inline const std::vector<std::string> s_CrystalStructures = {"Hexagonal-High 6/mmm",  "Cubic-High m-3m", "Hexagonal-Low 6/m", "Cubic-Low m-3 (Tetrahedral)",
                                                                      "Triclinic -1",          "Monoclinic 2/m",  "Orthorhombic mmm",  "Tetragonal-Low 4/m",
                                                                      "Tetragonal-High 4/mmm", "Trigonal-Low -3", "Trigonal-High -3m"};
  static inline const std::vector<std::string> s_PhaseTypes = {"Primary", "Precipitate", "Transformation", "Matrix", "Boundary", "Unknown Phase Type"};

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
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

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
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::EnsembleInfoParameter, "10d3924f-b4c9-4e06-9225-ce11ec8dff89");
