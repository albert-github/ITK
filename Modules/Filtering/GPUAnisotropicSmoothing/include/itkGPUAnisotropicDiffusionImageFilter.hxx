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
#ifndef itkGPUAnisotropicDiffusionImageFilter_hxx
#define itkGPUAnisotropicDiffusionImageFilter_hxx

#include "itkGPUAnisotropicDiffusionFunction.h"

namespace itk
{

template <typename TInputImage, typename TOutputImage, typename TParentImageFilter>
void
GPUAnisotropicDiffusionImageFilter<TInputImage, TOutputImage, TParentImageFilter>::InitializeIteration()
{
  auto * f =
    dynamic_cast<GPUAnisotropicDiffusionFunction<UpdateBufferType> *>(this->GetDifferenceFunction().GetPointer());
  if (!f)
  {
    throw ExceptionObject(__FILE__, __LINE__, "GPU anisotropic diffusion function is not set.", ITK_LOCATION);
  }

  f->SetConductanceParameter(this->GetConductanceParameter());
  f->SetTimeStep(this->GetTimeStep());

  // Check the timestep for stability
  double minSpacing;
  if (this->GetUseImageSpacing())
  {
    const auto & spacing = this->GetInput()->GetSpacing();

    minSpacing = spacing[0];
    for (unsigned int i = 1; i < ImageDimension; ++i)
    {
      if (spacing[i] < minSpacing)
      {
        minSpacing = spacing[i];
      }
    }
  }
  else
  {
    minSpacing = 1.0;
  }
  if (this->GetTimeStep() > (minSpacing / double{ 1ULL << (ImageDimension + 1) }))
  {
    //    f->SetTimeStep(1.0 / double{ 1ULL << ImageDimension });
    itkWarningMacro("Anisotropic diffusion unstable time step: "
                    << this->GetTimeStep() << std::endl
                    << "Stable time step for this image must be smaller than "
                    << minSpacing / double{ 1ULL << (ImageDimension + 1) });
  }

  if (this->m_GradientMagnitudeIsFixed == false)
  {
    if ((this->GetElapsedIterations() % this->GetConductanceScalingUpdateInterval()) == 0)
    {
      /** GPU version of average squared gradient magnitude calculation */
      f->GPUCalculateAverageGradientMagnitudeSquared(this->GetOutput());
    }
  }
  else
  {
    f->SetAverageGradientMagnitudeSquared(this->GetFixedAverageGradientMagnitude() *
                                          this->GetFixedAverageGradientMagnitude());
  }
  f->InitializeIteration();

  if (this->GetNumberOfIterations() != 0)
  {
    this->UpdateProgress((static_cast<float>(this->GetElapsedIterations())) /
                         (static_cast<float>(this->GetNumberOfIterations())));
  }
  else
  {
    this->UpdateProgress(0);
  }
}

} // end namespace itk

#endif
