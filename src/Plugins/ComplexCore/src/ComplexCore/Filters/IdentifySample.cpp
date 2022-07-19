#include "IdentifySample.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

namespace complex
{
namespace
{
constexpr int64 k_MISSING_GEOM_ERR = -650;

template <typename T>
void _execute(DataStructure& data, const DataPath& imageGeomPath, const DataPath& goodVoxelsArrayPath, bool fillHoles)
{
  using ArrayType = DataArray<T>;

  auto* imageGeom = data.getDataAs<ImageGeom>(imageGeomPath);

  std::vector<usize> cDims = {1};
  auto* goodVoxelsPtr = data.getDataAs<ArrayType>(goodVoxelsArrayPath);
  auto& goodVoxels = goodVoxelsPtr->getDataStoreRef();

  int64 totalPoints = static_cast<int64>(goodVoxelsPtr->getNumberOfTuples());

  SizeVec3 udims = imageGeom->getDimensions();

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  int64 xp = dims[0];
  int64 yp = dims[1];
  int64 zp = dims[2];

  neighpoints[0] = -(xp * yp);
  neighpoints[1] = -xp;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xp;
  neighpoints[5] = (xp * yp);
  std::vector<int64> currentvlist;
  std::vector<bool> checked(totalPoints, false);
  std::vector<bool> sample(totalPoints, false);
  int64 biggestBlock = 0;
  usize count = 0;
  int32 good = 0;
  int64 neighbor = 0;
  int64 column = 0, row = 0, plane = 0;
  int64 index = 0;

  // In this loop over the data we are finding the biggest contiguous set of GoodVoxels and calling that the 'sample'  All GoodVoxels that do not touch the 'sample'
  // are flipped to be called 'bad' voxels or 'not sample'
  float threshold = 0.0f;
  for(int64 i = 0; i < totalPoints; i++)
  {
    float percentIncrement = static_cast<float>(i) / static_cast<float>(totalPoints) * 100.0f;
    if(percentIncrement > threshold)
    {
      // std::string ss = fmt::format("{}% Scanned", static_cast<int32>(percentIncrement));
      // filter->notifyStatusMessage(ss);
      threshold = threshold + 5.0f;
      if(threshold < percentIncrement)
      {
        threshold = percentIncrement;
      }
    }

    if(!checked[i] && goodVoxels.getValue(i))
    {
      currentvlist.push_back(i);
      count = 0;
      while(count < currentvlist.size())
      {
        index = currentvlist[count];
        column = index % xp;
        row = (index / xp) % yp;
        plane = index / (xp * yp);
        for(int32 j = 0; j < 6; j++)
        {
          good = 1;
          neighbor = index + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = 0;
          }
          if(j == 5 && plane == (zp - 1))
          {
            good = 0;
          }
          if(j == 1 && row == 0)
          {
            good = 0;
          }
          if(j == 4 && row == (yp - 1))
          {
            good = 0;
          }
          if(j == 2 && column == 0)
          {
            good = 0;
          }
          if(j == 3 && column == (xp - 1))
          {
            good = 0;
          }
          if(good == 1 && !checked[neighbor] && goodVoxels.getValue(neighbor))
          {
            currentvlist.push_back(neighbor);
            checked[neighbor] = true;
          }
        }
        count++;
      }
      if(static_cast<int64>(currentvlist.size()) >= biggestBlock)
      {
        biggestBlock = currentvlist.size();
        sample.assign(totalPoints, false);
        for(int64 j = 0; j < biggestBlock; j++)
        {
          sample[currentvlist[j]] = true;
        }
      }
      currentvlist.clear();
    }
  }
  for(int64 i = 0; i < totalPoints; i++)
  {
    if(!sample[i] && goodVoxels.getValue(i))
    {
      goodVoxels.setValue(i, false);
    }
  }
  sample.clear();
  checked.assign(totalPoints, false);

  // In this loop we are going to 'close' all of the 'holes' inside of the region already identified as the 'sample' if the user chose to do so.
  // This is done by flipping all 'bad' voxel features that do not touch the outside of the sample (i.e. they are fully contained inside of the 'sample'.
  threshold = 0.0F;
  if(fillHoles)
  {
    bool touchesBoundary = false;
    for(int64 i = 0; i < totalPoints; i++)
    {
      float percentIncrement = static_cast<float>(i) / static_cast<float>(totalPoints) * 100.0f;
      if(percentIncrement > threshold)
      {
        // std::string ss = fmt::format("{}% Filling Holes", static_cast<int32>(percentIncrement));
        // filter->notifyStatusMessage(ss);
        threshold = threshold + 5.0f;
        if(threshold < percentIncrement)
        {
          threshold = percentIncrement;
        }
      }

      if(!checked[i] && !goodVoxels.getValue(i))
      {
        currentvlist.push_back(i);
        count = 0;
        touchesBoundary = false;
        while(count < currentvlist.size())
        {
          index = currentvlist[count];
          column = index % xp;
          row = (index / xp) % yp;
          plane = index / (xp * yp);
          if(column == 0 || column == (xp - 1) || row == 0 || row == (yp - 1) || plane == 0 || plane == (zp - 1))
          {
            touchesBoundary = true;
          }
          for(int32 j = 0; j < 6; j++)
          {
            good = 1;
            neighbor = index + neighpoints[j];
            if(j == 0 && plane == 0)
            {
              good = 0;
            }
            if(j == 5 && plane == (zp - 1))
            {
              good = 0;
            }
            if(j == 1 && row == 0)
            {
              good = 0;
            }
            if(j == 4 && row == (yp - 1))
            {
              good = 0;
            }
            if(j == 2 && column == 0)
            {
              good = 0;
            }
            if(j == 3 && column == (xp - 1))
            {
              good = 0;
            }
            if(good == 1 && !checked[neighbor] && !goodVoxels.getValue(neighbor))
            {
              currentvlist.push_back(neighbor);
              checked[neighbor] = true;
            }
          }
          count++;
        }
        if(!touchesBoundary)
        {
          for(int64_t& j : currentvlist)
          {
            goodVoxels.setValue(j, true);
          }
        }
        currentvlist.clear();
      }
    }
  }
  checked.clear();
}

int16 getArrayType(const IDataArray* inputData)
{
  if(dynamic_cast<const DataArray<bool>*>(inputData) != nullptr)
  {
    return 1;
  }
  else if(dynamic_cast<const DataArray<uint8>*>(inputData) != nullptr)
  {
    return 2;
  }
  else
  {
    return -1;
  }
}
} // namespace

std::string IdentifySample::name() const
{
  return FilterTraits<IdentifySample>::name;
}

std::string IdentifySample::className() const
{
  return FilterTraits<IdentifySample>::className;
}

Uuid IdentifySample::uuid() const
{
  return FilterTraits<IdentifySample>::uuid;
}

std::string IdentifySample::humanName() const
{
  return "Isolate Largest Feature (Identify Sample)";
}

//------------------------------------------------------------------------------
std::vector<std::string> IdentifySample::defaultTags() const
{
  return {"#Core", "#Identify Sample"};
}

Parameters IdentifySample::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<BoolParameter>(k_FillHoles_Key, "Fill Holes in Largest Feature", "Fill Holes in Largest Feature", true));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geometry", "DataPath to the target ImageGeom", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxels_Key, "Mask", "DataPath to the target Good Voxels array", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{complex::DataType::boolean, complex::DataType::uint8}));
  return params;
}

IFilter::UniquePointer IdentifySample::clone() const
{
  return std::make_unique<IdentifySample>();
}

IFilter::PreflightResult IdentifySample::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto goodVoxelsArrayPath = args.value<DataPath>(k_GoodVoxels_Key);

  auto* imageGeom = data.getDataAs<ImageGeom>(imageGeomPath);
  if(imageGeom == nullptr)
  {
    std::string ss = fmt::format("Could not find ImageGeom at path '{}'", imageGeomPath.toString());
    return {MakeErrorResult<OutputActions>(k_MISSING_GEOM_ERR, ss)};
  }

  int8 arrayType = 0;

  auto* inputData = data.getDataAs<IDataArray>(goodVoxelsArrayPath);
  if(inputData == nullptr)
  {
    std::string ss = fmt::format("Could not find IDataArray at path '{}'", goodVoxelsArrayPath.toString());
    return {MakeErrorResult<OutputActions>(k_MISSING_GEOM_ERR, ss)};
  }

  arrayType = getArrayType(inputData);
  if(arrayType < 0)
  {
    std::string ss = fmt::format("The input data must be of type BOOL or UINT8");
    return {MakeErrorResult<OutputActions>(-12001, ss)};
  }

  OutputActions actions;
  return {std::move(actions)};
}

Result<> IdentifySample::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto fillHoles = args.value<bool>(k_FillHoles_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto goodVoxelsArrayPath = args.value<DataPath>(k_GoodVoxels_Key);

  auto* inputData = data.getDataAs<IDataArray>(goodVoxelsArrayPath);
  auto arrayType = getArrayType(inputData);

  if(arrayType == 1)
  {
    _execute<bool>(data, imageGeomPath, goodVoxelsArrayPath, fillHoles);
  }
  if(arrayType == 2)
  {
    _execute<uint8>(data, imageGeomPath, goodVoxelsArrayPath, fillHoles);
  }

  return {};
}
} // namespace complex
