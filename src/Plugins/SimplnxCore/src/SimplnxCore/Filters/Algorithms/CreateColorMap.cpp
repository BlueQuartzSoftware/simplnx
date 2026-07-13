#include "CreateColorMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <array>

using namespace nx::core;

namespace
{
constexpr usize k_ControlPointCompSize = 4;

/**
 * @brief The CreateColorMapImpl class implements a threaded algorithm that computes the RGB values
 * for each element in a given array of data
 */
template <typename T>
class CreateColorMapImpl
{
public:
  CreateColorMapImpl(const AbstractDataStore<T>& arrayStore, const std::vector<float32>& binPoints, const std::vector<float32>& controlPoints, int numControlColors, UInt8AbstractDataStore& colorStore,
                     const nx::core::IDataArray* goodVoxels, const std::vector<uint8>& invalidColor)
  : m_ArrayStore(arrayStore)
  , m_BinPoints(binPoints)
  , m_ArrayMin(arrayStore[0])
  , m_ArrayMax(arrayStore[0])
  , m_NumControlColors(numControlColors)
  , m_ControlPoints(controlPoints)
  , m_ColorStore(colorStore)
  , m_GoodVoxels(goodVoxels)
  , m_InvalidColor(invalidColor)
  {
    for(usize i = 1; i < arrayStore.getNumberOfTuples(); i++)
    {
      if(arrayStore[i] < m_ArrayMin)
      {
        m_ArrayMin = arrayStore[i];
      }
      if(arrayStore[i] > m_ArrayMax)
      {
        m_ArrayMax = arrayStore[i];
      }
    }
  }

  template <typename K>
  void convert(usize start, usize end) const
  {
    using MaskArrayType = DataArray<K>;
    const MaskArrayType* maskArray = nullptr;
    if(nullptr != m_GoodVoxels)
    {
      maskArray = dynamic_cast<const MaskArrayType*>(m_GoodVoxels);
    }

    for(size_t i = start; i < end; i++)
    {
      // Make sure we are using a valid voxel based on the "goodVoxels" arrays
      if(nullptr != maskArray)
      {
        if(!(*maskArray)[i])
        {
          m_ColorStore.setComponent(i, 0, m_InvalidColor[0]);
          m_ColorStore.setComponent(i, 1, m_InvalidColor[1]);
          m_ColorStore.setComponent(i, 2, m_InvalidColor[2]);
          continue;
        }
      }

      // Normalize value (with min==max guard) and interpolate an RGB triple via the shared helper.
      const float32 nValue = ColorTableUtilities::NormalizeValue(m_ArrayStore[i], m_ArrayMin, m_ArrayMax);
      const std::array<uint8, 3> rgb = ColorTableUtilities::ComputeRgbFromControlPoints(nValue, m_BinPoints, m_ControlPoints, static_cast<usize>(m_NumControlColors));

      m_ColorStore.setComponent(i, 0, rgb[0]);
      m_ColorStore.setComponent(i, 1, rgb[1]);
      m_ColorStore.setComponent(i, 2, rgb[2]);
    }
  }

  void operator()(const Range& range) const
  {
    if(m_GoodVoxels != nullptr)
    {
      if(m_GoodVoxels->getDataType() == DataType::boolean)
      {
        convert<bool>(range.min(), range.max());
      }
      else if(m_GoodVoxels->getDataType() == DataType::uint8)
      {
        convert<uint8>(range.min(), range.max());
      }
    }
    else
    {
      convert<bool>(range.min(), range.max());
    }
  }

private:
  const AbstractDataStore<T>& m_ArrayStore;
  const std::vector<float32>& m_BinPoints;
  T m_ArrayMin;
  T m_ArrayMax;
  int m_NumControlColors;
  const std::vector<float32>& m_ControlPoints;
  UInt8AbstractDataStore& m_ColorStore;
  const nx::core::IDataArray* m_GoodVoxels = nullptr;
  const std::vector<uint8>& m_InvalidColor;
};

struct GenerateColorArrayFunctor
{
  template <typename ScalarType>
  Result<> operator()(DataStructure& dataStructure, const CreateColorMapInputValues* inputValues, const std::vector<float32>& controlPoints)
  {
    // Control Points is a flattened 2D array with an unknown tuple count and a component size of 4
    const usize numControlColors = controlPoints.size() / k_ControlPointCompSize;

    // Store normalized A-values in binPoints via the shared helper.
    std::vector<float32> binPoints = ColorTableUtilities::NormalizeBinPoints(controlPoints);

    auto& colorArray = dataStructure.getDataAs<UInt8Array>(inputValues->RgbArrayPath)->getDataStoreRef();

    nx::core::IDataArray* goodVoxelsArray = nullptr;
    if(inputValues->UseMask)
    {
      goodVoxelsArray = dataStructure.getDataAs<IDataArray>(inputValues->MaskArrayPath);
    }

    const AbstractDataStore<ScalarType>& arrayRef = dataStructure.getDataAs<DataArray<ScalarType>>(inputValues->SelectedDataArrayPath)->getDataStoreRef();
    if(arrayRef.getNumberOfTuples() <= 0)
    {
      return MakeErrorResult(-34381, fmt::format("Array {} is empty", inputValues->SelectedDataArrayPath.getTargetName()));
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, arrayRef.getNumberOfTuples());
    dataAlg.execute(CreateColorMapImpl(arrayRef, binPoints, controlPoints, numControlColors, colorArray, goodVoxelsArray, inputValues->InvalidColor));
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
CreateColorMap::CreateColorMap(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, CreateColorMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
CreateColorMap::~CreateColorMap() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateColorMap::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateColorMap::operator()()
{
  const IDataArray& selectedIDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedDataArrayPath);

  auto controlPointsResult = ColorTableUtilities::ExtractControlPoints(m_InputValues->PresetName);
  if(controlPointsResult.invalid())
  {
    auto error = *controlPointsResult.errors().begin();
    return MakeErrorResult(error.code, error.message);
  }
  auto controlPoints = controlPointsResult.value();
  if(controlPoints.empty())
  {
    return MakeErrorResult(-34380, fmt::format("No valid points found from preset {}", m_InputValues->PresetName));
  }

  ExecuteDataFunction(GenerateColorArrayFunctor{}, selectedIDataArray.getDataType(), m_DataStructure, m_InputValues, controlPoints);
  return {};
}
