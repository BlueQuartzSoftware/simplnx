#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
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
constexpr int32 k_InputRepresentationTypeError = -67001;
constexpr int32 k_OutputRepresentationTypeError = -67002;
constexpr int32 k_InputComponentDimensionError = -67003;
constexpr int32 k_InputComponentCountError = -67004;
constexpr int32 k_MatchingTypesError = -67005;
} // namespace convert_orientations_constants

/**
 * @struct ConvertOrientationsInputValues
 * @brief Identifies orientation-conversion inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ConvertOrientationsInputValues
{
  ArraySelectionParameter::ValueType InputOrientationArrayPath;
  ebsdlib::orientations::Type InputType;
  DataObjectNameParameter::ValueType OutputOrientationArrayName;
  ebsdlib::orientations::Type OutputType;
};

/**
 * @class ConvertOrientations
 * @brief Converts between EbsdLib orientation representations.
 *
 * Macro-generated workers convert 4,096-tuple local buffers. Bulk I/O avoids
 * per-element OOC access.
 */
class ORIENTATIONANALYSIS_EXPORT ConvertOrientations
{
public:
  /**
   * @brief Initializes orientation conversion.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies input and output representations.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ConvertOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertOrientationsInputValues* inputValues);
  /**
   * @brief Destroys the orientation-conversion executor.
   */
  ~ConvertOrientations() noexcept;

  ConvertOrientations(const ConvertOrientations&) = delete;
  ConvertOrientations(ConvertOrientations&&) noexcept = delete;
  ConvertOrientations& operator=(const ConvertOrientations&) = delete;
  ConvertOrientations& operator=(ConvertOrientations&&) noexcept = delete;

  /**
   * @brief Converts orientations.
   * @return Success.
   *
   * Cancellation returns success with completed chunks preserved.
   */
  Result<> operator()();

  /**
   * @brief Returns the current cancellation state.
   * @return True if cancellation has been requested.
   */
  bool shouldCancel() const;

  /**
   * @brief Sends a throttled progress message.
   * @param counter Specifies completed tuples.
   *
   * The mutex serializes progress state updates from workers.
   */
  void sendThreadSafeProgressMessage(usize counter);

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel() const
  {
    return m_ShouldCancel;
  }

private:
  DataStructure& m_DataStructure;
  const ConvertOrientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();
  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_TotalPoints = 0;
  usize m_ProgressCounter = 0;
};

} // namespace nx::core
