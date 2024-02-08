#include "GenerateColorTable.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
usize findRightBinIndex(float32 nValue, const std::vector<float32>& binPoints)
{
  usize min = 0;
  usize max = binPoints.size() - 1;
  while(min < max)
  {
    const usize middle = (min + max) / 2;
    if(nValue > binPoints[middle])
    {
      min = middle + 1;
    }
    else
    {
      max = middle;
    }
  }
  return min;
}

/**
 * @brief The GenerateColorTableImpl class implements a threaded algorithm that computes the RGB values
 * for each element in a given array of data
 */
template <typename T>
class GenerateColorTableImpl
{
public:
  GenerateColorTableImpl(const DataArray<T>& arrayPtr, const std::vector<float32>& binPoints, const GenerateColorTableParameter::ValueType& controlPoints, int numControlColors, UInt8Array& colorArray,
                         const nx::core::IDataArray* goodVoxels, const std::vector<uint8>& invalidColor)
  : m_ArrayPtr(arrayPtr)
  , m_BinPoints(binPoints)
  , m_NumControlColors(numControlColors)
  , m_ControlPoints(controlPoints)
  , m_ColorArray(colorArray)
  , m_ArrayMin(arrayPtr[0])
  , m_ArrayMax(arrayPtr[0])
  , m_GoodVoxels(goodVoxels)
  , m_InvalidColor(invalidColor)
  , m_CompSize(GenerateColorTableParameter::k_ControlPointCompSize)
  {
    for(int i = 1; i < arrayPtr.getNumberOfTuples(); i++)
    {
      if(arrayPtr[i] < m_ArrayMin)
      {
        m_ArrayMin = arrayPtr[i];
      }
      if(arrayPtr[i] > m_ArrayMax)
      {
        m_ArrayMax = arrayPtr[i];
      }
    }
  }
  virtual ~GenerateColorTableImpl() = default;

  GenerateColorTableImpl(const GenerateColorTableImpl&) = default;
  GenerateColorTableImpl(GenerateColorTableImpl&&) noexcept = delete;
  GenerateColorTableImpl& operator=(const GenerateColorTableImpl&) = delete;
  GenerateColorTableImpl& operator=(GenerateColorTableImpl&&) noexcept = delete;

  template <typename K>
  void convert(size_t start, size_t end) const
  {
    using MaskArrayType = DataArray<K>;
    const MaskArrayType* maskArray = nullptr;
    if(nullptr != m_GoodVoxels)
    {
      maskArray = dynamic_cast<const MaskArrayType*>(m_GoodVoxels);
    }
    auto& colorArrayDS = m_ColorArray.getDataStoreRef();

    for(size_t i = start; i < end; i++)
    {
      // Make sure we are using a valid voxel based on the "goodVoxels" arrays
      if(nullptr != maskArray)
      {
        if(!(*maskArray)[i])
        {
          colorArrayDS.setComponent(i, 0, m_InvalidColor[0]);
          colorArrayDS.setComponent(i, 1, m_InvalidColor[1]);
          colorArrayDS.setComponent(i, 2, m_InvalidColor[2]);
          continue;
        }
      }

      // Normalize value
      const float32 nValue = (static_cast<float32>(m_ArrayPtr[i] - m_ArrayMin)) / static_cast<float32>((m_ArrayMax - m_ArrayMin));

      int rightBinIndex = findRightBinIndex(nValue, m_BinPoints);

      int leftBinIndex = rightBinIndex - 1;
      if(leftBinIndex < 0)
      {
        leftBinIndex = 0;
        rightBinIndex = 1;
      }

      // Find the fractional distance traveled between the beginning and end of the current color bin
      float32 currFraction;
      if(rightBinIndex < m_BinPoints.size())
      {
        currFraction = (nValue - m_BinPoints[leftBinIndex]) / (m_BinPoints[rightBinIndex] - m_BinPoints[leftBinIndex]);
      }
      else
      {
        currFraction = (nValue - m_BinPoints[leftBinIndex]) / (1 - m_BinPoints[leftBinIndex]);
      }

      // If the current color bin index is larger than the total number of control colors, automatically set the currentBinIndex
      // to the last control color.
      if(leftBinIndex > m_NumControlColors - 1)
      {
        leftBinIndex = m_NumControlColors - 1;
      }

      // Calculate the RGB values
      const unsigned char redVal = (m_ControlPoints[leftBinIndex * m_CompSize + 1] * (1.0 - currFraction) + m_ControlPoints[rightBinIndex * m_CompSize + 1] * currFraction) * 255;
      const unsigned char greenVal = (m_ControlPoints[leftBinIndex * m_CompSize + 2] * (1.0 - currFraction) + m_ControlPoints[rightBinIndex * m_CompSize + 2] * currFraction) * 255;
      const unsigned char blueVal = (m_ControlPoints[leftBinIndex * m_CompSize + 3] * (1.0 - currFraction) + m_ControlPoints[rightBinIndex * m_CompSize + 3] * currFraction) * 255;

      colorArrayDS.setComponent(i, 0, redVal);
      colorArrayDS.setComponent(i, 1, greenVal);
      colorArrayDS.setComponent(i, 2, blueVal);
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
  const DataArray<T>& m_ArrayPtr;
  const std::vector<float32>& m_BinPoints;
  T m_ArrayMin;
  T m_ArrayMax;
  int m_NumControlColors;
  const GenerateColorTableParameter::ValueType& m_ControlPoints;
  UInt8Array& m_ColorArray;
  const nx::core::IDataArray* m_GoodVoxels = nullptr;
  const std::vector<uint8>& m_InvalidColor;
  const usize m_CompSize;
};

struct GenerateColorArrayFunctor
{
  template <typename ScalarType>
  void operator()(DataStructure& dataStructure, const GenerateColorTableInputValues* inputValues)
  {
    // Control Points is a flattened 2D array with an unknown tuple count and a component size of 4
    const GenerateColorTableParameter::ValueType& controlPoints = inputValues->ControlPoints;
    const usize numControlColors = controlPoints.size() / GenerateColorTableParameter::k_ControlPointCompSize;

    // Store A-values in binPoints vector.
    std::vector<float32> binPoints;
    binPoints.reserve(numControlColors);
    for(usize i = 0; i < numControlColors; i++)
    {
      binPoints.push_back(static_cast<float32>(controlPoints[i * GenerateColorTableParameter::k_ControlPointCompSize]));
    }

    // Normalize binPoints values
    const float32 binMin = binPoints[0];
    const float32 binMax = binPoints[binPoints.size() - 1];
    for(auto& binPoint : binPoints)
    {
      binPoint = (binPoint - binMin) / (binMax - binMin);
    }

    auto& colorArray = dataStructure.getDataRefAs<UInt8Array>(inputValues->RgbArrayPath);

    nx::core::IDataArray* goodVoxelsArray = nullptr;
    if(inputValues->UseMask)
    {
      goodVoxelsArray = dataStructure.getDataAs<IDataArray>(inputValues->MaskArrayPath);
    }

    const DataArray<ScalarType>& arrayPtr = dataStructure.getDataRefAs<DataArray<ScalarType>>(inputValues->SelectedDataArrayPath);
    if(arrayPtr.getNumberOfTuples() <= 0)
    {
      return;
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, arrayPtr.getNumberOfTuples());
    dataAlg.execute(GenerateColorTableImpl(arrayPtr, binPoints, controlPoints, numControlColors, colorArray, goodVoxelsArray, inputValues->InvalidColor));
  }
};
} // namespace

// -----------------------------------------------------------------------------
GenerateColorTable::GenerateColorTable(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, GenerateColorTableInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateColorTable::~GenerateColorTable() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateColorTable::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateColorTable::operator()()
{
  const IDataArray& selectedIDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedDataArrayPath);
  ExecuteDataFunction(GenerateColorArrayFunctor{}, selectedIDataArray.getDataType(), m_DataStructure, m_InputValues);
  return {};
}
