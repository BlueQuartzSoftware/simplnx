#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include <EbsdLib/Orientation/OrientationFwd.hpp>

#include <chrono>
#include <concepts>
#include <mutex>

namespace nx::core
{

namespace convert_orientations_constants
{
// Error Code constants
constexpr int32 k_InputComponentDimensionError = -67003;
constexpr int32 k_InputComponentCountError = -67004;
constexpr int32 k_MatchingTypesError = -67005;
} // namespace convert_orientations_constants

struct ORIENTATIONANALYSIS_EXPORT ConvertOrientationsInputValues
{
  ArraySelectionParameter::ValueType InputOrientationArrayPath;
  ebsdlib::orientations::Type InputType;
  DataObjectNameParameter::ValueType OutputOrientationArrayName;
  ebsdlib::orientations::Type OutputType;
};

/**
 * @class ConvertOrientations
 * @brief This algorithm implements support code for the ConvertOrientationsFilter
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

  /**
   * @brief Returns true if the user has requested the filter be cancelled. Safe to call from the
   * parallel convertor workers.
   */
  bool shouldCancel() const;

  /**
   * @brief Mutex-protected, time-throttled progress reporter. The parallel convertor workers call
   * this once per processed chunk; messages are emitted at most ~once per second.
   * @param counter Number of tuples completed since the last call.
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const ConvertOrientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
