#include "MaskCompareUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

using namespace nx::core;

//-----------------------------------------------------------------------------
std::unique_ptr<MaskCompareUtilities::MaskCompare> MaskCompareUtilities::InstantiateMaskCompare(DataStructure& dataStructure, const DataPath& maskArrayPath)
{
  auto& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);

  return MaskCompareUtilities::InstantiateMaskCompare(maskArray);
}

//-----------------------------------------------------------------------------
std::unique_ptr<MaskCompareUtilities::MaskCompare> MaskCompareUtilities::InstantiateMaskCompare(IDataArray& maskArray)
{
  switch(maskArray.getDataType())
  {
  case DataType::boolean: {
    return std::make_unique<MaskCompareUtilities::BoolMaskCompare>(dynamic_cast<BoolArray&>(maskArray).getDataStoreRef());
  }
  case DataType::uint8: {
    return std::make_unique<MaskCompareUtilities::UInt8MaskCompare>(dynamic_cast<UInt8Array&>(maskArray).getDataStoreRef());
  }
  default:
    throw std::runtime_error("InstantiateMaskCompare: The Mask Array being used is NOT of type bool or uint8.");
  }
}