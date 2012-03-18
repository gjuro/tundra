// For conditions of distribution and use, see copyright notice in LICENSE

#include "DebugOperatorNew.h"

#include "KinectModule.h"
#include "KinectDevice.h"
#include "KinectSkeleton.h"
#include "Framework.h"
#include "CoreDefines.h"
#include "LoggingFunctions.h"

#include <MMSystem.h>

#include <QImage>
#include <QMutexLocker>
#include <QVariant>
#include <QDebug>
#include <QPainter>
#include <QRectF>
#include <QPair>

#include "MemoryLeakCheck.h"

KinectModule::KinectModule() : 
    IModule("KinectModule"),
    LC("[KinectModule]: "),
    kinectSensor_(0),
    kinectName_(0),
    videoSize_(640, 480),
    depthSize_(320, 240),
    tundraKinectDevice_(0),
    eventDepthFrame_(0),
    eventVideoFrame_(0),
    eventSkeletonFrame_(0),
    eventKinectProcessStop_(0),
    kinectProcess_(0),
    videoPreviewLabel_(0),
    depthPreviewLabel_(0),
    skeletonPreviewLabel_(0),
    debugging_(false) // Change to true to see debug widgets and prints
{
    if (debugging_)
    {
        videoPreviewLabel_ = new QLabel();
        videoPreviewLabel_->setWindowTitle("Kinect Video Preview");
        depthPreviewLabel_ = new QLabel();
        depthPreviewLabel_->setWindowTitle("Kinect Depth Preview");
        skeletonPreviewLabel_ = new QLabel();
        skeletonPreviewLabel_->setWindowTitle("Kinect Skeleton Preview");
    }

    // These signals are emitted from the processing thread, 
    // so we need Qt::QueuedConnection to get it to the main thread
    connect(this, SIGNAL(VideoAlert()), SLOT(OnVideoReady()), Qt::QueuedConnection);
    connect(this, SIGNAL(DepthAlert()), SLOT(OnDepthReady()), Qt::QueuedConnection);
    connect(this, SIGNAL(SkeletonsAlert()), SLOT(OnSkeletonsReady()), Qt::QueuedConnection);
    connect(this, SIGNAL(SkeletonTracking(bool)), SLOT(OnSkeletonTracking(bool)), Qt::QueuedConnection);
}

KinectModule::~KinectModule()
{
    SAFE_DELETE(videoPreviewLabel_);
    SAFE_DELETE(depthPreviewLabel_);
    SAFE_DELETE(skeletonPreviewLabel_);
}

void KinectModule::Initialize()
{
    // Push max count of skeleton objects to internal list.
    for (int i = 0; i<NUI_SKELETON_COUNT; i++)
        skeletons_ << new KinectSkeleton();

    StartKinect();

    tundraKinectDevice_ = new KinectDevice(this);
    if (GetFramework()->RegisterDynamicObject("kinect", tundraKinectDevice_))
        LogInfo(LC + "-- Registered 'kinect' dynamic object to Framework");
}

void KinectModule::Uninitialize()
{
    foreach(KinectSkeleton *skeleton, skeletons_)
    {
        if (skeleton)
            SAFE_DELETE(skeleton);
    }
    skeletons_.clear();

    StopKinect();

    SAFE_DELETE(tundraKinectDevice_);
}

bool KinectModule::HasKinect()
{
    if (IsStarted())
        return true;

    HRESULT result;
    int numKinects = 0;
    result = NuiGetSensorCount(&numKinects);
    if (FAILED(result))
        return false;

    INuiSensor *kinect = 0;
    result = NuiCreateSensorByIndex(0, &kinect);
    if (SUCCEEDED(result))
    {
        HRESULT status = kinect ? kinect->NuiStatus() : E_NUI_NOTCONNECTED;
        if (status == E_NUI_NOTCONNECTED)
        {
            LogWarning(LC + "Kinect is not connected to the machine.");
            kinect->Release();
            return false;
        }
        else if (status == E_NUI_NOTREADY)
        {
            LogWarning(LC + "Kinect was found but not ready, some part of the device not connected.");
            kinect->Release();
            return false;
        }

        kinect->Release();
        return true;
    }
    if (kinect)
        kinect->Release();

    return false;
}

bool KinectModule::IsStarted()
{
    return (kinectSensor_ != 0);
}

bool KinectModule::StartKinect()
{
    if (!HasKinect())
    {
        LogError(LC + "Cannot start Kinect, no device available!");
        return false;
    }
    if (IsStarted())
    {
        LogInfo("Kinect already started. Use StopKinect() first if you want to restart.");
        return false;
    }

    QString message;
    HRESULT result;
    
    result = NuiCreateSensorByIndex(0, &kinectSensor_);
    if (FAILED(result))
        return false;

    // Get device name
    SysFreeString(kinectName_);
    kinectName_ = kinectSensor_->NuiDeviceConnectionId();

    // Init events
    eventDepthFrame_ = CreateEventA(NULL, TRUE, FALSE, NULL);
    eventVideoFrame_ = CreateEventA(NULL, TRUE, FALSE, NULL);
    eventSkeletonFrame_ = CreateEventA(NULL, TRUE, FALSE, NULL);

    LogInfo(LC + "Successfully found a Kinect device.");

    DWORD nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON |  NUI_INITIALIZE_FLAG_USES_COLOR;
    result = kinectSensor_->NuiInitialize(nuiFlags);
    if (result == E_NUI_SKELETAL_ENGINE_BUSY)
    {
        LogWarning(LC + "-- Skeletal engine busy, trying to initialize without skeleton tracking!");
        nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH |  NUI_INITIALIZE_FLAG_USES_COLOR;
        result = kinectSensor_->NuiInitialize(nuiFlags);
    }
    if (FAILED(result))
    {
        if (result == E_NUI_DEVICE_IN_USE)
            LogError(LC + "Kinect device is already in use, cannot start!");
        else
            LogError(LC + "Starting kinect failed for unknown reason!");
        if (kinectSensor_)
        {
            kinectSensor_->NuiShutdown();
            kinectSensor_->Release();
        }
        kinectSensor_= 0;
        return false;
    }

    // Precautionary null check
    if (!kinectSensor_)
    {
        LogError(LC + "Kinect pointer after succesfull init is null?!");
        return false;
    }

    // Resolve the wanted video/depth resolution
    NUI_IMAGE_RESOLUTION depthResolution = NUI_IMAGE_RESOLUTION_320x240;
    depthSize_ = QSize(320, 240);

    NUI_IMAGE_RESOLUTION videoResolution = NUI_IMAGE_RESOLUTION_INVALID;
    if (videoSize_.width() == 320 && videoSize_.height() == 240)
        videoResolution = NUI_IMAGE_RESOLUTION_320x240;
    else if (videoSize_.width() == 640 && videoSize_.height() == 480)
        videoResolution = NUI_IMAGE_RESOLUTION_640x480;
    if (videoResolution == NUI_IMAGE_RESOLUTION_INVALID)
    {
        videoResolution = NUI_IMAGE_RESOLUTION_640x480;
        videoSize_ = QSize(640, 480);
    }

    // Skeleton
    if (HasSkeletalEngine(kinectSensor_))
    {
        result = kinectSensor_->NuiSkeletonTrackingEnable(eventSkeletonFrame_, 0);
        message = SUCCEEDED(result) ? "-- Skeleton tracking enabled" : "-- Skeleton tracking could not be enabled";
        LogInfo(LC + message);
    }

    // Image
    result = kinectSensor_->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, videoResolution,
        0, 2, eventVideoFrame_, &kinectVideoStream_);
    message = SUCCEEDED(result) ? "-- Image stream enabled" : "-- Image stream could not be enabled";
    LogInfo(LC + message);

    // Depth
    result = kinectSensor_->NuiImageStreamOpen(HasSkeletalEngine(kinectSensor_) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH, 
        depthResolution, 0, 2, eventDepthFrame_, &kinectDepthStream_);
    message = SUCCEEDED(result) ? "-- Depth stream enabled" : "-- Depth stream could not be enabled";
    LogInfo(LC + message);

    // Start the kinect processing thread
    eventKinectProcessStop_ = CreateEventA(NULL, FALSE, FALSE, NULL);
    kinectProcess_ = CreateThread(NULL, 0, KinectProcessThread, this, 0, NULL);

    return true;
}

void KinectModule::StopKinect()
{
    // Stop the Kinect processing thread
    if (eventKinectProcessStop_ != NULL)
    {
        // Signal the thread
        SetEvent(eventKinectProcessStop_);

        // Wait for thread to stop
        if (kinectProcess_ != NULL)
        {
            WaitForSingleObject(kinectProcess_, INFINITE);
            CloseHandle(kinectProcess_);
        }
        CloseHandle(eventKinectProcessStop_);
    }

    // Shutdown Kinect
    if (kinectSensor_)
        kinectSensor_->NuiShutdown();

    // Close related even handles
    if (eventSkeletonFrame_ && (eventSkeletonFrame_ != INVALID_HANDLE_VALUE))
    {
        CloseHandle(eventSkeletonFrame_);
        eventSkeletonFrame_ = 0;
    }
    if (eventDepthFrame_ && (eventDepthFrame_ != INVALID_HANDLE_VALUE))
    {
        CloseHandle(eventDepthFrame_);
        eventDepthFrame_ = 0;
    }
    if (eventVideoFrame_ && (eventVideoFrame_ != INVALID_HANDLE_VALUE))
    {
        CloseHandle(eventVideoFrame_);
        eventVideoFrame_ = 0;
    }

    // Release ptr
    if (kinectSensor_)
        kinectSensor_->Release();
    kinectSensor_ = 0;

    // Release Kinect name string
    SysFreeString(kinectName_);

    // Reset skeletons
    foreach(KinectSkeleton *skeleton, skeletons_)
    {
        if (skeleton)
        {
            skeleton->ResetSkeleton();
            skeleton->emitTrackingChange_ = false;
        }
    }
}

QImage KinectModule::VideoImage()
{
    QMutexLocker lock(&mutexVideo_);
    return video_;
}

QImage KinectModule::DepthImage()
{
    QMutexLocker lock(&mutexDepth_);
    return depth_;
}

DWORD WINAPI KinectModule::KinectProcessThread(LPVOID pParam)
{
    KinectModule *pthis = (KinectModule*)pParam;
    KinectHelper helper = pthis->helper_;

    HANDLE kinectEvents[4];
    int eventIndex;

    // Configure events to be listened on
    kinectEvents[0] = pthis->eventKinectProcessStop_;
    kinectEvents[1] = pthis->eventDepthFrame_;
    kinectEvents[2] = pthis->eventVideoFrame_;
    kinectEvents[3] = pthis->eventSkeletonFrame_;

    // Main thread loop
    while(1)
    {
        // Wait for an event to be signaled
        eventIndex = WaitForMultipleObjects(sizeof(kinectEvents) / sizeof(kinectEvents[0]), kinectEvents, FALSE, 100);
        if (eventIndex == 0)
            break;            

        // Process signal events
        switch (eventIndex)
        {
            case 1:
            {          
                pthis->GetDepth();
                break;
            }
            case 2:
            {
                pthis->GetVideo();
                break;
            }
            case 3:
            {
                pthis->GetSkeleton();
                break;
            }
            default:
                break;
        }
    }

    return 0;
}

void KinectModule::GetVideo()
{
    if (!kinectSensor_)
        return;

    NUI_IMAGE_FRAME imageFrame;
    HRESULT videoResult = kinectSensor_->NuiImageStreamGetNextFrame(kinectVideoStream_, 0, &imageFrame);
    if (FAILED(videoResult))
        return;

    INuiFrameTexture *texture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT lockedRect;
    texture->LockRect(0, &lockedRect, NULL, 0);
    if (lockedRect.Pitch != 0)
    {
        bool shouldEmit = false;

        // Create QImage
        DWORD frameWidth, frameHeight;
        NuiImageResolutionToSize(imageFrame.eResolution, frameWidth, frameHeight);
        QImage videoImage(static_cast<BYTE*>(lockedRect.pBits), frameWidth, frameHeight, QImage::Format_RGB32);

        // Update video image if not null
        if (!videoImage.isNull())
        {
            shouldEmit = true;
            QMutexLocker lock(&mutexVideo_);
            video_ = videoImage;
        }

        // Emit alert of the new frame
        if (shouldEmit)
            emit VideoAlert();
    }
    texture->UnlockRect(0);

    kinectSensor_->NuiImageStreamReleaseFrame(kinectVideoStream_, &imageFrame);
}

void KinectModule::OnVideoReady()
{
    // Lock video mutex if debugging before emitting data signal to 3rd party code.
    if (debugging_ && videoPreviewLabel_)
    {
        QMutexLocker lock(&mutexVideo_);
        if (videoPreviewLabel_->size() != video_.size())
            videoPreviewLabel_->resize(video_.size());
        if (!videoPreviewLabel_->isVisible())
            videoPreviewLabel_->show();
        videoPreviewLabel_->setPixmap(QPixmap::fromImage(video_));
    }

    if (tundraKinectDevice_)
        tundraKinectDevice_->EmitVideoUpdate();
}

void KinectModule::GetDepth()
{
    if (!kinectSensor_)
        return;

    NUI_IMAGE_FRAME imageFrame;
    HRESULT depthResult = kinectSensor_->NuiImageStreamGetNextFrame(kinectDepthStream_, 0, &imageFrame);
    if (FAILED(depthResult))
        return;

    INuiFrameTexture *depthTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT depthRect;
    depthTexture->LockRect(0, &depthRect, NULL, 0);
    if (depthRect.Pitch != 0)
    {
        bool shouldEmit = false;
        BYTE *depthByteBuffer = (BYTE*)depthRect.pBits;

        // Draw the bits to the bitmap
        RGBQUAD rgbrunTable[640 * 480]; // This will work even if our image is smaller
        RGBQUAD *rgbrun = rgbrunTable;
        USHORT *pBufferRun = (USHORT*)depthByteBuffer;
        for (int y=0; y<depthSize_.width(); y++)
        {
            for (int x=0; x<depthSize_.height(); x++)
            {
                RGBQUAD pixelRgb = helper_.ConvertDepthShortToQuad(*pBufferRun);
                *rgbrun = pixelRgb;

                pBufferRun++;
                rgbrun++;
            }
        }

        // Create QImage and update depth image if not null
        QImage depthImage((BYTE*)rgbrunTable, depthSize_.width(), depthSize_.height(), QImage::Format_RGB32);
        if (!depthImage.isNull())
        {
            shouldEmit = true;
            QMutexLocker lock(&mutexDepth_);
            depth_ = depthImage;
        }

        // Emit alert of the new frame
        if (shouldEmit)
            emit DepthAlert();
    }
    else
        LogWarning(LC + "Buffer length of received texture is bogus!");

    depthTexture->UnlockRect(0);
    kinectSensor_->NuiImageStreamReleaseFrame(kinectDepthStream_, &imageFrame);
}

void KinectModule::OnDepthReady()
{
    // Lock video mutex if debugging before emitting data signal to 3rd party code.
    if (debugging_ && depthPreviewLabel_)
    {
        QMutexLocker lock(&mutexDepth_);
        if (depthPreviewLabel_->size() != depth_.size())
            depthPreviewLabel_->resize(depth_.size());
        if (!depthPreviewLabel_->isVisible())
            depthPreviewLabel_->show();
        depthPreviewLabel_->setPixmap(QPixmap::fromImage(depth_));
    }

    if (tundraKinectDevice_)
        tundraKinectDevice_->EmitDepthUpdate();
}

void KinectModule::GetSkeleton()
{
    if (!kinectSensor_)
        return;

    NUI_SKELETON_FRAME skeletonFrame = {0};
    HRESULT skeletonResult = kinectSensor_->NuiSkeletonGetNextFrame(0, &skeletonFrame);
    if (FAILED(skeletonResult))
        return;

    /// \todo Support NUI_SKELETON_POSITION_ONLY. Whatever it might be, find out!
    bool foundSkeletons = false;
    for (int i = 0; i<NUI_SKELETON_COUNT; i++)
        if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED)
            foundSkeletons = true;

    // Set our KinectDevices tracking state, this will be handy
    // on 3rd party using code to know when tracking data should be coming
    // to eg. reset things when skeleton tracking is lost
    if (!foundSkeletons)
    {
        if (tundraKinectDevice_ && tundraKinectDevice_->IsTrackingSkeletons())
            emit SkeletonTracking(false);
        return;
    }

    // Smooth out the skeleton data (not actually sure what this does).
    // Could support this more extensively via UI and a NUI_TRANSFORM_SMOOTH_PARAMETERS as second param!
    skeletonResult = kinectSensor_->NuiTransformSmooth(&skeletonFrame, NULL);
    if (FAILED(skeletonResult))
        return;

    // Inform 3rd party code that tracking is now enabled.
    if (tundraKinectDevice_ && !tundraKinectDevice_->IsTrackingSkeletons())
        emit SkeletonTracking(true);

    // Lock skeletons and update
    {
        QMutexLocker lock(&mutexSkeleton_);
        for (int i=0; i<NUI_SKELETON_COUNT; i++)
        {
            KinectSkeleton *skeleton = skeletons_.value(i);
            if (!skeleton)
            {
                LogError(LC + "KinectSkeleton for index " + QString::number(i) + " is null!");
                continue;
            }

            if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&
                skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)
            {
                skeleton->UpdateSkeleton(skeletonFrame.SkeletonData[i]);
            }
            else
            {
                skeleton->ResetSkeleton();
            }
        }
    }

    emit SkeletonsAlert();
}

void KinectModule::OnSkeletonTracking(bool tracking)
{
    tundraKinectDevice_->SetTrackingSkeletons(tracking);

    if (debugging_)
        LogInfo(LC + "Tracking Skeletons    " + (tracking ? "ON" : "OFF"));
}

void KinectModule::OnSkeletonsReady()
{
    if (debugging_)
        DrawDebugSkeletons();

    QMutexLocker lock(&mutexSkeleton_);
    foreach(KinectSkeleton *skeleton, skeletons_)
    {
        if (!skeleton)
            continue;

        // This step will let 3rd party code (scripts) to hook to the skeleton ptr signals.
        // Only emit this signal when state changes. 3rd party code needs to do some book keeping
        // to know if the ID of the skeleton has already been connected or not.
        if (skeleton->emitTrackingChange_)
            tundraKinectDevice_->EmitSkeletonUpdate(skeleton);

        // This makes the skeleton emit tracking changes if one occurred, true or false as the param.
        skeleton->EmitIfTrackingChanged();

        // This makes the skeleton emit the updated signal that 3rd party code needs to catch and
        // if the code so chooses it can get bone positions etc. data with the designated slots/properties.
        skeleton->EmitIfUpdated();
    }
}

void KinectModule::DrawDebugSkeletons()
{
    if (!debugging_)
        return;

    QRectF imgRect(0, 0, 640, 480);
    QSize imgSize = imgRect.size().toSize();
    QImage manipImage(imgSize, QImage::Format_RGB32);

    QPainter p(&manipImage);
    p.setPen(Qt::black);
    p.setBrush(QBrush(Qt::white));
    p.drawRect(imgRect);

    {
        QMutexLocker lock(&mutexSkeleton_);

        QList<QPair<QString, QString> > bonePairs;
        bonePairs << QPair<QString, QString>("hip", "knee");
        bonePairs << QPair<QString, QString>("ankle", "knee");
        bonePairs << QPair<QString, QString>("ankle", "foot");
        bonePairs << QPair<QString, QString>("shoulder", "elbow");
        bonePairs << QPair<QString, QString>("wrist", "elbow");
        bonePairs << QPair<QString, QString>("wrist", "hand");

        QStringList sides;
        sides << "-left" << "-right";

        int skeletonIndex = 0;
        foreach(KinectSkeleton *skeleton, skeletons_)
        {
            if (!skeleton)
                continue;
            if (!skeleton->IsTracking())
                continue;

            if (skeletonIndex == 0)
                p.setBrush(QBrush(Qt::red));
            else if (skeletonIndex == 1)
                p.setBrush(QBrush(Qt::blue));
            else if (skeletonIndex == 3)
                p.setBrush(QBrush(Qt::green));
            else if (skeletonIndex == 4)
                p.setBrush(QBrush(Qt::magenta));

            // Draw bone lines
            QPen pen(Qt::SolidLine);
            pen.setWidth(3);
            pen.setBrush(p.brush()); 
            p.setPen(pen);

            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-head"), skeleton->BonePosition("bone-shoulder-center"));
            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-shoulder-center"), skeleton->BonePosition("bone-spine"));
            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-hip-center"), skeleton->BonePosition("bone-spine"));
            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-shoulder-left"), skeleton->BonePosition("bone-shoulder-center"));
            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-shoulder-right"), skeleton->BonePosition("bone-shoulder-center"));
            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-hip-center"), skeleton->BonePosition("bone-hip-left"));
            helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-hip-center"), skeleton->BonePosition("bone-hip-right"));

            foreach(QString side, sides)
                for(int i=0; i < bonePairs.length(); ++i)
                    helper_.ConnectBones(&p, imgRect, skeleton->BonePosition("bone-" + bonePairs[i].first + side), skeleton->BonePosition("bone-" + bonePairs[i].second + side));

            // Draw bone positions
            pen.setWidth(1);
            pen.setBrush(p.brush()); 
            p.setPen(pen);

            foreach(QString boneName, skeleton->BoneNames())
            {
                float4 vec = skeleton->BonePosition(boneName);
                vec *= 200;
                QPointF pos(vec.x, vec.y);

                if (boneName == "bone-head")
                    p.drawEllipse(imgRect.center() + pos, 15, 15);
                else
                    p.drawEllipse(imgRect.center() + pos, 4, 4);
            }

            skeletonIndex++;
        }
    }

    p.end();

    skeletonPreviewLabel_->resize(imgSize);
    skeletonPreviewLabel_->show();
    skeletonPreviewLabel_->setPixmap(QPixmap::fromImage(manipImage.mirrored(false, true))); // Need to do a vertical flip for some reason
}

extern "C"
{
    DLLEXPORT void TundraPluginMain(Framework *fw)
    {
        Framework::SetInstance(fw); // Inside this DLL, remember the pointer to the global framework object.
        IModule *module = new KinectModule();
        fw->RegisterModule(module);
    }
}
