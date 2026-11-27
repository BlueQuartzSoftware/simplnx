#include "MaskCompareUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

using namespace nx::core;

//-----------------------------------------------------------------------------
std::unique_ptr<mask_compare_utilities::MaskCompare> mask_compare_utilities::InstantiateMaskCompare(DataStructure& dataStructure, const DataPath& maskArrayPath)
{
  auto& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);

  return mask_compare_utilities::InstantiateMaskCompare(maskArray);
}

//-----------------------------------------------------------------------------
std::unique_ptr<mask_compare_utilities::MaskCompare> mask_compare_utilities::InstantiateMaskCompare(IDataArray& maskArray)
{
  switch(maskArray.getDataType())
  {
  case DataType::boolean: {
    return std::make_unique<mask_compare_utilities::BoolMaskCompare>(dynamic_cast<BoolArrayType&>(maskArray).getDataStoreRef());
  }
  case DataType::uint8: {
    return std::make_unique<mask_compare_utilities::UInt8MaskCompare>(dynamic_cast<UInt8ArrayType&>(maskArray).getDataStoreRef());
  }
  default:
    throw std::runtime_error("InstantiateMaskCompare: The Mask Array being used is NOT of type bool or uint8.");
  }
}