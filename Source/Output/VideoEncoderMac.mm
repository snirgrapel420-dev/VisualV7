// ============================================================================
//  VideoEncoder — macOS: AVAssetWriter → MP4 (H.264 via VideoToolbox + AAC).
//  Compiled with ARC (see CMakeLists.txt).
// ============================================================================
#include "VideoEncoder.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#include <unistd.h>
#include <cstring>

#if ! __has_feature(objc_arc)
 #error "VideoEncoderMac.mm must be compiled with -fobjc-arc"
#endif

namespace dali
{
namespace
{
std::string describe(NSError* e, const char* fallback)
{
    if (e != nil && e.localizedDescription != nil) return std::string(e.localizedDescription.UTF8String);
    return fallback;
}

class AVFoundationEncoder final : public VideoEncoder
{
public:
    ~AVFoundationEncoder() override
    {
        if (audioFormat != nullptr) CFRelease(audioFormat);
    }

    bool open(const char* utf8Path, const Settings& s, std::string& error) override
    {
        @autoreleasepool
        {
            settings = s;
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:utf8Path]];
            [[NSFileManager defaultManager] removeItemAtURL:url error:nil];

            NSError* err = nil;
            writer = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&err];
            if (writer == nil) { error = describe(err, "Could not create the video file"); return false; }

            NSDictionary* compression = @{ AVVideoAverageBitRateKey: @(s.videoBitrate),
                                           AVVideoMaxKeyFrameIntervalKey: @(s.fps * 2),
                                           AVVideoExpectedSourceFrameRateKey: @(s.fps),
                                           AVVideoProfileLevelKey: AVVideoProfileLevelH264HighAutoLevel };
            NSDictionary* videoSettings = @{ AVVideoCodecKey: AVVideoCodecTypeH264,
                                             AVVideoWidthKey: @(s.width),
                                             AVVideoHeightKey: @(s.height),
                                             AVVideoCompressionPropertiesKey: compression };
            videoInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
            videoInput.expectsMediaDataInRealTime = YES;

            NSDictionary* pixelAttributes = @{ (id) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
                                               (id) kCVPixelBufferWidthKey: @(s.width),
                                               (id) kCVPixelBufferHeightKey: @(s.height) };
            adaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:videoInput
                                                                                       sourcePixelBufferAttributes:pixelAttributes];

            AudioChannelLayout layout;
            std::memset(&layout, 0, sizeof(layout));
            layout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
            NSDictionary* audioSettings = @{ AVFormatIDKey: @(kAudioFormatMPEG4AAC),
                                             AVSampleRateKey: @(s.sampleRate),
                                             AVNumberOfChannelsKey: @2,
                                             AVEncoderBitRateKey: @(s.audioBitrate),
                                             AVChannelLayoutKey: [NSData dataWithBytes:&layout length:sizeof(layout)] };
            audioInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:audioSettings];
            audioInput.expectsMediaDataInRealTime = YES;

            if (![writer canAddInput:videoInput] || ![writer canAddInput:audioInput])
            {
                error = "The video settings are not supported on this Mac";
                return false;
            }
            [writer addInput:videoInput];
            [writer addInput:audioInput];

            AudioStreamBasicDescription asbd;
            std::memset(&asbd, 0, sizeof(asbd));
            asbd.mSampleRate       = double(s.sampleRate);
            asbd.mFormatID         = kAudioFormatLinearPCM;
            asbd.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
            asbd.mBytesPerPacket   = 8;
            asbd.mFramesPerPacket  = 1;
            asbd.mBytesPerFrame    = 8;
            asbd.mChannelsPerFrame = 2;
            asbd.mBitsPerChannel   = 32;
            if (CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &asbd, sizeof(layout), &layout, 0, nullptr, nullptr, &audioFormat) != noErr)
            {
                error = "Could not describe the audio format";
                return false;
            }

            if (![writer startWriting]) { error = describe(writer.error, "Could not start writing the video"); return false; }
            [writer startSessionAtSourceTime:kCMTimeZero];
            return true;
        }
    }

    bool writeVideo(const std::uint8_t* bgra, std::int64_t frameIndex) override
    {
        @autoreleasepool
        {
            if (writer == nil || writer.status != AVAssetWriterStatusWriting) return false;
            if (!waitUntilReady(videoInput, 25)) return true;               // encoder busy: drop this frame

            CVPixelBufferRef pixels = nullptr;
            CVPixelBufferPoolRef pool = adaptor.pixelBufferPool;
            if (pool == nullptr || CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pool, &pixels) != kCVReturnSuccess)
                if (CVPixelBufferCreate(kCFAllocatorDefault, size_t(settings.width), size_t(settings.height),
                                        kCVPixelFormatType_32BGRA, nullptr, &pixels) != kCVReturnSuccess)
                    return false;

            CVPixelBufferLockBaseAddress(pixels, 0);
            auto* dst = static_cast<std::uint8_t*>(CVPixelBufferGetBaseAddress(pixels));
            const size_t dstStride = CVPixelBufferGetBytesPerRow(pixels);
            const size_t rowBytes = size_t(settings.width) * 4;
            for (int y = 0; y < settings.height; ++y)
                std::memcpy(dst + size_t(y) * dstStride, bgra + size_t(y) * rowBytes, rowBytes);
            CVPixelBufferUnlockBaseAddress(pixels, 0);

            const BOOL ok = [adaptor appendPixelBuffer:pixels withPresentationTime:CMTimeMake(frameIndex, int32_t(settings.fps))];
            CVPixelBufferRelease(pixels);
            return ok == YES;
        }
    }

    bool writeAudio(const float* interleaved, int numFrames, std::int64_t startFrame) override
    {
        if (numFrames <= 0) return true;
        @autoreleasepool
        {
            if (writer == nil || writer.status != AVAssetWriterStatusWriting) return false;
            const size_t bytes = size_t(numFrames) * 8;
            CMBlockBufferRef block = nullptr;
            if (CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, nullptr, bytes, kCFAllocatorDefault, nullptr, 0, bytes,
                                                   kCMBlockBufferAssureMemoryNowFlag, &block) != kCMBlockBufferNoErr)
                return false;
            CMBlockBufferReplaceDataBytes(interleaved, block, 0, bytes);

            CMSampleBufferRef sample = nullptr;
            const OSStatus st = CMAudioSampleBufferCreateReadyWithPacketDescriptions(
                kCFAllocatorDefault, block, audioFormat, CMItemCount(numFrames),
                CMTimeMake(startFrame, int32_t(settings.sampleRate)), nullptr, &sample);
            CFRelease(block);
            if (st != noErr || sample == nullptr) return false;

            BOOL ok = NO;
            if (waitUntilReady(audioInput, 200)) ok = [audioInput appendSampleBuffer:sample];
            CFRelease(sample);
            return ok == YES;
        }
    }

    bool finish(std::string& error) override
    {
        @autoreleasepool
        {
            if (writer == nil) { error = "Nothing was recorded"; return false; }
            if (writer.status == AVAssetWriterStatusWriting)
            {
                [videoInput markAsFinished];
                [audioInput markAsFinished];
                dispatch_semaphore_t done = dispatch_semaphore_create(0);
                [writer finishWritingWithCompletionHandler:^{ dispatch_semaphore_signal(done); }];
                dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, (int64_t) (30 * NSEC_PER_SEC)));
            }
            const bool ok = writer.status == AVAssetWriterStatusCompleted;
            if (!ok) error = describe(writer.error, "The video file could not be finished");
            writer = nil; videoInput = nil; audioInput = nil; adaptor = nil;
            return ok;
        }
    }

private:
    static bool waitUntilReady(AVAssetWriterInput* input, int maxMs)
    {
        for (int i = 0; i < maxMs && !input.readyForMoreMediaData; ++i) usleep(1000);
        return input.readyForMoreMediaData == YES;
    }

    Settings settings;
    AVAssetWriter* writer = nil;
    AVAssetWriterInput* videoInput = nil;
    AVAssetWriterInput* audioInput = nil;
    AVAssetWriterInputPixelBufferAdaptor* adaptor = nil;
    CMAudioFormatDescriptionRef audioFormat = nullptr;
};
} // namespace

std::unique_ptr<VideoEncoder> VideoEncoder::create() { return std::make_unique<AVFoundationEncoder>(); }
} // namespace dali
