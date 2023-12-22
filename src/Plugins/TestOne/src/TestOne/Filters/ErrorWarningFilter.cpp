/* ============================================================================
 * Copyright (c) 2009-2016 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ErrorWarningFilter.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"

using namespace nx::core;

//------------------------------------------------------------------------------
std::string ErrorWarningFilter::name() const
{
  return FilterTraits<ErrorWarningFilter>::name;
}

//------------------------------------------------------------------------------
std::string ErrorWarningFilter::className() const
{
  return FilterTraits<ErrorWarningFilter>::className;
}

//------------------------------------------------------------------------------
nx::core::Uuid ErrorWarningFilter::uuid() const
{
  return FilterTraits<ErrorWarningFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErrorWarningFilter::humanName() const
{
  return "Error Warning and Test Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErrorWarningFilter::defaultTags() const
{
  return {className(), "Example", "Test", "Error"};
}

//------------------------------------------------------------------------------
nx::core::Parameters ErrorWarningFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<BoolParameter>(k_PreflightWarning_Key, "Preflight Warning", "Preflight warning parameter", false));
  params.insert(std::make_unique<BoolParameter>(k_PreflightError_Key, "Preflight Error", "Preflight error parameter", false));
  params.insert(std::make_unique<BoolParameter>(k_PreflightException_Key, "Preflight Exception", "Preflight exception parameter", false));
  params.insert(std::make_unique<BoolParameter>(k_ExecuteWarning_Key, "Execute Warning", "Execute warning parameter", false));
  params.insert(std::make_unique<BoolParameter>(k_ExecuteError_Key, "Execute Error", "Execute error parameter", false));
  params.insert(std::make_unique<BoolParameter>(k_ExecuteException_Key, "Execute Exception", "Execute exception parameter", false));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErrorWarningFilter::clone() const
{
  return std::make_unique<ErrorWarningFilter>();
}

//------------------------------------------------------------------------------
nx::core::IFilter::PreflightResult ErrorWarningFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto preflightWarning = args.value<bool>(k_PreflightWarning_Key);
  auto preflightError = args.value<bool>(k_PreflightError_Key);
  auto preflightException = args.value<bool>(k_PreflightException_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  if(preflightWarning)
  {
    resultOutputActions.warnings().push_back(Warning{-666000, "Intentional preflight warning generated"});
  }
  if(preflightError)
  {
    return {MakeErrorResult<OutputActions>(-666001, "Intentional preflight error generated")};
  }
  if(preflightException)
  {
    throw std::runtime_error("Intentional preflight runtime excption generated");
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
nx::core::Result<> ErrorWarningFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto executeWarning = args.value<bool>(k_ExecuteWarning_Key);
  auto executeError = args.value<bool>(k_ExecuteError_Key);
  auto executeException = args.value<bool>(k_ExecuteException_Key);

  nx::core::Result<> resultActions;

  if(executeWarning)
  {
    resultActions.warnings().push_back(Warning{-666000, "Intentional execute warning generated"});
  }
  if(executeError)
  {
    return {MakeErrorResult(-666001, "Intentional execute error generated")};
  }
  if(executeException)
  {
    throw std::runtime_error("Intentional execute runtime excption generated");
  }

  return std::move(resultActions);
}
