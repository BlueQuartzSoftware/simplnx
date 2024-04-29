#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ErodeDilateCoordinationNumberInputValues
{
  int32 CoordinationNumber;
  bool Loop;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
  DataPath InputImageGeometry;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ErodeDilateCoordinationNumber
{
public:
  ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateCoordinationNumberInputValues* inputValues);
  ~ErodeDilateCoordinationNumber() noexcept;

  ErodeDilateCoordinationNumber(const ErodeDilateCoordinationNumber&) = delete;
  ErodeDilateCoordinationNumber(ErodeDilateCoordinationNumber&&) noexcept = delete;
  ErodeDilateCoordinationNumber& operator=(const ErodeDilateCoordinationNumber&) = delete;
  ErodeDilateCoordinationNumber& operator=(ErodeDilateCoordinationNumber&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ErodeDilateCoordinationNumberInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
