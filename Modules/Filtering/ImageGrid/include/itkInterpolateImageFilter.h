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
#ifndef itkInterpolateImageFilter_h
#define itkInterpolateImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"

namespace itk
{
/**
 * \class InterpolateImageFilter
 * \brief Interpolate an image from two N-D images.
 *
 * Interpolates an image from two input images of the same type
 * and same dimension (N). In particular, this filter forms an intermediate
 * (N+1)D image by concatenating the two input images and interpolating
 * an image a distance \f$ d \in [0,1] \f$ away from the first image.
 *
 * The interpolation is delegated to a user specified InterpolateImageFunction.
 * By default, linear interpolation is used.
 *
 * The filter is templated over the input image type and output image type.
 * It assumes that the input and output have the same number of dimensions.
 *
 * \ingroup MultiThreaded
 * \ingroup ITKImageGrid
 */
template <typename TInputImage, typename TOutputImage>
class ITK_TEMPLATE_EXPORT InterpolateImageFilter : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(InterpolateImageFilter);

  /** Standard class type aliases. */
  using Self = InterpolateImageFilter;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(InterpolateImageFilter);

  /** Inherit type alias from Superclass */
  using typename Superclass::InputImageType;
  using typename Superclass::InputImagePointer;
  using typename Superclass::OutputImageType;
  using typename Superclass::OutputImagePointer;
  using typename Superclass::OutputImageRegionType;

  /** Number of dimensions. */
  static constexpr unsigned int ImageDimension = TOutputImage::ImageDimension;
  static constexpr unsigned int IntermediateImageDimension = TOutputImage::ImageDimension + 1;

  /** Interpolator type alias. */
  using InputPixelType = typename TInputImage::PixelType;
  using IntermediateImageType = Image<InputPixelType, Self::IntermediateImageDimension>;
  using InterpolatorType = InterpolateImageFunction<IntermediateImageType>;

  /** Set/Get the first image */
  /** @ITKStartGrouping */
  void
  SetInput1(const InputImageType * image)
  {
    this->SetInput(image);
  }
  const InputImageType *
  GetInput1()
  {
    return this->GetInput();
  }
  /** @ITKEndGrouping */
  /** Set/Get the second image */
  void
  SetInput2(const InputImageType * image);

  const InputImageType *
  GetInput2();

  /** Set/Get the distance from the first image from which to generate
   * interpolated image. The default value is 0.5 */
  /** @ITKStartGrouping */
  itkSetClampMacro(Distance, double, 0.0, 1.0);
  itkGetConstMacro(Distance, double);
  /** @ITKEndGrouping */
  /** Get/Set the interpolator function */
  /** @ITKStartGrouping */
  itkSetObjectMacro(Interpolator, InterpolatorType);
  itkGetModifiableObjectMacro(Interpolator, InterpolatorType);
  /** @ITKEndGrouping */
  /**
   * Set up the state of the filter before multi-threading.
   * InterpolatorType::SetInputImage is not thread-safe and hence
   * has to be setup before ThreadedGenerateData.
   */
  void
  BeforeThreadedGenerateData() override;

  /** This method is used to run after multi-threading. */
  void
  AfterThreadedGenerateData() override;

  itkConceptMacro(InputHasNumericTraitsCheck, (Concept::HasNumericTraits<InputPixelType>));

protected:
  InterpolateImageFilter();
  ~InterpolateImageFilter() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  /** InterpolateImageFilter can be implemented as a multithreaded filter. */
  void
  DynamicThreadedGenerateData(const OutputImageRegionType & outputRegionForThread) override;


private:
  typename InterpolatorType::Pointer m_Interpolator{};

  typename IntermediateImageType::Pointer m_IntermediateImage{};

  double m_Distance{};
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkInterpolateImageFilter.hxx"
#endif

#endif
