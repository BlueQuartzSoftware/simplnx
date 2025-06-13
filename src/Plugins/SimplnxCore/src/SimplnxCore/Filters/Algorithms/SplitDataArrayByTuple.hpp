#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT SplitDataArrayByTupleInputValues
{
  DataPath InputArrayPath;
  std::vector<DataPath> OutputArrayPaths;
  usize SplitDimension;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT SplitDataArrayByTuple
{
public:
  enum class OutputContainer : uint8
  {
    NewDataGroup = 0,
    ExistingDataGroup = 1,
    NewAttrMatrix = 2,
    ExistingAttrMatrix = 3
  };

  // Error Codes
  enum class ErrorCodes : int32
  {
    NoInputArray = -65400,
    SplitDimLessThanZero = -65401,
    SplitDimOutOfRange = -65402,
    SplitCountLessThanZero = -65403,
    SplitCountSumNotEqual = -65404,
    AttrMatrixTupleShapeNegative = -65405,
    AttrMatrixTupleShapeNoCommonMultiplier = -65406,
    AnyArrayType = -65407,
    UnsupportedArrayType = -65408,
    MultiDimensionalSplitCount = -65409
  };

  SplitDataArrayByTuple(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitDataArrayByTupleInputValues* inputValues);
  ~SplitDataArrayByTuple() noexcept;

  SplitDataArrayByTuple(const SplitDataArrayByTuple&) = delete;
  SplitDataArrayByTuple(SplitDataArrayByTuple&&) noexcept = delete;
  SplitDataArrayByTuple& operator=(const SplitDataArrayByTuple&) = delete;
  SplitDataArrayByTuple& operator=(SplitDataArrayByTuple&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SplitDataArrayByTupleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
