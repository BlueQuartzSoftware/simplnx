#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct ConvertColorToGrayScaleInputValues
 * @brief Stores selected color-array paths and grayscale options.
 */
struct SIMPLNXCORE_EXPORT ConvertColorToGrayScaleInputValues
{
  ChoicesParameter::ValueType ConversionAlgorithm;
  VectorFloat32Parameter::ValueType ColorWeights;
  int32 ColorChannel;
  MultiArraySelectionParameter::ValueType InputDataArrayPaths;
  std::vector<DataPath> OutputDataArrayPaths;
  StringParameter::ValueType OutputArrayPrefix;
};

/**
 * @class ConvertColorToGrayScale
 * @brief Converts selected three-component uint8 RGB arrays to single-component grayscale arrays.
 *
 * Concrete in-memory stores use parallel raw pointers. Disk-backed arrays use
 * bounded chunk buffers and bulk I/O. The abstract direct fallback accesses
 * DataStore instances in parallel and has no general thread-safety guarantee.
 */
class SIMPLNXCORE_EXPORT ConvertColorToGrayScale
{
public:
  /**
   * @brief Creates a grayscale conversion dispatcher.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later input arrays when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ConvertColorToGrayScale(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertColorToGrayScaleInputValues* inputValues);
  /**
   * @brief Destroys the non-owning grayscale dispatcher.
   */
  ~ConvertColorToGrayScale() noexcept;

  ConvertColorToGrayScale(const ConvertColorToGrayScale&) = delete;
  ConvertColorToGrayScale(ConvertColorToGrayScale&&) noexcept = delete;
  ConvertColorToGrayScale& operator=(const ConvertColorToGrayScale&) = delete;
  ConvertColorToGrayScale& operator=(ConvertColorToGrayScale&&) noexcept = delete;

  /**
   * @brief Defines the conversion-mode storage type.
   */
  using EnumType = uint32_t;
  /**
   * @enum ConversionType
   * @brief Specifies grayscale conversion modes.
   */
  enum class ConversionType : EnumType
  {
    Luminosity = 0,   ///< Applies the selected RGB weights.
    Average = 1,      ///< Applies equal RGB weights.
    Lightness = 2,    ///< Averages the minimum and maximum RGB components.
    SingleChannel = 3 ///< Uses the selected RGB component.
  };

  /**
   * @brief Converts selected RGB arrays to grayscale arrays.
   * @return Error from the selected conversion, or success after cancellation.
   *
   * The Direct path does not test cancellation within an array. The Scanline
   * path checks before each chunk. The current loop reuses the first output path
   * for every selected input array.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertColorToGrayScaleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
