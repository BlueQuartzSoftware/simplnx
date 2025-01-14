#include "IdentifySampleFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
struct IdentifySampleFunctor
{
  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles)
  {
    std::vector<usize> cDims = {1};
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const auto totalPoints = static_cast<int64>(goodVoxelsPtr->getNumberOfTuples());

    SizeVec3 uDims = imageGeom->getDimensions();

    const int64 dims[3] = {
        static_cast<int64>(uDims[0]),
        static_cast<int64>(uDims[1]),
        static_cast<int64>(uDims[2]),
    };

    int64 neighborPoints[6] = {0, 0, 0, 0, 0, 0};
    const int64 xp = dims[0];
    const int64 yp = dims[1];
    const int64 zp = dims[2];

    neighborPoints[0] = -(xp * yp);
    neighborPoints[1] = -xp;
    neighborPoints[2] = -1;
    neighborPoints[3] = 1;
    neighborPoints[4] = xp;
    neighborPoints[5] = (xp * yp);
    std::vector<int64> currentVList;
    std::vector<bool> checked(totalPoints, false);
    std::vector<bool> sample(totalPoints, false);
    int64 biggestBlock = 0;

    int64 neighbor = 0;

    // In this loop over the data we are finding the biggest contiguous set of GoodVoxels and calling that the 'sample'  All GoodVoxels that do not touch the 'sample'
    // are flipped to be called 'bad' voxels or 'not sample'
    float threshold = 0.0f;
    for(int64 i = 0; i < totalPoints; i++)
    {
      const float percentIncrement = static_cast<float>(i) / static_cast<float>(totalPoints) * 100.0f;
      if(percentIncrement > threshold)
      {
        threshold = threshold + 5.0f;
        if(threshold < percentIncrement)
        {
          threshold = percentIncrement;
        }
      }

      if(!checked[i] && goodVoxels.getValue(i))
      {
        currentVList.push_back(i);
        usize count = 0;
        while(count < currentVList.size())
        {
          int64 index = currentVList[count];
          int64 column = index % xp;
          int64 row = (index / xp) % yp;
          int64 plane = index / (xp * yp);
          for(int32 j = 0; j < 6; j++)
          {
            int32 good = 1;
            neighbor = index + neighborPoints[j];
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
              currentVList.push_back(neighbor);
              checked[neighbor] = true;
            }
          }
          count++;
        }
        if(static_cast<int64>(currentVList.size()) >= biggestBlock)
        {
          biggestBlock = currentVList.size();
          sample.assign(totalPoints, false);
          for(int64 j = 0; j < biggestBlock; j++)
          {
            sample[currentVList[j]] = true;
          }
        }
        currentVList.clear();
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

    // In this loop we are going to 'close' all the 'holes' inside the region already identified as the 'sample' if the user chose to do so.
    // This is done by flipping all 'bad' voxel features that do not touch the outside of the sample (i.e. they are fully contained inside the 'sample').
    threshold = 0.0F;
    if(fillHoles)
    {
      bool touchesBoundary = false;
      for(int64 i = 0; i < totalPoints; i++)
      {
        const float percentIncrement = static_cast<float>(i) / static_cast<float>(totalPoints) * 100.0f;
        if(percentIncrement > threshold)
        {
          threshold = threshold + 5.0f;
          if(threshold < percentIncrement)
          {
            threshold = percentIncrement;
          }
        }

        if(!checked[i] && !goodVoxels.getValue(i))
        {
          currentVList.push_back(i);
          usize count = 0;
          touchesBoundary = false;
          while(count < currentVList.size())
          {
            int64 index = currentVList[count];
            int64 column = index % xp;
            int64 row = (index / xp) % yp;
            int64 plane = index / (xp * yp);
            if(column == 0 || column == (xp - 1) || row == 0 || row == (yp - 1) || plane == 0 || plane == (zp - 1))
            {
              touchesBoundary = true;
            }
            for(int32 j = 0; j < 6; j++)
            {
              int32 good = 1;
              neighbor = index + neighborPoints[j];
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
                currentVList.push_back(neighbor);
                checked[neighbor] = true;
              }
            }
            count++;
          }
          if(!touchesBoundary)
          {
            for(int64_t j : currentVList)
            {
              goodVoxels.setValue(j, true);
            }
          }
          currentVList.clear();
        }
      }
    }
    checked.clear();
  }
};

struct IdentifySampleSliceBySliceFunctor
{
  enum class Plane
  {
    XY,
    XZ,
    YZ
  };

  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, Plane plane)
  {
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    SizeVec3 uDims = imageGeom->getDimensions();
    const int64 dimX = static_cast<int64>(uDims[0]);
    const int64 dimY = static_cast<int64>(uDims[1]);
    const int64 dimZ = static_cast<int64>(uDims[2]);

    int64 planeDim1, planeDim2, fixedDim;
    int64 stride1, stride2, fixedStride;

    switch(plane)
    {
    case Plane::XY:
      planeDim1 = dimX;
      planeDim2 = dimY;
      fixedDim = dimZ;
      stride1 = 1;
      stride2 = dimX;
      fixedStride = dimX * dimY;
      break;

    case Plane::XZ:
      planeDim1 = dimX;
      planeDim2 = dimZ;
      fixedDim = dimY;
      stride1 = 1;
      stride2 = dimX * dimY;
      fixedStride = dimX;
      break;

    case Plane::YZ:
      planeDim1 = dimY;
      planeDim2 = dimZ;
      fixedDim = dimX;
      stride1 = dimX;
      stride2 = dimX * dimY;
      fixedStride = 1;
      break;
    }

    for(int64 fixedIdx = 0; fixedIdx < fixedDim; ++fixedIdx) // Process each slice
    {
      std::vector<bool> checked(planeDim1 * planeDim2, false);
      std::vector<bool> sample(planeDim1 * planeDim2, false);
      std::vector<int64> currentVList;
      int64 biggestBlock = 0;

      // Identify the largest contiguous set of good voxels in the slice
      for(int64 p2 = 0; p2 < planeDim2; ++p2)
      {
        for(int64 p1 = 0; p1 < planeDim1; ++p1)
        {
          int64 planeIndex = p2 * planeDim1 + p1;
          int64 globalIndex = fixedIdx * fixedStride + p2 * stride2 + p1 * stride1;

          if(!checked[planeIndex] && goodVoxels.getValue(globalIndex))
          {
            currentVList.push_back(planeIndex);
            int64 count = 0;

            while(count < currentVList.size())
            {
              int64 localIdx = currentVList[count];
              int64 localP1 = localIdx % planeDim1;
              int64 localP2 = localIdx / planeDim1;

              for(int j = 0; j < 4; ++j)
              {
                int64 dp1[4] = {0, 0, -1, 1};
                int64 dp2[4] = {-1, 1, 0, 0};

                int64 neighborP1 = localP1 + dp1[j];
                int64 neighborP2 = localP2 + dp2[j];

                if(neighborP1 >= 0 && neighborP1 < planeDim1 && neighborP2 >= 0 && neighborP2 < planeDim2)
                {
                  int64 neighborIdx = neighborP2 * planeDim1 + neighborP1;
                  int64 globalNeighborIdx = fixedIdx * fixedStride + neighborP2 * stride2 + neighborP1 * stride1;

                  if(!checked[neighborIdx] && goodVoxels.getValue(globalNeighborIdx))
                  {
                    currentVList.push_back(neighborIdx);
                    checked[neighborIdx] = true;
                  }
                }
              }
              count++;
            }

            if(static_cast<int64>(currentVList.size()) > biggestBlock)
            {
              biggestBlock = currentVList.size();
              sample.assign(planeDim1 * planeDim2, false);
              for(int64 idx : currentVList)
              {
                sample[idx] = true;
              }
            }
            currentVList.clear();
          }
        }
      }

      for(int64 p2 = 0; p2 < planeDim2; ++p2)
      {
        for(int64 p1 = 0; p1 < planeDim1; ++p1)
        {
          int64 planeIndex = p2 * planeDim1 + p1;
          int64 globalIndex = fixedIdx * fixedStride + p2 * stride2 + p1 * stride1;

          if(!sample[planeIndex])
          {
            goodVoxels.setValue(globalIndex, false);
          }
        }
      }

      if(fillHoles)
      {
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          for(int64 p1 = 0; p1 < planeDim1; ++p1)
          {
            int64 planeIndex = p2 * planeDim1 + p1;
            int64 globalIndex = fixedIdx * fixedStride + p2 * stride2 + p1 * stride1;

            if(!checked[planeIndex] && !goodVoxels.getValue(globalIndex))
            {
              currentVList.push_back(planeIndex);
              int64 count = 0;
              bool touchesBoundary = false;

              while(count < currentVList.size())
              {
                int64 localIdx = currentVList[count];
                int64 localP1 = localIdx % planeDim1;
                int64 localP2 = localIdx / planeDim1;

                if(localP1 == 0 || localP1 == planeDim1 - 1 || localP2 == 0 || localP2 == planeDim2 - 1)
                {
                  touchesBoundary = true;
                }

                for(int j = 0; j < 4; ++j)
                {
                  int64 dp1[4] = {0, 0, -1, 1};
                  int64 dp2[4] = {-1, 1, 0, 0};

                  int64 neighborP1 = localP1 + dp1[j];
                  int64 neighborP2 = localP2 + dp2[j];

                  if(neighborP1 >= 0 && neighborP1 < planeDim1 && neighborP2 >= 0 && neighborP2 < planeDim2)
                  {
                    int64 neighborIdx = neighborP2 * planeDim1 + neighborP1;
                    int64 globalNeighborIdx = fixedIdx * fixedStride + neighborP2 * stride2 + neighborP1 * stride1;

                    if(!checked[neighborIdx] && !goodVoxels.getValue(globalNeighborIdx))
                    {
                      currentVList.push_back(neighborIdx);
                      checked[neighborIdx] = true;
                    }
                  }
                }
                count++;
              }

              if(!touchesBoundary)
              {
                for(int64 idx : currentVList)
                {
                  goodVoxels.setValue(fixedIdx * fixedStride + idx, true);
                }
              }
              currentVList.clear();
            }
          }
        }
      }
    }
  }
};
} // namespace

//------------------------------------------------------------------------------
std::string IdentifySampleFilter::name() const
{
  return FilterTraits<IdentifySampleFilter>::name;
}

//------------------------------------------------------------------------------
std::string IdentifySampleFilter::className() const
{
  return FilterTraits<IdentifySampleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid IdentifySampleFilter::uuid() const
{
  return FilterTraits<IdentifySampleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string IdentifySampleFilter::humanName() const
{
  return "Isolate Largest Feature (Identify Sample)";
}

//------------------------------------------------------------------------------
std::vector<std::string> IdentifySampleFilter::defaultTags() const
{
  return {className(), "Core", "Identify Sample"};
}

//------------------------------------------------------------------------------
Parameters IdentifySampleFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_FillHoles_Key, "Fill Holes in Largest Feature", "Whether to fill holes within sample after it is identified", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SliceBySlice_Key, "Process Data Slice-By-Slice",
                                                                 "Whether to identify the largest sample (and optionally fill holes) slice-by-slice.  This option is useful if you have a sample that "
                                                                 "is not water-tight and the holes open up to the overscan section, or if you have holes that sit on a boundary.  The original "
                                                                 "algorithm will not fill holes that have these characteristics, only holes that are completely enclosed by the sample and "
                                                                 "water-tight.  If you have holes that are not water-tight or sit on a boundary, choose this option and then pick the plane that will "
                                                                 "allow the holes to be water-tight on each slice of that plane.",
                                                                 false));
  params.insert(
      std::make_unique<ChoicesParameter>(k_SliceBySlicePlane_Key, "Slice-By-Slice Plane",
                                         "Set the plane that the data will be processed slice-by-slice.  For example, if you pick the XY plane, the data will be processed in the Z direction.", 0,
                                         ChoicesParameter::Choices{"XY", "XZ", "YZ"}));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "DataPath to the target ImageGeom", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the mask array defining what is sample and what is not", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::boolean, nx::core::DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.linkParameters(k_SliceBySlice_Key, k_SliceBySlicePlane_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType IdentifySampleFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IdentifySampleFilter::clone() const
{
  return std::make_unique<IdentifySampleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IdentifySampleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  const auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto goodVoxelsArrayPath = args.value<DataPath>(k_MaskArrayPath_Key);

  const auto& inputData = dataStructure.getDataRefAs<IDataArray>(goodVoxelsArrayPath);
  const DataType arrayType = inputData.getDataType();
  if(arrayType != DataType::boolean && arrayType != DataType::uint8)
  {
    return MakePreflightErrorResult(-12001, fmt::format("The input data must be of type BOOL or UINT8"));
  }

  Result<OutputActions> outputActions;
  std::vector<PreflightValue> outputValues;
  return {std::move(outputActions), std::move(outputValues)};
}

//------------------------------------------------------------------------------
Result<> IdentifySampleFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  const auto fillHoles = args.value<bool>(k_FillHoles_Key);
  const auto sliceBySlice = args.value<bool>(k_SliceBySlice_Key);
  const auto sliceBySlicePlane = static_cast<IdentifySampleSliceBySliceFunctor::Plane>(args.value<ChoicesParameter::ValueType>(k_SliceBySlicePlane_Key));
  const auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto goodVoxelsArrayPath = args.value<DataPath>(k_MaskArrayPath_Key);

  auto* inputData = dataStructure.getDataAs<IDataArray>(goodVoxelsArrayPath);
  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);

  if(sliceBySlice)
  {
    ExecuteDataFunction(IdentifySampleSliceBySliceFunctor{}, inputData->getDataType(), imageGeom, inputData, fillHoles, sliceBySlicePlane);
  }
  else
  {
    ExecuteDataFunction(IdentifySampleFunctor{}, inputData->getDataType(), imageGeom, inputData, fillHoles);
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FillHolesKey = "FillHoles";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> IdentifySampleFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = IdentifySampleFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FillHolesKey, k_FillHoles_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
