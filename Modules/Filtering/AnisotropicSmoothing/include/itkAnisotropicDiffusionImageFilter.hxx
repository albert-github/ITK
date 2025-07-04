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
#ifndef itkAnisotropicDiffusionImageFilter_hxx
#define itkAnisotropicDiffusionImageFilter_hxx

#include <cmath>

namespace itk
{

template <typename TInputImage, typename TOutputImage>
AnisotropicDiffusionImageFilter<TInputImage, TOutputImage>::AnisotropicDiffusionImageFilter()
  : m_ConductanceParameter(1.0)
  , m_ConductanceScalingParameter(1.0)
  , m_ConductanceScalingUpdateInterval(1)
  , m_FixedAverageGradientMagnitude(1.0)
  , m_TimeStep(0.5 / double{ 1ULL << ImageDimension })
{
  this->SetNumberOfIterations(1);
}

template <typename TInputImage, typename TOutputImage>
void
AnisotropicDiffusionImageFilter<TInputImage, TOutputImage>::InitializeIteration()
{
  auto * f = dynamic_cast<AnisotropicDiffusionFunction<UpdateBufferType> *>(this->GetDifferenceFunction().GetPointer());
  if (!f)
  {
    throw ExceptionObject(__FILE__, __LINE__, "Anisotropic diffusion function is not set.", ITK_LOCATION);
  }

  f->SetConductanceParameter(m_ConductanceParameter);
  f->SetTimeStep(m_TimeStep);

  // Check the timestep for stability
  double minSpacing = 1.0;
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

  if (m_TimeStep > (minSpacing / double{ 1ULL << (ImageDimension + 1) }))
  {
    //    f->SetTimeStep(1.0 / double{ 1ULL << ImageDimension });
    itkWarningMacro("Anisotropic diffusion unstable time step: "
                    << m_TimeStep << std::endl
                    << "Stable time step for this image must be smaller than "
                    << minSpacing / double{ 1ULL << (ImageDimension + 1) });
  }

  if (m_GradientMagnitudeIsFixed == false)
  {
    if ((this->GetElapsedIterations() % m_ConductanceScalingUpdateInterval) == 0)
    {
      f->CalculateAverageGradientMagnitudeSquared(this->GetOutput());
    }
  }
  else
  {
    f->SetAverageGradientMagnitudeSquared(m_FixedAverageGradientMagnitude * m_FixedAverageGradientMagnitude);
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

template <typename TInputImage, typename TOutputImage>
void
AnisotropicDiffusionImageFilter<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "TimeStep: " << m_TimeStep << std::endl;
  os << indent << "ConductanceParameter: " << m_ConductanceParameter << std::endl;
  os << indent << "ConductanceScalingParameter: " << m_ConductanceScalingParameter << std::endl;
  os << indent << "ConductanceScalingUpdateInterval: " << m_ConductanceScalingUpdateInterval << std::endl;
  os << indent << "FixedAverageGradientMagnitude: " << m_FixedAverageGradientMagnitude << std::endl;
}
} // end namespace itk

#endif
