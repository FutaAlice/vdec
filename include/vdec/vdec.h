#pragma once
#include <future>
#include <functional>
#include <atomic>
#include <mutex>

#pragma warning(disable: 4251)

#ifdef VDEC_EXPORTS
#define VDEC_API __declspec(dllexport)
#else
#define VDEC_API
#endif

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVStream;

class VDEC_API VDEC {
public:
    enum Status {
        Playing, Pause, Stop,
    };

private:
    std::function<void(AVFrame *)> cbVideo = nullptr;
    std::function<void(int)> cbTimer = nullptr;

public:
    VDEC();
    ~VDEC();

    const char *getConfiguration() const;
    int getWidth() const;
    int getHeight() const;
    double getTotalTime() const;
    Status getStatus();
    int64_t getCurrentPts();

    bool setCallback(decltype(cbVideo), decltype(cbTimer));
    bool setVideoSpeed(double); // 0.1f ~ 3.0f

    bool open(const char *url);
    bool start();
    bool pause(bool);
    bool pause();
    bool stop();
    void seek(double sec);

    uint8_t *getCurrentRGB();
    AVFrame *getCurrentFrame();

    static void freePacket(AVPacket **);
    static void freeFrame(AVFrame **);
    
private:
    AVFrame *getNextFrame();

private:
    AVFormatContext *pFmtCtx_ = NULL;
    AVCodecContext *pDecCtx_ = NULL;
    AVStream *pVideoStream_ = NULL;
    AVPacket *pCurrentPacket_ = NULL;
    AVFrame *pCurrentFrame_ = NULL;

    int videoStreamIndex_ = 0;
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    int videoTime_ = 0;
    double videoTimeBase_ = 1e6;

    std::future<void> f_;
    std::atomic<Status> status_ = Stop;
    std::chrono::microseconds gap_;
    std::mutex m_;
};
