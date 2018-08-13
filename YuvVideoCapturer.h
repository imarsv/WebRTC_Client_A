#include <memory>

//
// Created by serhii on 13.08.18.
//

#include <media/base/videocapturer.h>
#include <api/video/i420_buffer.h>

#include <rtc_base/thread.h>
#include <rtc_base/asyncinvoker.h>
#include <rtc_base/bind.h>

#include <rtc_base/logging.h>
#include "YuvFrameGenerator.h"

class YuvVideoCapturer : public cricket::VideoCapturer,
                         public rtc::Thread {
public:
  YuvVideoCapturer()
      : _frameGenerator(nullptr), _frameIndex(0), _width(640), _height(480) {
    SetCaptureFormat(nullptr);

    _frameGenerator = new YuvFrameGenerator(_width, _height, true);

    std::vector<cricket::VideoFormat> formats;
    formats.emplace_back(_width, _height, cricket::VideoFormat::FpsToInterval(25), cricket::FOURCC_IYUV);
    SetSupportedFormats(formats);
  }

  virtual ~YuvVideoCapturer() {
    Stop();

    rtc::Thread::Stop();
    _asyncInvoker.reset();

    if (_frameGenerator) {
      delete _frameGenerator;
    }
  }

  void SignalFrameCapturedOnStartThread() {
    _frameGenerator->GenerateNextFrame(_frameIndex);
    _frameIndex++;

    auto buffer = webrtc::I420Buffer::Copy(_width, _height,
                                           _frameGenerator->DataY(), _width,
                                           _frameGenerator->DataU(), _height,
                                           _frameGenerator->DataV(), _height);

    OnFrame(webrtc::VideoFrame(
        buffer, webrtc::kVideoRotation_0,
        rtc::TimeNanos() / rtc::kNumNanosecsPerMicrosec),
            _width, _height);
  }

  void Run() {
    while (IsRunning()) {
      _asyncInvoker->AsyncInvoke<void>(RTC_FROM_HERE, _startThread,
                                       rtc::Bind(&YuvVideoCapturer::SignalFrameCapturedOnStartThread, this));

      ProcessMessages(40);
    }
  }

  virtual cricket::CaptureState Start(const cricket::VideoFormat &format) {
    RTC_LOG(INFO) << "YuvVideoCapturer::Start";

    _startThread = rtc::Thread::Current();
    _asyncInvoker = std::make_unique<rtc::AsyncInvoker>();

    SetCaptureFormat(&format);
    SetCaptureState(cricket::CS_RUNNING);

    rtc::Thread::Start();

    return cricket::CS_RUNNING;
  }

  virtual void Stop() {
    RTC_LOG(INFO) << "YuvVideoCapturer::Stop";

    SetCaptureFormat(nullptr);
    SetCaptureState(cricket::CS_STOPPED);

    rtc::Thread::Stop();
    _asyncInvoker.reset();
  }

  virtual bool GetPreferredFourccs(std::vector<uint32_t> *fourccs) {
    fourccs->push_back(cricket::FOURCC_IYUV);
    return true;
  }

  virtual bool IsScreencast() const {
    return false;
  }

  virtual bool IsRunning() {
    return this->capture_state() == cricket::CS_RUNNING;
  }

private:
  YuvFrameGenerator *_frameGenerator;

  int _frameIndex;
  int _width;
  int _height;

  rtc::Thread *_startThread;
  std::unique_ptr<rtc::AsyncInvoker> _asyncInvoker;
};