#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include <atomic>

namespace nx::core
{

/**
 * @struct ConvertQuaternionInputValues
 * @brief Contains the paths and conversion options consumed by ConvertQuaternion.
 */
struct ORIENTATIONANALYSIS_EXPORT ConvertQuaternionInputValues
{
  DataPath QuaternionDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
  ChoicesParameter::ValueType ConversionType;
};

/**
 * @class ConvertQuaternion
 * @brief Dispatches quaternion component-order conversion.
 *
 * Contiguous arrays use raw pointers. OOC arrays use 65,536-tuple bulk
 * buffers, keeping local memory independent of tuple count.
 */
class ORIENTATIONANALYSIS_EXPORT ConvertQuaternion
{
public:
  /**
   * @brief Initializes quaternion component-order conversion.
   * @param dataStructure Provides selected arrays.
   * @param messageHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and component order.
   * @pre dataStructure, messageHandler, shouldCancel, and inputValues outlive
   *      this executor.
   */
  ConvertQuaternion(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ConvertQuaternionInputValues* inputValues);

  /**
   * @brief Destroys the quaternion conversion executor.
   */
  ~ConvertQuaternion() noexcept;

  ConvertQuaternion(const ConvertQuaternion&) = delete;
  ConvertQuaternion(ConvertQuaternion&&) noexcept = delete;
  ConvertQuaternion& operator=(const ConvertQuaternion&) = delete;
  ConvertQuaternion& operator=(ConvertQuaternion&&) noexcept = delete;

  /**
   * @brief Converts every quaternion to the selected component order.
   * @return Success, or a type or bulk-I/O error.
   *
   * Cancellation returns success with completed blocks preserved.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertQuaternionInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
