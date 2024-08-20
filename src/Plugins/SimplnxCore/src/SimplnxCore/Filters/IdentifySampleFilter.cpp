#include "IdentifySampleFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
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
        count = 0;
        while(count < currentVList.size())
        {
          index = currentVList[count];
          column = index % xp;
          row = (index / xp) % yp;
          plane = index / (xp * yp);
          for(int32 j = 0; j < 6; j++)
          {
            good = 1;
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
          count = 0;
          touchesBoundary = false;
          while(count < currentVList.size())
          {
            index = currentVList[count];
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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "DataPath to the target ImageGeom", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the mask array defining what is sample and what is not", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::boolean, nx::core::DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  return params;
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
  const auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto goodVoxelsArrayPath = args.value<DataPath>(k_MaskArrayPath_Key);

  auto* inputData = dataStructure.getDataAs<IDataArray>(goodVoxelsArrayPath);
  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);

  ExecuteDataFunction(IdentifySampleFunctor{}, inputData->getDataType(), imageGeom, inputData, fillHoles);

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
