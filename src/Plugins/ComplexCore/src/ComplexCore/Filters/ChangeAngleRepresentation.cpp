#include "ChangeAngleRepresentation.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <cmath>
#include <fmt/format.h>

using namespace complex;

namespace
{

namespace EulerAngleConversionType
{
constexpr uint64 DegreesToRadians = 0;
constexpr uint64 RadiansToDegrees = 1;
} // namespace EulerAngleConversionType

class ChangeAngleRepresentationImpl
{
public:
  ChangeAngleRepresentationImpl(Float32Array& angles, float factor)
  : m_Angles(angles)
  , m_ConvFactor(factor)
  {
  }
  ~ChangeAngleRepresentationImpl() noexcept = default;

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      m_Angles[i] = m_Angles[i] * m_ConvFactor;
    }
  }

  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_Angles;
  float m_ConvFactor = 0.0F;
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ChangeAngleRepresentation::name() const
{
  return FilterTraits<ChangeAngleRepresentation>::name.str();
}

//------------------------------------------------------------------------------
std::string ChangeAngleRepresentation::className() const
{
  return FilterTraits<ChangeAngleRepresentation>::className;
}

//------------------------------------------------------------------------------
Uuid ChangeAngleRepresentation::uuid() const
{
  return FilterTraits<ChangeAngleRepresentation>::uuid;
}

//------------------------------------------------------------------------------
std::string ChangeAngleRepresentation::humanName() const
{
  return "Convert Angles to Degrees or Radians";
}

//------------------------------------------------------------------------------
std::vector<std::string> ChangeAngleRepresentation::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ChangeAngleRepresentation::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "", 0, ChoicesParameter::Choices{"Degrees to Radians", "Radians to Degrees"}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AnglesArrayPath_Key, "Angles", "The DataArray containing the angles to be converted.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ChangeAngleRepresentation::clone() const
{
  return std::make_unique<ChangeAngleRepresentation>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ChangeAngleRepresentation::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pConversionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

  if(pConversionTypeValue > 1)
  {
    return {MakeErrorResult<OutputActions>(-67001, fmt::format("The conversion type must be either [0|1]. Value given is '{}'", pConversionTypeValue))};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> ChangeAngleRepresentation::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pConversionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);
  auto pAnglesDataPathValue = filterArgs.value<DataPath>(k_AnglesArrayPath_Key);

  Float32Array& angles = dataStructure.getDataRefAs<Float32Array>(pAnglesDataPathValue);

  float conversionFactor = 1.0f;
  if(pConversionTypeValue == EulerAngleConversionType::DegreesToRadians)
  {
    conversionFactor = static_cast<float>(complex::numbers::pi / 180.0f);
  }
  else if(pConversionTypeValue == EulerAngleConversionType::RadiansToDegrees)
  {
    conversionFactor = static_cast<float>(180.0f / complex::numbers::pi);
  }

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, angles.getSize());
  dataAlg.execute(::ChangeAngleRepresentationImpl(angles, conversionFactor));

  return {};
}
} // namespace complex
