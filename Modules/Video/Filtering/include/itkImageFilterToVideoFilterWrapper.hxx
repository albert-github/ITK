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
#ifndef itkImageFilterToVideoFilterWrapper_hxx
#define itkImageFilterToVideoFilterWrapper_hxx


namespace itk
{

template <typename TImageToImageFilter>
ImageFilterToVideoFilterWrapper<TImageToImageFilter>::ImageFilterToVideoFilterWrapper()
{
  // Always a 1-to-1 filter
  this->TemporalProcessObject::m_UnitInputNumberOfFrames = 1;
  this->TemporalProcessObject::m_UnitOutputNumberOfFrames = 1;
  this->TemporalProcessObject::m_InputStencilCurrentFrameIndex = 0;
  this->TemporalProcessObject::m_FrameSkipPerOutput = 1;

  // Initialize image filter to nullptr
  m_ImageFilter = nullptr;
}

template <typename TImageToImageFilter>
void
ImageFilterToVideoFilterWrapper<TImageToImageFilter>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  itkPrintSelfObjectMacro(ImageFilter);
}

template <typename TImageToImageFilter>
void
ImageFilterToVideoFilterWrapper<TImageToImageFilter>::TemporalStreamingGenerateData()
{
  // Make sure ImageFilter is not null
  if (m_ImageFilter.IsNull())
  {
    itkExceptionMacro("ImageFilter has not been set");
  }

  // Get the input and output video streams
  const InputVideoStreamType * input = this->GetInput();
  OutputVideoStreamType *      output = this->GetOutput();

  // Get input and output frame numbers
  const typename OutputVideoStreamType::TemporalRegionType outReqTempRegion = output->GetRequestedTemporalRegion();
  const SizeValueType                                      outFrameNum = outReqTempRegion.GetFrameStart();

  const typename InputVideoStreamType::TemporalRegionType inReqTempRegion = input->GetRequestedTemporalRegion();
  const SizeValueType                                     inFrameNum = inReqTempRegion.GetFrameStart();

  // Set up the internal image pipeline
  m_ImageFilter->SetInput(input->GetFrame(inFrameNum));

  // Update the filter
  m_ImageFilter->Update();

  // Set the output frame
  output->SetFrame(outFrameNum, m_ImageFilter->GetOutput());

  // Make a new output for the filter so this output doesn't get destroyed
  m_ImageFilter->GetOutput()->DisconnectPipeline();
  m_ImageFilter->MakeOutput(0);
}


} // end namespace itk

#endif
