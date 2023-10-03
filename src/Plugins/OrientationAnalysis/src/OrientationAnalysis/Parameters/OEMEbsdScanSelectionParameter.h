#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/Utilities/FilePathGenerator.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>
#include <list>
#include <string>
#include <unordered_set>

namespace fs = std::filesystem;

namespace complex
{
/** @brief RefFrameZDir defined for the Stacking order of images into a 3D Volume. This is taken from
 * EbsdLib. If EbsdLib changes, this should follow suit.
 */
namespace RefFrameZDir
{
inline constexpr uint32_t k_LowtoHigh = 0;
inline constexpr uint32_t k_HightoLow = 1;
inline constexpr uint32_t UnknownRefFrameZDirection = 2;
} // namespace RefFrameZDir

class COMPLEX_EXPORT OEMEbsdScanSelectionParameter : public ValueParameter
{

public:
  /**
   * @brief This struct holds all of the data necessary to generate a list of file paths.
   */
  struct ValueType
  {
    fs::path inputFilePath;
    uint32_t stackingOrder = RefFrameZDir::k_LowtoHigh;
    std::list<std::string> scanNames = {};
  };

  using ExtensionsType = std::unordered_set<std::string>;
  enum EbsdReaderType : uint8
  {
    Oim = 0,
    Esprit = 1,
    H5Oina = 2
  };

  OEMEbsdScanSelectionParameter() = delete;

  /**
   * @brief Constructor
   * @param name internal string key for the parameter
   * @param humanName Human facing string for the parameter
   * @param helpText The help text that should be displayed to a user
   * @param defaultValue The default value for the parameter
   */
  OEMEbsdScanSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                /* const AllowedManufacturers& allowedManufacturers, */
                                const EbsdReaderType& readerType, const ExtensionsType& extensionsType);

  ~OEMEbsdScanSelectionParameter() override = default;

  OEMEbsdScanSelectionParameter(const OEMEbsdScanSelectionParameter&) = delete;
  OEMEbsdScanSelectionParameter(OEMEbsdScanSelectionParameter&&) noexcept = delete;
  OEMEbsdScanSelectionParameter& operator=(const OEMEbsdScanSelectionParameter&) = delete;
  OEMEbsdScanSelectionParameter& operator=(OEMEbsdScanSelectionParameter&&) noexcept = delete;

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

  /**
   * @brief Returns all of the valid extension types that can be used.
   * @return
   */
  ExtensionsType getAvailableExtensions() const;

  /**
   * @brief Returns the type of ebsd reader to be used to read in the data.
   * @return
   */
  EbsdReaderType getReaderType() const;

private:
  ValueType m_DefaultValue = {};
  ExtensionsType m_AvailableExtensions = {};
  EbsdReaderType m_ReaderType = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::OEMEbsdScanSelectionParameter, "3935c833-aa51-4a58-81e9-3a51972c05ea");
