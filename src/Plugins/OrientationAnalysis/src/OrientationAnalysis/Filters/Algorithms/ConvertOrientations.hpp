#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include <EbsdLib/Orientation/OrientationFwd.hpp>

#include <concepts>

namespace nx::core
{
// Error Code constants
constexpr nx::core::int32 k_InputRepresentationTypeError = -67001;
constexpr nx::core::int32 k_OutputRepresentationTypeError = -67002;
constexpr nx::core::int32 k_InputComponentDimensionError = -67003;
constexpr nx::core::int32 k_InputComponentCountError = -67004;
constexpr nx::core::int32 k_MatchingTypesError = -67005;

struct ORIENTATIONANALYSIS_EXPORT ConvertOrientationsInputValues
{
  ArraySelectionParameter::ValueType InputOrientationArrayPath;
  ebsdlib::orientations::Type InputType;
  DataObjectNameParameter::ValueType OutputOrientationArrayName;
  ebsdlib::orientations::Type OutputType;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ConvertOrientations
{
public:
  ConvertOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertOrientationsInputValues* inputValues);
  ~ConvertOrientations() noexcept;

  ConvertOrientations(const ConvertOrientations&) = delete;
  ConvertOrientations(ConvertOrientations&&) noexcept = delete;
  ConvertOrientations& operator=(const ConvertOrientations&) = delete;
  ConvertOrientations& operator=(ConvertOrientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertOrientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
