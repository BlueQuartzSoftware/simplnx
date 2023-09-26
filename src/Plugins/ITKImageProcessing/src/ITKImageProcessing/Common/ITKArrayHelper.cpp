#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Common/TypesUtility.hpp"

#include <fmt/ranges.h>

using namespace complex;

bool ITK::DoTuplesMatch(const IDataStore& dataStore, const ImageGeom& imageGeom)
{
  return imageGeom.getNumberOfCells() == dataStore.getNumberOfTuples();
}

bool ITK::DoDimensionsMatch(const IDataStore& dataStore, const ImageGeom& imageGeom)
{
  // Stored fastest to slowest i.e. X Y Z
  SizeVec3 geomDims = imageGeom.getDimensions();

  // Stored slowest to fastest i.e. Z Y X
  auto imageArrayDims = dataStore.getTupleShape();

  SizeVec3 orderedArrayDims;

  if(imageArrayDims.size() == 3)
  {
    orderedArrayDims = SizeVec3(imageArrayDims[2], imageArrayDims[1], imageArrayDims[0]);
  }
  else if(imageArrayDims.size() == 2 && geomDims.getZ() == 1)
  {
    orderedArrayDims = SizeVec3(imageArrayDims[1], imageArrayDims[0], 1);
  }
  else
  {
    return false;
  }

  return orderedArrayDims == geomDims;
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
