#include "../common.h"

using namespace de;
using namespace de::stream_webrtc;



// create webrtc::VideoCaptureModule capturer.
bool de::stream_webrtc::VideoDevCapturerComposite::Init(size_t width,
                       size_t height,
                       size_t target_fps,
                       const char * unique_name) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> video_info(webrtc::VideoCaptureFactory::CreateDeviceInfo());

  //Create video capturer that is linked to video device. e.g.: /dev/video1
  m_Capturer = webrtc::VideoCaptureFactory::Create(unique_name);
  if (!m_Capturer) {
    return false;
  }

  // link capturer to sink to receive onFrame
  // notice that this OnFrame has nothing to do with Stream->getTrack->AddorUpdateSinlk()
  // this is a local onFrame where you can change frame contents from the very first beginning.
  // actually THIS "RegisterCaptureDataCallback" is a mandatory step for frame and delevering it to TrackSource.
  m_Capturer->RegisterCaptureDataCallback(this);

  // start capturing with given parameters.
  webrtc::VideoCaptureCapability cabability;

  cabability.width = static_cast<int32_t>(width);
  cabability.height = static_cast<int32_t>(height);
  cabability.maxFPS = static_cast<int32_t>(target_fps);
  cabability.videoType = webrtc::VideoType::kI420; //TODO: check this !!!!!!

  video_info->GetBestMatchedCapability(m_Capturer->CurrentDeviceName(), cabability, m_cabability);

  std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Valid Video Device Found: "  
        << _SUCCESS_CONSOLE_TEXT_ << " cam_unique_name:" << _INFO_BOLD_CONSOLE_TEXT << unique_name 
        << _SUCCESS_CONSOLE_TEXT_ << " width:" << _INFO_BOLD_CONSOLE_TEXT << m_cabability.width 
        << _SUCCESS_CONSOLE_TEXT_ << " height:" << _INFO_BOLD_CONSOLE_TEXT << m_cabability.height 
        << _SUCCESS_CONSOLE_TEXT_ << " maxFPS:" << _INFO_BOLD_CONSOLE_TEXT << m_cabability.maxFPS 
        << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
  


  
  return true;
}

bool de::stream_webrtc::VideoDevCapturerComposite::StartCapture()
{
  if (!m_Capturer) {
    return false;
  }

  if (m_Capturer->StartCapture(m_cabability) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(m_Capturer->CaptureStarted());

  return true;
}

bool de::stream_webrtc::VideoDevCapturerComposite::StopCapture()
{
  if (!m_Capturer) {
    return false;
  }

  if (m_Capturer->StopCapture() != 0)
  {
    return false;
  }
  
  return true;
}

de::stream_webrtc::VideoDevCapturerComposite*  de::stream_webrtc::VideoDevCapturerComposite::Create(size_t width,
                                 size_t height,
                                 size_t target_fps,
                                 const char * unique_name) {
  // create instance.                                   
  VideoDevCapturerComposite* capturer = new VideoDevCapturerComposite();

  
  if (!capturer->Init(width, height, target_fps, unique_name)) {
    std::cout << "Failed to create capturer(w = " << width
                        << ", h = " << height << ", fps = " << target_fps
                        << ")" << std::endl;
    return nullptr;
  }
  
  return capturer;
}

void de::stream_webrtc::VideoDevCapturerComposite::Destroy() {

   std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;

  if (!m_Capturer)
    return;

  m_Capturer->StopCapture();

  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << "\033[1;31m" << "STOP CAPTURER" << _NORMAL_CONSOLE_TEXT_ << std::endl;

  m_Capturer->DeRegisterCaptureDataCallback();
  
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << "\033[1;31m" << "STOP CAPTURER" << _NORMAL_CONSOLE_TEXT_ << std::endl;

// Release reference to VCM.
  m_Capturer = nullptr;

  
}

de::stream_webrtc::VideoDevCapturerComposite::~VideoDevCapturerComposite() {
  
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
  Destroy();
}                       


void de::stream_webrtc::VideoDevCapturerComposite::OnFrame(const webrtc::VideoFrame& original_frame)
{
  webrtc::VideoFrame frame = MaybePreprocess(original_frame);
 
  if (m_is_video_recording)
  {
    // save video if needed
    printVideoFrame (frame);
  }
  
  // save image if needed
  saveFrameAsPNG(frame);  
  saveFrameAsRGB(frame);
  
  
  if (!m_once)
  {
    std::cout <<"VideoDevCapturerComposite::onFrame" << std::endl;
    m_once = true;
  }

  int cropped_width = 0;
  int cropped_height = 0;
  int out_width = 640;
  int out_height = 480;

  
  if ((out_width>=frame.width()) || (out_height>=frame.height()))
  {
    m_broadCaster.OnFrame(frame);
    return ;
  }

  if (!m_videoAdapter.AdaptFrameResolution(
          frame.width(), frame.height(), frame.timestamp_us() * 1000,
          &cropped_width, &cropped_height, &out_width, &out_height)) 
  {
    #ifdef DEBUG
      // Drop frame in order to respect frame rate constraint.
      std::cout << "Drop frame in order to respect frame rate constraint." << std::endl;
    #endif
    return;
  }
  
  if (out_height != frame.height() || out_width != frame.width()) {
    // Video adapter has requested a down-scale. Allocate a new buffer and
    // return scaled version.
    rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
        webrtc::I420Buffer::Create(out_width, out_height);
    scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());

    webrtc::VideoFrame::Builder new_frame_builder = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(scaled_buffer)
            .set_rotation(webrtc::kVideoRotation_0)
            .set_timestamp_us(frame.timestamp_us())
            .set_id(frame.id());
    
    webrtc::VideoFrame adapted_frame = new_frame_builder.build();

    if (frame.has_update_rect()) {
      webrtc::VideoFrame::UpdateRect new_rect = frame.update_rect().ScaleWithFrame(
          frame.width(), frame.height(), 0, 0, frame.width(), frame.height(),
          out_width, out_height);
      adapted_frame.set_update_rect(new_rect);
    }
    
    m_broadCaster.OnFrame(adapted_frame);
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Video adapter has requested a down-scale. Allocate a new buffer and return scaled version." << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        

  } else {
    m_broadCaster.OnFrame(frame);
  }
  
}


rtc::VideoSinkWants de::stream_webrtc::VideoDevCapturerComposite::GetSinkWants() {
  return m_broadCaster.wants();
}

void de::stream_webrtc::VideoDevCapturerComposite::AddOrUpdateSink( rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                        const rtc::VideoSinkWants& wants) 
{
  #ifdef DEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "\r\n" << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: VideoDevCapturerComposite::AddOrUpdateSink" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  m_broadCaster.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
    
}

void de::stream_webrtc::VideoDevCapturerComposite::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  
  #ifdef DEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "\r\n" << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: VideoDevCapturerComposite::RemoveSink" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  m_broadCaster.RemoveSink(sink);
  UpdateVideoAdapter();

}

void de::stream_webrtc::VideoDevCapturerComposite::UpdateVideoAdapter() {

}

webrtc::VideoFrame de::stream_webrtc::VideoDevCapturerComposite::MaybePreprocess(const webrtc::VideoFrame& frame) {
  webrtc::MutexLock lock(&lock_);
  
  return frame;
}




