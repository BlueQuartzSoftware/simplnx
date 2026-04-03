#include "ConvertQuaternion.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Utilities/EbsdStringUtils.hpp>

using namespace nx::core;

namespace
{

constexpr ChoicesParameter::ValueType k_ToScalarVector = 0;
constexpr ChoicesParameter::ValueType k_ToVectorScalar = 1;

template <typename T>
class ConvertQuaternionImpl
{

public:
  ConvertQuaternionImpl(const DataArray<T>& inputQuat, DataArray<T>& outputQuat, ChoicesParameter::ValueType conversionType, const std::atomic_bool& shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ConversionType(conversionType)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void convert(size_t start, size_t end) const
  {
    // Let's assume k_ToScalarVector which means the incoming quaternions are Vector-Scalar
    // <x,y,z> w  ---> w <x,y,z>
    std::array<size_t, 4> mapping = {{1, 2, 3, 0}};

    if(m_ConversionType == ::k_ToVectorScalar) // Ensure the Quaternion is the proper order
    {
      // w <x,y,z>  ---> <x,y,z> w
      mapping = {{3, 0, 1, 2}};
    }

    std::array<float, 4> temp = {0.0f, 0.0f, 0.0f, 0.0f};
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      temp[mapping[0]] = m_Input[i * 4];
      temp[mapping[1]] = m_Input[i * 4 + 1];
      temp[mapping[2]] = m_Input[i * 4 + 2];
      temp[mapping[3]] = m_Input[i * 4 + 3];

      m_Output[i * 4] = temp[0];
      m_Output[i * 4 + 1] = temp[1];
      m_Output[i * 4 + 2] = temp[2];
      m_Output[i * 4 + 3] = temp[3];
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const DataArray<T>& m_Input;
  DataArray<T>& m_Output;
  ChoicesParameter::ValueType m_ConversionType = 0;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
ConvertQuaternion::ConvertQuaternion(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertQuaternionInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertQuaternion::~ConvertQuaternion() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertQuaternion::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertQuaternion::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Converting Quaternions...");

  const auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->QuaternionDataArrayPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, iDataArray.getNumberOfTuples());

  if(iDataArray.getDataType() == DataType::float32)
  {
    const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuaternionDataArrayPath);
    auto& convertedQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);
    dataAlg.execute(ConvertQuaternionImpl<float32>(quats, convertedQuats, m_InputValues->ConversionType, m_ShouldCancel));
  }
  else if(iDataArray.getDataType() == DataType::float64)
  {
    const auto& quats = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->QuaternionDataArrayPath);
    auto& convertedQuats = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->OutputDataArrayPath);
    dataAlg.execute(ConvertQuaternionImpl<float64>(quats, convertedQuats, m_InputValues->ConversionType, m_ShouldCancel));
  }
  else
  {
    return MakeErrorResult(-74836, fmt::format("The input and output arrays must be either Float32 or Float64 type"));
  }
  return {};
}
