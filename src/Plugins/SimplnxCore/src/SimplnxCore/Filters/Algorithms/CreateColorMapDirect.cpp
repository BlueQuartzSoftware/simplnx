#include "CreateColorMapDirect.hpp"

#include "CreateColorMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <array>
#include <type_traits>

using namespace nx::core;

namespace
{
// A control point stores scalar position and RGB components.
constexpr usize k_ControlPointCompSize = 4;
constexpr usize k_ColorComponentCount = 3;

/**
 * @class AbstractStoreAccess
 * @brief Accesses scalar, mask, and RGB values through DataStore methods.
 * @tparam T Specifies the scalar input type.
 *
 * This direct fallback accesses DataStore instances in parallel and has no
 * general thread-safety guarantee.
 */
template <typename T>
class AbstractStoreAccess
{
public:
  AbstractStoreAccess(const AbstractDataStore<T>& arrayStore, UInt8AbstractDataStore& colorStore, const IDataArray* maskArray)
  : m_ArrayStore(arrayStore)
  , m_ColorStore(colorStore)
  , m_MaskArray(maskArray)
  {
    if(maskArray != nullptr)
    {
      if(maskArray->getDataType() == DataType::boolean)
      {
        m_BoolMaskStore = &maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>();
      }
      else if(maskArray->getDataType() == DataType::uint8)
      {
        m_UInt8MaskStore = &maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>();
      }
    }
  }

  T inputValue(usize index) const
  {
    return m_ArrayStore[index];
  }

  template <typename MaskType>
  bool maskValue(usize index) const
  {
    if constexpr(std::is_same_v<MaskType, bool>)
    {
      return (*m_BoolMaskStore)[index];
    }
    else
    {
      return (*m_UInt8MaskStore)[index] != 0;
    }
  }

  void setColor(usize index, uint8 red, uint8 green, uint8 blue) const
  {
    m_ColorStore.setComponent(index, 0, red);
    m_ColorStore.setComponent(index, 1, green);
    m_ColorStore.setComponent(index, 2, blue);
  }

  const IDataArray* maskArray() const
  {
    return m_MaskArray;
  }

private:
  const AbstractDataStore<T>& m_ArrayStore;
  UInt8AbstractDataStore& m_ColorStore;
  const IDataArray* m_MaskArray = nullptr;
  const AbstractDataStore<bool>* m_BoolMaskStore = nullptr;
  const AbstractDataStore<uint8>* m_UInt8MaskStore = nullptr;
};

/**
 * @class ContiguousStoreAccess
 * @brief Accesses scalar, mask, and RGB values through raw pointers.
 * @tparam T Specifies the scalar input type.
 *
 * Raw pointers avoid DataStore access during parallel mapping.
 */
template <typename T>
class ContiguousStoreAccess
{
public:
  ContiguousStoreAccess(const DataStore<T>& arrayStore, UInt8DataStore& colorStore, const IDataArray* maskArray, const BoolDataStore* boolMaskStore, const UInt8DataStore* uint8MaskStore)
  : m_ArrayData(arrayStore.data())
  , m_ColorData(colorStore.data())
  , m_MaskArray(maskArray)
  , m_BoolMaskData(boolMaskStore == nullptr ? nullptr : boolMaskStore->data())
  , m_UInt8MaskData(uint8MaskStore == nullptr ? nullptr : uint8MaskStore->data())
  {
  }

  T inputValue(usize index) const
  {
    return m_ArrayData[index];
  }

  template <typename MaskType>
  bool maskValue(usize index) const
  {
    if constexpr(std::is_same_v<MaskType, bool>)
    {
      return m_BoolMaskData[index];
    }
    else
    {
      return m_UInt8MaskData[index] != 0;
    }
  }

  void setColor(usize index, uint8 red, uint8 green, uint8 blue) const
  {
    const usize colorIndex = index * k_ColorComponentCount;
    m_ColorData[colorIndex] = red;
    m_ColorData[colorIndex + 1] = green;
    m_ColorData[colorIndex + 2] = blue;
  }

  const IDataArray* maskArray() const
  {
    return m_MaskArray;
  }

private:
  const T* m_ArrayData = nullptr;
  uint8* m_ColorData = nullptr;
  const IDataArray* m_MaskArray = nullptr;
  const bool* m_BoolMaskData = nullptr;
  const uint8* m_UInt8MaskData = nullptr;
};

/**
 * @class CreateColorMapImpl
 * @brief Computes RGB values in parallel using either contiguous or abstract data access.
 * @tparam T Specifies the scalar input type.
 * @tparam DataAccess Specifies scalar, mask, and RGB access operations.
 *
 * The constructor finds the scalar range before parallel mapping. Unsupported
 * mask types do not invoke a conversion.
 */
template <typename T, typename DataAccess>
class CreateColorMapImpl
{
public:
  CreateColorMapImpl(DataAccess dataAccess, usize numTuples, const std::vector<float32>& binPoints, const std::vector<float32>& controlPoints, int numControlColors,
                     const std::vector<uint8>& invalidColor)
  : m_DataAccess(dataAccess)
  , m_BinPoints(binPoints)
  , m_ArrayMin(m_DataAccess.inputValue(0))
  , m_ArrayMax(m_DataAccess.inputValue(0))
  , m_NumControlColors(numControlColors)
  , m_ControlPoints(controlPoints)
  , m_InvalidColor(invalidColor)
  {
    for(usize i = 1; i < numTuples; i++)
    {
      const T value = m_DataAccess.inputValue(i);
      if(value < m_ArrayMin)
      {
        m_ArrayMin = value;
      }
      if(value > m_ArrayMax)
      {
        m_ArrayMax = value;
      }
    }
  }

  template <typename K>
  void convert(usize start, usize end) const
  {
    const IDataArray* maskArray = m_DataAccess.maskArray();

    for(usize i = start; i < end; i++)
    {
      if(maskArray != nullptr && !m_DataAccess.template maskValue<K>(i))
      {
        m_DataAccess.setColor(i, m_InvalidColor[0], m_InvalidColor[1], m_InvalidColor[2]);
        continue;
      }

      const float32 normalizedValue = ColorTableUtilities::NormalizeValue(m_DataAccess.inputValue(i), m_ArrayMin, m_ArrayMax);
      const std::array<uint8, 3> rgb = ColorTableUtilities::ComputeRgbFromControlPoints(normalizedValue, m_BinPoints, m_ControlPoints, static_cast<usize>(m_NumControlColors));
      m_DataAccess.setColor(i, rgb[0], rgb[1], rgb[2]);
    }
  }

  void operator()(const Range& range) const
  {
    const IDataArray* maskArray = m_DataAccess.maskArray();
    if(maskArray != nullptr)
    {
      if(maskArray->getDataType() == DataType::boolean)
      {
        convert<bool>(range.min(), range.max());
      }
      else if(maskArray->getDataType() == DataType::uint8)
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
  DataAccess m_DataAccess;
  const std::vector<float32>& m_BinPoints;
  T m_ArrayMin;
  T m_ArrayMax;
  int m_NumControlColors;
  const std::vector<float32>& m_ControlPoints;
  const std::vector<uint8>& m_InvalidColor;
};

/**
 * @struct GenerateColorArrayFunctor
 * @brief Adapts runtime scalar types to direct RGB generation.
 */
struct GenerateColorArrayFunctor
{
  /**
   * @brief Generates RGB values for one scalar type.
   * @tparam ScalarType Specifies the scalar input type.
   * @param dataStructure Provides selected arrays.
   * @param inputValues Specifies validated color-map settings.
   * @param controlPoints Provides flattened scalar-RGB control points.
   * @return Error for an empty input array, or success after mapping.
   */
  template <typename ScalarType>
  Result<> operator()(DataStructure& dataStructure, const CreateColorMapInputValues* inputValues, const std::vector<float32>& controlPoints)
  {
    // Control points store scalar position followed by RGB components.
    const usize numControlColors = controlPoints.size() / k_ControlPointCompSize;

    std::vector<float32> binPoints = ColorTableUtilities::NormalizeBinPoints(controlPoints);

    auto& colorStoreRef = dataStructure.getDataRefAs<UInt8Array>(inputValues->RgbArrayPath).getDataStoreRef();

    const IDataArray* goodVoxelsArray = nullptr;
    if(inputValues->UseMask)
    {
      goodVoxelsArray = dataStructure.getDataAs<IDataArray>(inputValues->MaskArrayPath);
    }

    const auto& arrayStoreRef = dataStructure.getDataRefAs<DataArray<ScalarType>>(inputValues->SelectedDataArrayPath).getDataStoreRef();
    const usize numTuples = arrayStoreRef.getNumberOfTuples();
    if(numTuples == 0)
    {
      return MakeErrorResult(-34381, fmt::format("Array {} is empty", inputValues->SelectedDataArrayPath.getTargetName()));
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numTuples);

    const auto* arrayStore = dynamic_cast<const DataStore<ScalarType>*>(&arrayStoreRef);
    auto* colorStore = dynamic_cast<UInt8DataStore*>(&colorStoreRef);
    const BoolDataStore* boolMaskStore = nullptr;
    const UInt8DataStore* uint8MaskStore = nullptr;
    bool hasContiguousMask = goodVoxelsArray == nullptr;
    if(goodVoxelsArray != nullptr && goodVoxelsArray->getDataType() == DataType::boolean)
    {
      const auto* maskArray = dynamic_cast<const BoolArray*>(goodVoxelsArray);
      boolMaskStore = maskArray == nullptr ? nullptr : dynamic_cast<const BoolDataStore*>(&maskArray->getDataStoreRef());
      hasContiguousMask = boolMaskStore != nullptr;
    }
    else if(goodVoxelsArray != nullptr && goodVoxelsArray->getDataType() == DataType::uint8)
    {
      const auto* maskArray = dynamic_cast<const UInt8Array*>(goodVoxelsArray);
      uint8MaskStore = maskArray == nullptr ? nullptr : dynamic_cast<const UInt8DataStore*>(&maskArray->getDataStoreRef());
      hasContiguousMask = uint8MaskStore != nullptr;
    }

    if(arrayStore != nullptr && colorStore != nullptr && hasContiguousMask)
    {
      ContiguousStoreAccess<ScalarType> dataAccess(*arrayStore, *colorStore, goodVoxelsArray, boolMaskStore, uint8MaskStore);
      dataAlg.execute(CreateColorMapImpl<ScalarType, ContiguousStoreAccess<ScalarType>>(dataAccess, numTuples, binPoints, controlPoints, numControlColors, inputValues->InvalidColor));
    }
    else
    {
      AbstractStoreAccess<ScalarType> dataAccess(arrayStoreRef, colorStoreRef, goodVoxelsArray);
      dataAlg.execute(CreateColorMapImpl<ScalarType, AbstractStoreAccess<ScalarType>>(dataAccess, numTuples, binPoints, controlPoints, numControlColors, inputValues->InvalidColor));
    }
    return {};
  }
};
} // namespace

CreateColorMapDirect::CreateColorMapDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const CreateColorMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

CreateColorMapDirect::~CreateColorMapDirect() noexcept = default;

Result<> CreateColorMapDirect::operator()()
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
  if(controlPoints.size() < 8)
  {
    return MakeErrorResult(-34382, fmt::format("Preset '{}' must define at least 2 control colors", m_InputValues->PresetName));
  }

  // Current direct behavior does not propagate the typed generator Result.
  ExecuteDataFunction(GenerateColorArrayFunctor{}, selectedIDataArray.getDataType(), m_DataStructure, m_InputValues, controlPoints);
  return {};
}
