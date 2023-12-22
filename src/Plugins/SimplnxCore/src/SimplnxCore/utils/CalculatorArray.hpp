#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/ICalculatorArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

namespace nx::core
{

template <typename T>
class SIMPLNXCORE_EXPORT CalculatorArray : public ICalculatorArray
{
public:
  using Self = CalculatorArray<T>;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer()
  {
    return Pointer(static_cast<Self*>(nullptr));
  }

  /**
   * @brief Returns the name of the class for AbstractMessage
   */
  virtual std::string getNameOfClass() const
  {
    return "CalculatorArray<T>";
  }
  /**
   * @brief Returns the name of the class for AbstractMessage
   */
  static std::string ClassName()
  {
    return "CalculatorArray<T>";
  }

  static Pointer New(DataStructure& dataStructure, const DataArray<T>* array, ValueType type, bool allocate)
  {
    return Pointer(new CalculatorArray(dataStructure, array, type, allocate));
  }

  ~CalculatorArray() override = default;

  Float64Array* getArray() override
  {
    if(m_ArrayId.has_value())
    {
      return m_DataStructure.getDataAs<Float64Array>(m_ArrayId.value());
    }
    return nullptr;
  }

  void setValue(int i, double val) override
  {
    if(m_ArrayId.has_value())
    {
      m_DataStructure.getDataRefAs<Float64Array>(m_ArrayId.value())[i] = val;
    }
  }

  double getValue(int i) override
  {
    if(!m_ArrayId.has_value())
    {
      // ERROR: The array is empty!
      return 0.0;
    }
    auto& array = m_DataStructure.getDataRefAs<Float64Array>(m_ArrayId.value());
    if(array.getNumberOfTuples() > 1)
    {
      return static_cast<double>(array[i]);
    }
    if(array.getNumberOfTuples() == 1)
    {
      return static_cast<double>(array[0]);
    }
    // ERROR: The array is empty!
    return 0.0;
  }

  ICalculatorArray::ValueType getType() override
  {
    return m_Type;
  }

  Float64Array* reduceToOneComponent(int c, bool allocate) override
  {
    if(!m_ArrayId.has_value())
    {
      return nullptr;
    }
    auto* array = m_DataStructure.getDataAs<Float64Array>(m_ArrayId.value());
    auto numComponents = array->getNumberOfComponents();
    if(c >= 0 && c <= numComponents)
    {
      if(numComponents > 1)
      {
        DataPath reducedArrayPath = GetUniquePathName(m_DataStructure, array->getDataPaths()[0]); // doesn't matter which path since we only use the target name
        Float64Array* newArray = Float64Array::CreateWithStore<Float64DataStore>(m_DataStructure, reducedArrayPath.getTargetName(), array->getTupleShape(), {1});
        if(allocate)
        {
          for(int i = 0; i < array->getNumberOfTuples(); i++)
          {
            (*newArray)[i] = (*array)[i * numComponents + c];
          }
        }

        return newArray;
      }
    }

    return nullptr;
  }

  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) override
  {
    return CalculatorItem::ErrorCode::Success;
  }

protected:
  CalculatorArray() = default;

  CalculatorArray(DataStructure& dataStructure, const DataArray<T>* dataArray, ValueType type, bool allocate)
  : ICalculatorArray()
  , m_Type(type)
  , m_DataStructure(dataStructure)
  {
    DataPath targetPath({dataArray->getName()});
    if(dataStructure.containsData(targetPath))
    {
      m_ArrayId = dataStructure.getId(targetPath);
    }
    else
    {
      if(allocate)
      {
        auto* tempArray = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, dataArray->getName(), dataArray->getTupleShape(), dataArray->getComponentShape());
        for(int i = 0; i < dataArray->getSize(); i++)
        {
          (*tempArray)[i] = static_cast<double>(dataArray->at(i));
        }
        m_ArrayId = std::optional{tempArray->getId()};
      }
      else
      {
        auto* tempArray =
            Float64Array::Create(dataStructure, dataArray->getName(), std::make_shared<Float64DataStore>(Float64DataStore(nullptr, dataArray->getTupleShape(), dataArray->getComponentShape())));
        m_ArrayId = std::optional{tempArray->getId()};
      }
    }
  }

private:
  std::optional<DataObject::IdType> m_ArrayId = {};
  ValueType m_Type = Unknown;
  DataStructure& m_DataStructure;

public:
  CalculatorArray(const CalculatorArray&) = delete;            // Copy Constructor Not Implemented
  CalculatorArray(CalculatorArray&&) = delete;                 // Move Constructor Not Implemented
  CalculatorArray& operator=(const CalculatorArray&) = delete; // Copy Assignment Not Implemented
  CalculatorArray& operator=(CalculatorArray&&) = delete;      // Move Assignment Not Implemented
};

} // namespace nx::core
