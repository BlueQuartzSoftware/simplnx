#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/Filters/Algorithms/ApplyTransformationToNodeGeometry.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace complex
{
struct RotateArgs
{
  int64 xp = 0;
  int64 yp = 0;
  int64 zp = 0;
  float32 xRes = 0.0f;
  float32 yRes = 0.0f;
  float32 zRes = 0.0f;
  int64 xpNew = 0;
  int64 ypNew = 0;
  int64 zpNew = 0;
  float32 xResNew = 0.0f;
  float32 yResNew = 0.0f;
  float32 zResNew = 0.0f;
  float32 xMinNew = 0.0f;
  float32 yMinNew = 0.0f;
  float32 zMinNew = 0.0f;
};

struct COMPLEXCORE_EXPORT ApplyTransformationToImageGeometryInputValues
{
  DataPath pGeometryToTransform;
  TransformType pTransformationType;
  std::vector<float> transformationMatrix;
  int pInterpolationType;
  RotateArgs rotateArgs;
  bool useArraySelector;
  std::vector<DataPath> selectedArrays;
};

class COMPLEXCORE_EXPORT ApplyTransformationToImageGeometry
{
public:
  ApplyTransformationToImageGeometry(DataStructure& data, ApplyTransformationToImageGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ApplyTransformationToImageGeometry() noexcept;

  ApplyTransformationToImageGeometry(const ApplyTransformationToImageGeometry&) = delete;
  ApplyTransformationToImageGeometry(ApplyTransformationToImageGeometry&&) noexcept = delete;
  ApplyTransformationToImageGeometry& operator=(const ApplyTransformationToImageGeometry&) = delete;
  ApplyTransformationToImageGeometry& operator=(ApplyTransformationToImageGeometry&&) noexcept = delete;

  Result<> operator()();

    template <class T>
  void linearEquivalent(T& linEquivalent, IDataArray& linIData, int64_t linIntIndexes, double xt, double yt, double zt)
  {
    DataArray<T>& lin = std::dynamic_pointer_cast<DataArray<T>>(linIData);
    int index0 = linIntIndexes;
    int index1 = linIntIndexes + 1;
    int index2 = linIntIndexes + p_Impl->m_Params.xp;
    int index3 = linIntIndexes + 1 + p_Impl->m_Params.xp;
    int index4 = linIntIndexes + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    int index5 = linIntIndexes + 1 + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    int index6 = linIntIndexes + p_Impl->m_Params.xp + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    int index7 = linIntIndexes + 1 + p_Impl->m_Params.xp + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    if(index0 >= 0 && index0 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += lin->getPointer(0)[index0];
    }
    if(index1 >= 0 && index1 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index1] - lin->getPointer(0)[index0]) * xt);
    }
    if(index2 >= 0 && index2 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index2] - lin->getPointer(0)[index0]) * yt);
    }
    if(index3 >= 0 && index3 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index3] - lin->getPointer(0)[index2] - lin->getPointer(0)[index1] + lin->getPointer(0)[index0]) * xt * yt);
    }
    if(index4 >= 0 && index4 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index4] - lin->getPointer(0)[index0]) * zt);
    }
    if(index5 >= 0 && index5 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index5] - lin->getPointer(0)[index4] - lin->getPointer(0)[index1] + lin->getPointer(0)[index0]) * xt * zt);
    }
    if(index6 >= 0 && index6 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index6] - lin->getPointer(0)[index4] - lin->getPointer(0)[index2] + lin->getPointer(0)[index0]) * yt * zt);
    }
    if(index7 >= 0 && index7 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      linEquivalent += ((lin->getPointer(0)[index7] - lin->getPointer(0)[index6] - lin->getPointer(0)[index5] - lin->getPointer(0)[index3] + lin->getPointer(0)[index1] + lin->getPointer(0)[index4] +
                         lin->getPointer(0)[index2] - lin->getPointer(0)[index0]) *
                        xt * yt * zt);
    }
  }

  template <class T>
  void linearEquivalentRGB(T linEquivalent[3], IDataArray& linIData, int64_t linIntIndexes, double xt, double yt, double zt)
  {
    DataArray<T>& lin = std::dynamic_pointer_cast<DataArray<T>>(linIData);
    int index0 = linIntIndexes;
    int index1 = linIntIndexes + 1;
    int index2 = linIntIndexes + p_Impl->m_Params.xp;
    int index3 = linIntIndexes + 1 + p_Impl->m_Params.xp;
    int index4 = linIntIndexes + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    int index5 = linIntIndexes + 1 + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    int index6 = linIntIndexes + p_Impl->m_Params.xp + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    int index7 = linIntIndexes + 1 + p_Impl->m_Params.xp + p_Impl->m_Params.xp * p_Impl->m_Params.yp;
    if(index0 >= 0 && index0 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += lin->getComponent(index0, i);
      }
    }
    if(index1 >= 0 && index1 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index1, i) - lin->getComponent(index0, i)) * xt);
      }
    }
    if(index2 >= 0 && index2 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index2, i) - lin->getComponent(index0, i)) * yt);
      }
    }
    if(index3 >= 0 && index3 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index3, i) - lin->getComponent(index2, i) - lin->getComponent(index1, i) + lin->getComponent(index0, i)) * xt * yt);
      }
    }
    if(index4 >= 0 && index4 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index4, i) - lin->getComponent(index0, i)) * zt);
      }
    }
    if(index5 >= 0 && index5 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index5, i) - lin->getComponent(index4, i) - lin->getComponent(index1, i) + lin->getComponent(index0, i)) * xt * zt);
      }
    }
    if(index6 >= 0 && index6 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index6, i) - lin->getComponent(index4, i) - lin->getComponent(index2, i) + lin->getComponent(index0, i)) * yt * zt);
      }
    }
    if(index7 >= 0 && index7 <= p_Impl->m_Params.xp * p_Impl->m_Params.yp * p_Impl->m_Params.zp)
    {
      for(int i = 0; i < 3; i++)
      {
        linEquivalent[i] += ((lin->getComponent(index7, i) - lin->getComponent(index6, i) - lin->getComponent(index5, i) - lin->getComponent(index3, i) + lin->getComponent(index1, i) +
                              lin->getComponent(index4, i) + lin->getComponent(index2, i) - lin->getComponent(index0, i)) *
                             xt * yt * zt);
      }
    }
  }

  template <class T>
  bool linearIndexes(double* LinearInterpolationData, int64_t tupleIndex, T& linEquivalent, IDataArray& linIData)
  {
    const ApplyTransformationProgress::RotateArgs& m_Params = p_Impl->m_Params;
    bool write = false;
    double xt = LinearInterpolationData[tupleIndex];
    double yt = LinearInterpolationData[tupleIndex + 1];
    double zt = LinearInterpolationData[tupleIndex + 2];
    double colOld = LinearInterpolationData[tupleIndex + 3];
    double rowOld = LinearInterpolationData[tupleIndex + 4];
    double planeOld = LinearInterpolationData[tupleIndex + 5];

    if(colOld >= 0 && colOld < m_Params.xp && colOld >= 0 && colOld < m_Params.xp && rowOld >= 0 && rowOld < m_Params.yp && planeOld >= 0 && planeOld < m_Params.zp)
    {
      int planeFloor = std::floor(planeOld);
      int rowFloor = std::floor(rowOld);
      int colFloor = std::floor(colOld);

      int64_t linIntIndexes = std::nearbyint((m_Params.xp * m_Params.yp * planeFloor) + (m_Params.xp * rowFloor) + colFloor);
      linearEquivalent<T>(linEquivalent, linIData, linIntIndexes, xt, yt, zt);
      write = true;
    }
    return write;
  }

  template <class T>
  bool linearIndexesRGB(double* LinearInterpolationData, int64_t tupleIndex, T linEquivalent[3], IDataArray& linIData)
  {
    const ApplyTransformationProgress::RotateArgs& m_Params = p_Impl->m_Params;
    bool write = false;

    double xt = LinearInterpolationData[tupleIndex];
    double yt = LinearInterpolationData[tupleIndex + 1];
    double zt = LinearInterpolationData[tupleIndex + 2];
    double colOld = LinearInterpolationData[tupleIndex + 3];
    double rowOld = LinearInterpolationData[tupleIndex + 4];
    double planeOld = LinearInterpolationData[tupleIndex + 5];

    if(colOld >= 0 && colOld < m_Params.xp && colOld >= 0 && colOld < m_Params.xp && rowOld >= 0 && rowOld < m_Params.yp && planeOld >= 0 && planeOld < m_Params.zp)
    {
      int planeFloor = std::floor(planeOld);
      int rowFloor = std::floor(rowOld);
      int colFloor = std::floor(colOld);

      int64_t linIntIndexes = std::nearbyint((m_Params.xp * m_Params.yp * planeFloor) + (m_Params.xp * rowFloor) + colFloor);
      linearEquivalentRGB<T>(linEquivalent, linIData, linIntIndexes, xt, yt, zt);
      write = true;
    }
    return write;
  }

  template <typename T>
  void wrapLinearIndexes(double* LinearInterpolationData, int64_t tupleIndex, typename DataArray<T>& lin, typename IDataArray& linData)
  {
    bool wrote = false;
    int index = tupleIndex / 6;

    T linEquivalent = 0;
    typename IDataArray::Pointer linIData = std::dynamic_pointer_cast<IDataArray>(lin);
    wrote = linearIndexes<T>(LinearInterpolationData, tupleIndex, linEquivalent, linIData);
    if(wrote)
    {
      linData->initializeTuple(index, &linEquivalent);
    }
    else
    {
      int var = 0;
      linData->initializeTuple(index, &var);
    }
  }

  template <typename T>
  void wrapLinearIndexesRGB(double* LinearInterpolationData, int64_t tupleIndex, typename DataArray<T>& lin, typename IDataArray& linData)
  {
    bool wrote = false;
    int index = tupleIndex / 6;
    T linEquivalent[3] = {0, 0, 0};
    typename IDataArray::Pointer linIData = std::dynamic_pointer_cast<IDataArray>(lin);
    wrote = linearIndexesRGB<T>(LinearInterpolationData, tupleIndex, linEquivalent, linIData);
    if(wrote)
    {
      linData->initializeTuple(index, &linEquivalent);
    }
    else
    {
      int var = 0;
      linData->initializeTuple(index, &var);
    }
  }

  template <typename T>
  bool applyLinearInterpolation(typename DataArray<T>& lin, int64_t index, int64_t tupleIndex, double* LinearInterpolationData, typename IDataArray& linData, bool RGB)
  {
    const ApplyTransformationProgress::RotateArgs& m_Params = p_Impl->m_Params;
    if(!lin)
    {
      return false;
    }

    if(RGB)
    {
      wrapLinearIndexesRGB<T>(LinearInterpolationData, tupleIndex, lin, linData);
    }
    else
    {
      wrapLinearIndexes<T>(LinearInterpolationData, tupleIndex, lin, linData);
    }
    return true;
  }


  /**
   * @brief Allows thread safe progress updates
   * @param counter
   */
  //void sendThreadSafeProgressMessage(size_t counter);

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  //mutable std::mutex m_ProgressMessage_Mutex;
  //size_t m_InstanceIndex = 0;
  //size_t m_TotalElements = 0;
  //size_t m_ProgressCounter = 0;
  //size_t m_LastProgressInt = 0;
};

} // namespace complex
