#include "ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{

constexpr ChoicesParameter::ValueType k_ToScalarVector = 0;
constexpr ChoicesParameter::ValueType k_ToVectorScalar = 1;

class ConvertHexGridToSquareGridImpl
{

public:
  ConvertHexGridToSquareGridImpl(const Float32Array& inputQuat, Float32Array& outputQuat, ChoicesParameter::ValueType conversionType, const std::atomic_bool& shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ConversionType(conversionType)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~ConvertHexGridToSquareGridImpl() = default;

  ConvertHexGridToSquareGridImpl(const ConvertHexGridToSquareGridImpl&) = default;           // Copy Constructor Default Implemented
  ConvertHexGridToSquareGridImpl(ConvertHexGridToSquareGridImpl&&) = delete;                 // Move Constructor Not Implemented
  ConvertHexGridToSquareGridImpl& operator=(const ConvertHexGridToSquareGridImpl&) = delete; // Copy Assignment Not Implemented
  ConvertHexGridToSquareGridImpl& operator=(ConvertHexGridToSquareGridImpl&&) = delete;      // Move Assignment Not Implemented

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
  const Float32Array& m_Input;
  Float32Array& m_Output;
  ChoicesParameter::ValueType m_ConversionType = 0;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::ConvertHexGridToSquareGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertHexGridToSquareGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::~ConvertHexGridToSquareGrid() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertHexGridToSquareGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGrid::operator()()
{
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuaternionDataArrayPath);
  auto& convertedQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, quats.getNumberOfTuples());
  dataAlg.execute(ConvertHexGridToSquareGridImpl(quats, convertedQuats, m_InputValues->ConversionType, m_ShouldCancel));

  return {};
}
