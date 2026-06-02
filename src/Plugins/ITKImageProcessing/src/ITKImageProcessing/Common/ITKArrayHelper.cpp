#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "simplnx/Common/TypesUtility.hpp"

#include <fmt/ranges.h>

using namespace nx::core;

DataType ITK::detail::ConvertChoiceToDataType(types::usize choice)
{
  switch(choice)
  {
  case 0:
    return DataType::uint8;
  case 1:
    return DataType::uint16;
  case 2:
    return DataType::uint32;
  }
  return DataType::uint8;
}

Result<> ITK::CheckImageType(const std::vector<DataType>& types, const DataStructure& dataStructure, const DataPath& path)
{
  const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);

  DataType dataType = dataArray.getDataType();
  auto iter = std::find(types.cbegin(), types.cend(), dataType);

  std::vector<std::string> names;
  for(auto type : types)
  {
    names.push_back(DataTypeToString(type));
  }

  if(iter == types.cend())
  {
    return MakeErrorResult(-1, fmt::format("Wrong data type in {}. Expected {}, but got {}. Try CastImageFilter or RescaleImageFilter to convert input data to a supported type.", path.toString(),
                                           names, DataTypeToString(dataType)));
  }

  return {};
}
