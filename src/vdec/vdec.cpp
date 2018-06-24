#include <chrono>
#include <typeinfo>
#include <iostream>
#include <algorithm>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include "vdec.h"

using namespace std;

VDEC::VDEC() {
    pCurrentPacket_ = av_packet_alloc();
    pCurrentFrame_ = av_frame_alloc();
}

VDEC::~VDEC() {
    stop();
    av_packet_free(&pCurrentPacket_);
    av_frame_free(&pCurrentFrame_);
}

const char *VDEC::getConfiguration() const {
    return avcodec_configuration();
}

int VDEC::getWidth() const {
    return videoWidth_;
}

int VDEC::getHeight() const {
    return videoHeight_;
}

double VDEC::getTotalTime() const {
    return videoTime_ / 1000.0;
}


bool VDEC::setCallback(
    decltype(cbVideo) videoCallback,
    decltype(cbTimer) timeCallback) {
    lock_guard<mutex> lock(m_);
    if (!pVideoStream_)
        return false;
    cbVideo = videoCallback;
    cbTimer = timeCallback;
    return true;
}

bool VDEC::setVideoSpeed(double speed) {
    lock_guard<mutex> lock(m_);
    if (speed < 0.1 || speed > 3.0 || !pVideoStream_)
        return false;
    double ms = pVideoStream_->avg_frame_rate.den == 0 ?
        4e3 : 1e6 / (pVideoStream_->avg_frame_rate.num / (double)pVideoStream_->avg_frame_rate.den);
    ms /= speed;
    gap_ = chrono::microseconds((int)ms);
    return true;
}

bool VDEC::open(const char * url) {
    stop();
    lock_guard<mutex> lock(m_);

    int errCode = 0;
    do {
        // demux media input
        errCode = avformat_open_input(&pFmtCtx_, url, NULL, NULL);
        if (errCode)
            break;

        errCode = avformat_find_stream_info(pFmtCtx_, NULL);
        if (errCode)
            break;

        videoStreamIndex_ = av_find_best_stream(pFmtCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, NULL);
        if (videoStreamIndex_ < 0)
            break;
        pVideoStream_ = pFmtCtx_->streams[videoStreamIndex_];
        videoWidth_ = pVideoStream_->codecpar->width;
        videoHeight_ = pVideoStream_->codecpar->height;
        videoTimeBase_ = pVideoStream_->time_base.den == 0 || pVideoStream_->time_base.num == 0 ?
            AV_TIME_BASE : pVideoStream_->time_base.den / (double)pVideoStream_->time_base.num;
        videoTime_ = int(pVideoStream_->duration * 1000 / videoTimeBase_);

        // open decoder
        AVCodec *decoder = avcodec_find_decoder(pVideoStream_->codecpar->codec_id);
        if (!decoder)
            break;

        pDecCtx_ = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(pDecCtx_, pVideoStream_->codecpar);
        pDecCtx_->thread_count = 4;
        errCode = avcodec_open2(pDecCtx_, decoder, NULL);
        if (errCode)
            break;

        cout << pVideoStream_->avg_frame_rate.num << "/"
            << pVideoStream_->avg_frame_rate.den << " = "
            << pVideoStream_->avg_frame_rate.num / (double)pVideoStream_->avg_frame_rate.den << "\n";

        double ms = pVideoStream_->avg_frame_rate.den == 0 ?
            4e3 : 1e6 / (pVideoStream_->avg_frame_rate.num / (double)pVideoStream_->avg_frame_rate.den);
        gap_ = chrono::microseconds((int)ms);
        return true;
    } while (0);

    char errStr[2000];
    if (errCode)
        av_strerror(errCode, errStr, 2000);
    cout << typeid(*this).name() << "::" << __func__ << " failed:" << errStr << endl;
    return false;
}

bool VDEC::start() {
    lock_guard<mutex> lock(m_);
    const char *errStr = nullptr;
    if (pFmtCtx_ == NULL)
        errStr = "No media source.";
    else if (!cbVideo)
        errStr = "Callback do not set.";
    else if (status_ != Stop)
        errStr = "Now playing...";

    if (errStr) {
        cout << typeid(*this).name() << "::" << __func__ << " failed:" << errStr << endl;
        return false;
    }

    status_ = Playing;
    auto nextFrame = &VDEC::getNextFrame;
    f_ = async([this, nextFrame] {
        auto sleep_to = chrono::steady_clock::now();
        for (;;) {
            sleep_to += gap_;
            this_thread::sleep_until(sleep_to);
            if (status_ == Stop)
                break;
            else if (status_ == Pause)
                continue;

            auto frame = (this->*nextFrame)();
            if (!frame)
                status_ = Stop;
            if (cbVideo)
                cbVideo(reinterpret_cast<AVFrame *>(frame));
            if (cbTimer) {
                double time = 1000 * pCurrentFrame_->pts / videoTimeBase_;
                cbTimer((int)time);
            }
        }
    });
    return true;
}

bool VDEC::pause(bool changeToPause) {
    lock_guard<mutex> lock(m_);
    if (changeToPause && status_ == Stop)
        return false;
    status_ = changeToPause ? Pause : Playing;
    return true;
}

bool VDEC::pause() {
    m_.lock();
    if (status_ == Stop && pFmtCtx_ && pDecCtx_) {
        m_.unlock();
        seek(0);
        return start();
    } else if (status_ == Playing) {
        status_ = Pause;
    } else if (status_ == Pause) {
        status_ = Playing;
    } else {
        m_.unlock();
        return false;
    }
    m_.unlock();
    return true;
}

bool VDEC::stop() {
    lock_guard<mutex> lock(m_);
    status_ = Stop;
    pVideoStream_ = NULL;
    videoStreamIndex_ = 0;
    cbVideo = nullptr;
    cbTimer = nullptr;
    videoTime_ = 0;
    videoTimeBase_ = AV_TIME_BASE;

    if (f_.valid())
        f_.wait();
    if (pFmtCtx_)
        avformat_close_input(&pFmtCtx_);
    if (pDecCtx_)
        avcodec_free_context(&pDecCtx_);

    return true;
}

void VDEC::seek(double sec) {
    m_.lock();
    if (!pFmtCtx_ || !pDecCtx_)
        return;

    Status oldStatus = status_;
    status_ = Pause;
    m_.unlock();

    sec = max(sec, 0.0);
    sec = min(sec, videoTime_ / 1000.0);

    int pts = int(sec * videoTimeBase_);

    avformat_flush(pFmtCtx_);
    avcodec_flush_buffers(pDecCtx_);
    av_seek_frame(pFmtCtx_, videoStreamIndex_, pts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_BACKWARD);

    AVFrame *frame = nullptr;
    double frame_sec = 0;
    do {
        av_frame_free(&frame);
        frame = getNextFrame();
        if (!frame)
            break;

        frame_sec = frame->pts / videoTimeBase_;
    } while (frame_sec <= sec - 0.05);

    if (!frame) {
        status_ = Stop;
        return;
    }

    if (cbVideo)
        cbVideo(reinterpret_cast<AVFrame *>(frame));
    if (cbTimer) {
        double time = 1000 * pCurrentFrame_->pts / videoTimeBase_;
        cbTimer((int)time);
    }

    status_ = oldStatus;
}

uint8_t * VDEC::getCurrentRGB() {
    auto *frame = getCurrentFrame();
    if (!frame)
        return nullptr;

    SwsContext *pSwsCtx = sws_getContext(
        frame->width, frame->height,    // input width and height
        (AVPixelFormat)frame->format,	// input format YUV420p
        frame->width, frame->height,    // output width and height
        AV_PIX_FMT_RGB24,               // output format RGB24
        SWS_BILINEAR, 0, 0, 0
    );
    if (!pSwsCtx) {
        av_frame_free(&frame);
        return nullptr;
    }

    auto rgb = new uint8_t[frame->width * frame->height * 3];
    uint8_t *data[2] = { 0 };
    data[0] = rgb;
    int lines[2] = { 0 };
    lines[0] = frame->width * 3;
    int outputHeight = sws_scale(
        pSwsCtx,
        frame->data, frame->linesize, 0, frame->height, // input data, linesize, 0, height
        data, lines				                        // output data and size
    );
    sws_freeContext(pSwsCtx);
    if (outputHeight != frame->height) {
        av_frame_free(&frame);
        delete rgb;
        return nullptr;
    }

    cout << "r" << (int)0[rgb]
        << " g" << (int)1[rgb]
        << " b" << (int)2[rgb] << "\n";
    av_frame_free(&frame);
    return rgb;
}

AVFrame * VDEC::getCurrentFrame() {
    lock_guard<mutex> lock(m_);
    if (!pCurrentFrame_->height || !pCurrentFrame_->width)
        return nullptr;
    AVFrame *frame = av_frame_alloc();
    av_frame_ref(frame, pCurrentFrame_);
    return frame;
}

AVFrame *VDEC::getNextFrame() {
    lock_guard<mutex> lock(m_);

    // get next video packet (return nullptr at EOF)
    AVPacket *packet = av_packet_alloc();
    auto getNextPacket = [this, packet]() {
        int errCode = 0;
        do {
            av_packet_unref(packet); // release if is not video packet

            errCode = av_read_frame(pFmtCtx_, packet);
            if (errCode)
                break;
        } while (packet->stream_index != videoStreamIndex_);
        return errCode;
    };

    AVFrame *frame = av_frame_alloc();
    int errCode = 0;
    char errStr[1024] = { 0 };
    for (;;) {
        errCode = getNextPacket();
        if (errCode) {
            cout << "Send packet failed: ";
            av_strerror(errCode, errStr, sizeof(errStr));
            cout << errStr << endl;
            // break; // continue if EOF
        }

        errCode = avcodec_send_packet(pDecCtx_, packet);
        av_packet_unref(packet);
        if (errCode) {
            cout << "Send packet failed: ";
            av_strerror(errCode, errStr, sizeof(errStr));
            cout << errStr << endl;
            break;
        }

        // try to read frame from decoder
        errCode = avcodec_receive_frame(pDecCtx_, frame);
        if (errCode) {
            cout << "Receive frame failed: ";
            av_strerror(errCode, errStr, sizeof(errStr));
            cout << errStr << endl;
        } else {
            // read success
            break;
        }
    }

    av_packet_free(&packet);
    if (errCode) {
        av_frame_free(&frame);
        return nullptr;
    }

    // update pCurrentFrame_ reference
    av_frame_unref(pCurrentFrame_);
    av_frame_ref(pCurrentFrame_, frame);
    return frame;
}

VDEC::Status VDEC::getStatus() {
    return status_;
}

int64_t VDEC::getCurrentPts() {
    lock_guard<mutex> lock(m_);
    if (!pFmtCtx_ || !pDecCtx_)
        return 0;
    return pCurrentFrame_->pts;
}

void VDEC::freePacket(AVPacket **pkt) {
    if (!pkt || !(*pkt))return;
    av_packet_free(pkt);
}

void VDEC::freeFrame(AVFrame **frame) {
    if (!frame || !(*frame))return;
    av_frame_free(frame);
}
