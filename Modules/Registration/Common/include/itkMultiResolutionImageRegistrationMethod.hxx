/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkMultiResolutionImageRegistrationMethod_hxx
#define itkMultiResolutionImageRegistrationMethod_hxx

#include "itkRecursiveMultiResolutionPyramidImageFilter.h"
#include "itkPrintHelper.h"

namespace itk
{

template <typename TFixedImage, typename TMovingImage>
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::MultiResolutionImageRegistrationMethod()
  : m_Metric(nullptr)
  , m_Optimizer(nullptr)
  , m_MovingImage(nullptr)
  , m_FixedImage(nullptr)
  , m_Transform(nullptr)
  , m_Interpolator(nullptr)
  , m_MovingImagePyramid(MovingImagePyramidType::New())
  , m_FixedImagePyramid(FixedImagePyramidType::New())
  , m_InitialTransformParameters(ParametersType(1))
  , m_InitialTransformParametersOfNextLevel(ParametersType(1))
  , m_LastTransformParameters(ParametersType(1))
  , m_NumberOfLevels(1)

{
  this->SetNumberOfRequiredOutputs(1); // for the Transform

  // Use MultiResolutionPyramidImageFilter as the default
  // image pyramids.
  m_InitialTransformParameters.Fill(0.0f);
  m_InitialTransformParametersOfNextLevel.Fill(0.0f);
  m_LastTransformParameters.Fill(0.0f);

  const TransformOutputPointer transformDecorator =
    itkDynamicCastInDebugMode<TransformOutputType *>(this->MakeOutput(0).GetPointer());

  this->ProcessObject::SetNthOutput(0, transformDecorator.GetPointer());
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::Initialize()
{
  // Sanity checks
  if (!m_Metric)
  {
    itkExceptionMacro("Metric is not present");
  }

  if (!m_Optimizer)
  {
    itkExceptionMacro("Optimizer is not present");
  }

  if (!m_Transform)
  {
    itkExceptionMacro("Transform is not present");
  }

  if (!m_Interpolator)
  {
    itkExceptionMacro("Interpolator is not present");
  }

  // Setup the metric
  m_Metric->SetMovingImage(m_MovingImagePyramid->GetOutput(m_CurrentLevel));
  m_Metric->SetFixedImage(m_FixedImagePyramid->GetOutput(m_CurrentLevel));
  m_Metric->SetTransform(m_Transform);
  m_Metric->SetInterpolator(m_Interpolator);
  m_Metric->SetFixedImageRegion(m_FixedImageRegionPyramid[m_CurrentLevel]);
  m_Metric->Initialize();

  // Setup the optimizer
  m_Optimizer->SetCostFunction(m_Metric);
  m_Optimizer->SetInitialPosition(m_InitialTransformParametersOfNextLevel);

  //
  // Connect the transform to the Decorator.
  //
  auto * transformOutput = static_cast<TransformOutputType *>(this->ProcessObject::GetOutput(0));

  transformOutput->Set(m_Transform);
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::StopRegistration()
{
  m_Stop = true;
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::SetSchedules(
  const ScheduleType & fixedImagePyramidSchedule,
  const ScheduleType & movingImagePyramidSchedule)
{
  if (m_NumberOfLevelsSpecified)
  {
    itkExceptionMacro("SetSchedules should not be used "
                      << "if numberOfLevelves are specified using SetNumberOfLevels");
  }
  m_FixedImagePyramidSchedule = fixedImagePyramidSchedule;
  m_MovingImagePyramidSchedule = movingImagePyramidSchedule;
  m_ScheduleSpecified = true;

  // Set the number of levels based on the pyramid schedule specified
  if (m_FixedImagePyramidSchedule.rows() != m_MovingImagePyramidSchedule.rows())
  {
    itkExceptionMacro("The specified schedules contain unequal number of levels");
  }
  else
  {
    m_NumberOfLevels = m_FixedImagePyramidSchedule.rows();
  }

  this->Modified();
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::SetNumberOfLevels(SizeValueType numberOfLevels)
{
  if (m_ScheduleSpecified)
  {
    itkExceptionMacro("SetNumberOfLevels should not be used "
                      << "if schedules have been specified using SetSchedules method ");
  }

  m_NumberOfLevels = numberOfLevels;
  m_NumberOfLevelsSpecified = true;
  this->Modified();
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::PreparePyramids()
{
  if (!m_Transform)
  {
    itkExceptionMacro("Transform is not present");
  }

  m_InitialTransformParametersOfNextLevel = m_InitialTransformParameters;

  if (m_InitialTransformParametersOfNextLevel.Size() != m_Transform->GetNumberOfParameters())
  {
    itkExceptionMacro("Size mismatch between initial parameter and transform");
  }

  // Sanity checks
  if (!m_FixedImage)
  {
    itkExceptionMacro("FixedImage is not present");
  }

  if (!m_MovingImage)
  {
    itkExceptionMacro("MovingImage is not present");
  }

  if (!m_FixedImagePyramid)
  {
    itkExceptionMacro("Fixed image pyramid is not present");
  }

  if (!m_MovingImagePyramid)
  {
    itkExceptionMacro("Moving image pyramid is not present");
  }

  // Setup the fixed and moving image pyramid
  if (m_NumberOfLevelsSpecified)
  {
    m_FixedImagePyramid->SetNumberOfLevels(m_NumberOfLevels);
    m_MovingImagePyramid->SetNumberOfLevels(m_NumberOfLevels);
  }

  if (m_ScheduleSpecified)
  {
    m_FixedImagePyramid->SetNumberOfLevels(m_FixedImagePyramidSchedule.rows());
    m_FixedImagePyramid->SetSchedule(m_FixedImagePyramidSchedule);

    m_MovingImagePyramid->SetNumberOfLevels(m_MovingImagePyramidSchedule.rows());
    m_MovingImagePyramid->SetSchedule(m_MovingImagePyramidSchedule);
  }

  m_FixedImagePyramid->SetInput(m_FixedImage);
  m_FixedImagePyramid->UpdateLargestPossibleRegion();

  // Setup the moving image pyramid
  m_MovingImagePyramid->SetInput(m_MovingImage);
  m_MovingImagePyramid->UpdateLargestPossibleRegion();

  using SizeType = typename FixedImageRegionType::SizeType;
  using IndexType = typename FixedImageRegionType::IndexType;

  ScheduleType schedule = m_FixedImagePyramid->GetSchedule();
  itkDebugMacro("FixedImage schedule: " << schedule);

  const ScheduleType movingschedule = m_MovingImagePyramid->GetSchedule();
  itkDebugMacro("MovingImage schedule: " << movingschedule);

  SizeType  inputSize = m_FixedImageRegion.GetSize();
  IndexType inputStart = m_FixedImageRegion.GetIndex();

  const SizeValueType numberOfLevels = m_FixedImagePyramid->GetNumberOfLevels();

  m_FixedImageRegionPyramid.reserve(numberOfLevels);
  m_FixedImageRegionPyramid.resize(numberOfLevels);

  // Compute the FixedImageRegion corresponding to each level of the
  // pyramid. This uses the same algorithm of the ShrinkImageFilter
  // since the regions should be compatible.
  for (unsigned int level = 0; level < numberOfLevels; ++level)
  {
    SizeType  size;
    IndexType start;
    for (unsigned int dim = 0; dim < TFixedImage::ImageDimension; ++dim)
    {
      const auto scaleFactor = static_cast<float>(schedule[level][dim]);

      size[dim] =
        static_cast<typename SizeType::SizeValueType>(std::floor(static_cast<float>(inputSize[dim]) / scaleFactor));
      if (size[dim] < 1)
      {
        size[dim] = 1;
      }

      start[dim] =
        static_cast<typename IndexType::IndexValueType>(std::ceil(static_cast<float>(inputStart[dim]) / scaleFactor));
    }
    m_FixedImageRegionPyramid[level].SetSize(size);
    m_FixedImageRegionPyramid[level].SetIndex(start);
  }
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  using namespace print_helper;

  Superclass::PrintSelf(os, indent);

  itkPrintSelfObjectMacro(Metric);
  itkPrintSelfObjectMacro(Optimizer);

  itkPrintSelfObjectMacro(MovingImage);
  itkPrintSelfObjectMacro(FixedImage);

  itkPrintSelfObjectMacro(Transform);
  itkPrintSelfObjectMacro(Interpolator);

  itkPrintSelfObjectMacro(MovingImagePyramid);
  itkPrintSelfObjectMacro(FixedImagePyramid);

  os << indent << "InitialTransformParameters: "
     << static_cast<typename NumericTraits<ParametersType>::PrintType>(m_InitialTransformParameters) << std::endl;
  os << indent << "InitialTransformParametersOfNextLevel: "
     << static_cast<typename NumericTraits<ParametersType>::PrintType>(m_InitialTransformParametersOfNextLevel)
     << std::endl;
  os << indent << "LastTransformParameters: "
     << static_cast<typename NumericTraits<ParametersType>::PrintType>(m_LastTransformParameters) << std::endl;

  os << indent << "FixedImageRegion: " << m_FixedImageRegion << std::endl;
  os << indent << "FixedImageRegionPyramid: " << m_FixedImageRegionPyramid << std::endl;

  os << indent << "NumberOfLevels: " << static_cast<typename NumericTraits<SizeValueType>::PrintType>(m_NumberOfLevels)
     << std::endl;
  os << indent << "CurrentLevel: " << static_cast<typename NumericTraits<SizeValueType>::PrintType>(m_CurrentLevel)
     << std::endl;

  itkPrintSelfBooleanMacro(Stop);

  os << indent << "FixedImagePyramidSchedule: "
     << static_cast<typename NumericTraits<ScheduleType>::PrintType>(m_FixedImagePyramidSchedule) << std::endl;
  os << indent << "MovingImagePyramidSchedule: "
     << static_cast<typename NumericTraits<ScheduleType>::PrintType>(m_MovingImagePyramidSchedule) << std::endl;

  itkPrintSelfBooleanMacro(ScheduleSpecified);
  itkPrintSelfBooleanMacro(Stop);
}

template <typename TFixedImage, typename TMovingImage>
void
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::GenerateData()
{
  m_Stop = false;

  this->PreparePyramids();

  for (m_CurrentLevel = 0; m_CurrentLevel < m_NumberOfLevels; ++m_CurrentLevel)
  {
    // Invoke an iteration event.
    // This allows a UI to reset any of the components between
    // resolution level.
    this->InvokeEvent(MultiResolutionIterationEvent());

    // Check if there has been a stop request
    if (m_Stop)
    {
      break;
    }

    try
    {
      // initialize the interconnects between components
      this->Initialize();
    }
    catch (const ExceptionObject &)
    {
      m_LastTransformParameters = ParametersType(1);
      m_LastTransformParameters.Fill(0.0f);

      // pass exception to caller
      throw;
    }

    try
    {
      // do the optimization
      m_Optimizer->StartOptimization();
    }
    catch (const ExceptionObject &)
    {
      // An error has occurred in the optimization.
      // Update the parameters
      m_LastTransformParameters = m_Optimizer->GetCurrentPosition();

      // Pass exception to caller
      throw;
    }

    // get the results
    m_LastTransformParameters = m_Optimizer->GetCurrentPosition();
    m_Transform->SetParameters(m_LastTransformParameters);

    // setup the initial parameters for next level
    if (m_CurrentLevel < m_NumberOfLevels - 1)
    {
      m_InitialTransformParametersOfNextLevel = m_LastTransformParameters;
    }
  }
}

template <typename TFixedImage, typename TMovingImage>
ModifiedTimeType
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::GetMTime() const
{
  ModifiedTimeType mtime = Superclass::GetMTime();

  // Some of the following should be removed once ivars are put in the
  // input and output lists

  if (m_Transform)
  {
    ModifiedTimeType m = m_Transform->GetMTime();
    mtime = (m > mtime ? m : mtime);
  }

  if (m_Interpolator)
  {
    ModifiedTimeType m = m_Interpolator->GetMTime();
    mtime = (m > mtime ? m : mtime);
  }

  if (m_Metric)
  {
    ModifiedTimeType m = m_Metric->GetMTime();
    mtime = (m > mtime ? m : mtime);
  }

  if (m_Optimizer)
  {
    ModifiedTimeType m = m_Optimizer->GetMTime();
    mtime = (m > mtime ? m : mtime);
  }

  if (m_FixedImage)
  {
    ModifiedTimeType m = m_FixedImage->GetMTime();
    mtime = (m > mtime ? m : mtime);
  }

  if (m_MovingImage)
  {
    ModifiedTimeType m = m_MovingImage->GetMTime();
    mtime = (m > mtime ? m : mtime);
  }

  return mtime;
}

template <typename TFixedImage, typename TMovingImage>
auto
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::GetOutput() const -> const TransformOutputType *
{
  return static_cast<const TransformOutputType *>(this->ProcessObject::GetOutput(0));
}

template <typename TFixedImage, typename TMovingImage>
DataObject::Pointer
MultiResolutionImageRegistrationMethod<TFixedImage, TMovingImage>::MakeOutput(DataObjectPointerArraySizeType output)
{
  if (output > 0)
  {
    itkExceptionMacro("MakeOutput request for an output number larger than the expected number of outputs.");
  }
  return TransformOutputType::New().GetPointer();
}
} // end namespace itk

#endif
