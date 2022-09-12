#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{

struct CORE_EXPORT ConvertColorToGrayScaleInputValues
{
  ChoicesParameter::ValueType ConversionAlgorithm;
  VectorFloat32Parameter::ValueType ColorWeights;
  int32 ColorChannel;
  MultiArraySelectionParameter::ValueType InputDataArrayPaths;
  std::vector<DataPath> OutputDataArrayPaths;
  StringParameter::ValueType OutputArrayPrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ConvertColorToGrayScale
{
public:
  ConvertColorToGrayScale(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertColorToGrayScaleInputValues* inputValues);
  ~ConvertColorToGrayScale() noexcept;

  ConvertColorToGrayScale(const ConvertColorToGrayScale&) = delete;
  ConvertColorToGrayScale(ConvertColorToGrayScale&&) noexcept = delete;
  ConvertColorToGrayScale& operator=(const ConvertColorToGrayScale&) = delete;
  ConvertColorToGrayScale& operator=(ConvertColorToGrayScale&&) noexcept = delete;

  using EnumType = uint32_t;
  enum class ConversionType : EnumType
  {
    Luminosity = 0,
    Average = 1,
    Lightness = 2,
    SingleChannel = 3
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertColorToGrayScaleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
