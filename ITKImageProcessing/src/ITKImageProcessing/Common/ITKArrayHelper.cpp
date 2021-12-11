#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

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
