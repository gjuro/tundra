// For conditions of distribution and use, see copyright notice in LICENSE

#include "DebugOperatorNew.h"

#include "KinectModule.h"
#include "KinectDevice.h"

#include "Framework.h"
#include "CoreDefines.h"
#include "Profiler.h"
#include "LoggingFunctions.h"

#include "JavascriptModule.h"
#include "QScriptEngineHelpers.h"

#include "UiAPI.h"
#include "UiMainWindow.h"

#include <QMutexLocker>
#include <QPair>
#include <QPixmap>
#include <QDebug>
#include <QThread>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

#include "MemoryLeakCheck.h"

Q_DECLARE_METATYPE(KinectDevice*)
Q_DECLARE_METATYPE(KinectSkeleton*)

KinectModule::KinectModule() : 
    IModule("KinectModule"),
    LC("[KinectModule]: "),
    kinectSensor_(0),
    kinectName_(0),
    videoSize_(640, 480),
    depthSize_(320, 240),
    kinectDevice_(0),
    eventDepthFrame_(0),
    eventVideoFrame_(0),
    eventSkeletonFrame_(0),
    eventKinectProcessStop_(0),
    kinectProcess_(0),
    kinectWidget_(0)
{
    drawBoneSides_ << "-left" 
                   << "-right";
    drawBonePairs_ << QPair<QString, QString>("hip", "knee")
                   << QPair<QString, QString>("ankle", "knee")
                   << QPair<QString, QString>("ankle", "foot")
                   << QPair<QString, QString>("shoulder", "elbow")
                   << QPair<QString, QString>("wrist", "elbow")
                   << QPair<QString, QString>("wrist", "hand");
}

KinectModule::~KinectModule()
{
}

void KinectModule::Initialize()
{
    if (framework_->IsHeadless())
        return;

    // Connect javascript engine creation.
    JavascriptModule *javascriptModule = framework_->GetModule<JavascriptModule>();
    if (javascriptModule)
        connect(javascriptModule, SIGNAL(ScriptEngineCreated(QScriptEngine*)), SLOT(OnScriptEngineCreated(QScriptEngine*)));
    else
        LogWarning(LC + "JavascriptModule not present, KinectModule usage from scripts will be limited!");

    // These signals are emitted from the processing thread, 
    // so we need Qt::QueuedConnection to get them to the main thread.
    connect(this, SIGNAL(VideoAlert()), SLOT(OnVideoReady()), Qt::QueuedConnection);
    connect(this, SIGNAL(DepthAlert()), SLOT(OnDepthReady()), Qt::QueuedConnection);
    connect(this, SIGNAL(SkeletonsAlert()), SLOT(OnSkeletonsReady()), Qt::QueuedConnection);
    connect(this, SIGNAL(SkeletonTracking(bool)), SLOT(OnSkeletonTracking(bool)), Qt::QueuedConnection);

    // Init control panel widget and menu item
    kinectWidget_ = new QWidget();

    kinectUi_.setupUi(kinectWidget_);
    connect(kinectUi_.buttonStartStop, SIGNAL(clicked()), SLOT(OnStartStopToggled()));
    connect(kinectUi_.buttonAdjustElevation, SIGNAL(clicked()), SLOT(ShowElevationWidget()));

    if (framework_->Ui()->MainWindow())
    {
        kinectWidget_->setWindowIcon(framework_->Ui()->MainWindow()->windowIcon());

        QAction *controlPanelAction = framework_->Ui()->MainWindow()->AddMenuAction("Kinect", "Control Panel");
        if (controlPanelAction)
            connect(controlPanelAction, SIGNAL(triggered()), SLOT(OnShowControlPanel()));
    }

    // Push max count of skeleton objects to internal list.
    for (int i = 0; i<NUI_SKELETON_COUNT; i++)
        skeletons_ << new KinectSkeleton();

    kinectDevice_ = new KinectDevice(this);
    framework_->RegisterDynamicObject("kinect", kinectDevice_);

    // Mandatory Kinect status callback
    NuiSetDeviceStatusCallback(&KinectModule::KinectStatusChange, this);
}

void KinectModule::Uninitialize()
{
    if (framework_->IsHeadless())
        return;

    foreach(KinectSkeleton *skeleton, skeletons_)
        if (skeleton)
            SAFE_DELETE(skeleton);
    skeletons_.clear();

    StopKinect();

    SAFE_DELETE(kinectDevice_);
    SAFE_DELETE(kinectWidget_);
}

bool KinectModule::HasKinect()
{
    if (framework_->IsHeadless())
    {
        LogError(LC + "You are calling HasKinect() in headless mode. Kinect is not supported in headless mode!");
        return false;
    }

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
    if (framework_->IsHeadless())
    {
        LogError(LC + "You are calling StartKinect() in headless mode. Kinect is not supported in headless mode!");
        return false;
    }

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
        {
            LogError(LC + "Kinect device is already in use, cannot start!");
            if (kinectWidget_)
                kinectUi_.labelStatus->setText("<span style=\"color:red;\">Kinect device is already in use, cannot start!</span>");
        }
        else
        {
            LogError(LC + "Starting kinect failed for unknown reason!");
            if (kinectWidget_)
                kinectUi_.labelStatus->setText("<span style=\"color:red;\">Starting kinect failed for unknown reason!</span>");
        }
        if (kinectSensor_)
        {
            kinectSensor_->NuiShutdown();
            kinectSensor_->Release();
        }
        kinectSensor_= 0;
        return false;
    }

    LogInfo(LC + "Kinect started.");

    // Precautionary null check
    if (!kinectSensor_)
    {
        LogError(LC + "Kinect sensor pointer after successful init is null?!");
        StopKinect();
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

    // Reset images behind the mutexes
    {
        QMutexLocker lockVideo(&mutexVideo_);
        QImage blankVideo(videoSize_, QImage::Format_RGB32);
        blankVideo.fill(Qt::black);
        video_ = blankVideo;
    }
    {
        QMutexLocker lockDepth(&mutexDepth_);
        QImage blankDepth(depthSize_, QImage::Format_RGB32);
        blankDepth.fill(Qt::black);
        depth_ = blankDepth;
    }

    UpdateControlPanelState();

    return true;
}

void KinectModule::StopKinect()
{
    if (framework_->IsHeadless())
    {
        LogError(LC + "You are calling StopKinect() in headless mode. Kinect is not supported in headless mode!");
        return;
    }

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
        eventKinectProcessStop_ = 0;
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
    {
        kinectSensor_->Release();
        LogInfo(LC + "Kinect stopped.");
    }
    kinectSensor_ = 0;

    // Release Kinect name string
    SysFreeString(kinectName_);

    // Reset skeletons
    {
        QMutexLocker lockSkeletons(&mutexSkeleton_);
        foreach(KinectSkeleton *skeleton, skeletons_)
        {
            if (skeleton)
            {
                skeleton->ResetSkeleton();
                skeleton->emitTrackingChange = false;
            }
        }
    }

    // Reset images behind the mutexes
    {
        QMutexLocker lockVideo(&mutexVideo_);
        QImage blankVideo(videoSize_, QImage::Format_RGB32);
        blankVideo.fill(Qt::black);
        video_ = blankVideo;
    }
    {
        QMutexLocker lockDepth(&mutexDepth_);
        QImage blankDepth(depthSize_, QImage::Format_RGB32);
        blankDepth.fill(Qt::black);
        depth_ = blankDepth;
    }

    if (kinectElevationWidget_)
        kinectElevationWidget_->close();

    UpdateControlPanelState();
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

KinectDevice* KinectModule::Kinect()
{
    return kinectDevice_;
}

DWORD WINAPI KinectModule::KinectProcessThread(LPVOID pParam)
{
    KinectModule *kinectModule = (KinectModule*)pParam;

    const int numEvents = 4;
    HANDLE kinectEvents[numEvents];
    int eventIndex;

    // Configure events to be listened on
    kinectEvents[0] = kinectModule->eventKinectProcessStop_;
    kinectEvents[1] = kinectModule->eventDepthFrame_;
    kinectEvents[2] = kinectModule->eventVideoFrame_;
    kinectEvents[3] = kinectModule->eventSkeletonFrame_;

    // Main thread loop
    while(1)
    {
        // Wait for an event to be signaled
        eventIndex = WaitForMultipleObjects(numEvents, kinectEvents, FALSE, 100);

        // Thread stop event
        if (eventIndex == 0)
            break;

        // Process signal events
        switch (eventIndex)
        {
            case 1:
            {          
                kinectModule->GetDepth();
                break;
            }
            case 2:
            {
                kinectModule->GetVideo();
                break;
            }
            case 3:
            {
                kinectModule->GetSkeleton();
                break;
            }
            default:
                break;
        }
    }

    return 0;
}

void CALLBACK KinectModule::KinectStatusChange(HRESULT status, const OLECHAR *instanceName, const OLECHAR *uniqueDeviceName, void *userData)
{
    /*
    KinectModule *kinectModule = reinterpret_cast<KinectModule*>(userData);
    if (SUCCEEDED(status))
    {
        if (status == S_OK)
            qDebug() << "KinectStatusChange: OK";
        else
            qDebug() << "KinectStatusChange: NOT OK";
    }
    else
        qDebug() << "KinectStatusChange: ERROR";
    qDebug() << "Instance:" << instanceName << "Device:" << uniqueDeviceName << "Status:" << status;
    */
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
    PROFILE(KinectModule_Update_Color)
    // Lock video mutex for preview before emitting data signal to 3rd party code.
    if (kinectWidget_ && kinectWidget_->isVisible())
    {
        PROFILE(Update_Control_Panel_Color_Image)
        QMutexLocker lock(&mutexVideo_);
        if (video_.size() != kinectUi_.widgetColor->size())
            kinectUi_.widgetColor->setPixmap(QPixmap::fromImage(video_.scaled(kinectUi_.widgetColor->size(), Qt::KeepAspectRatio)));
        else
            kinectUi_.widgetColor->setPixmap(QPixmap::fromImage(video_));
    }

    if (kinectDevice_)
    {
        PROFILE(KinectDevice_EmitVideoUpdate)
        kinectDevice_->EmitVideoUpdate();
    }
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
                RGBQUAD pixelRgb = ConvertDepthShortToQuad(*pBufferRun);
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
    PROFILE(KinectModule_Update_Depth)
    // Lock depth mutex for preview before emitting data signal to 3rd party code.
    if (kinectWidget_ && kinectWidget_->isVisible())
    {
        PROFILE(Update_Control_Panel_Depth_Image)
        QMutexLocker lock(&mutexDepth_);
        if (depth_.size() != kinectUi_.widgetDepth->size())
            kinectUi_.widgetDepth->setPixmap(QPixmap::fromImage(depth_.scaled(kinectUi_.widgetDepth->size(), Qt::KeepAspectRatio)));
        else
            kinectUi_.widgetDepth->setPixmap(QPixmap::fromImage(depth_));
    }

    if (kinectDevice_)
    {
        PROFILE(KinectDevice_EmitDepthUpdate)
        kinectDevice_->EmitDepthUpdate();
    }
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
        if (kinectDevice_ && kinectDevice_->IsTrackingSkeletons())
            emit SkeletonTracking(false);
        return;
    }

    // Smooth out the skeleton data (not actually sure what this does).
    // Could support this more extensively via UI and a NUI_TRANSFORM_SMOOTH_PARAMETERS as second param!
    skeletonResult = kinectSensor_->NuiTransformSmooth(&skeletonFrame, NULL);
    if (FAILED(skeletonResult))
        return;

    // Inform 3rd party code that tracking is now enabled.
    if (kinectDevice_ && !kinectDevice_->IsTrackingSkeletons())
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
    // Tracking lost for all skeletons, clear the rendering
    if (!tracking && kinectWidget_)
    {
        QPixmap blank(kinectUi_.widgetSkeletons->size());
        blank.fill(Qt::black);
        QPainter p(&blank);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 10));
        p.drawText(blank.rect(), "No Skeletons Detected", QTextOption(Qt::AlignCenter));
        p.end();
        kinectUi_.widgetSkeletons->setPixmap(blank);
    }
    kinectDevice_->SetTrackingSkeletons(tracking);
}

void KinectModule::OnSkeletonsReady()
{
    PROFILE(KinectModule_Update_Skeleton)

    DrawSkeletons();

    QMutexLocker lock(&mutexSkeleton_);
    foreach(KinectSkeleton *skeleton, skeletons_)
    {
        if (!skeleton)
            continue;

        if (skeleton->emitTrackingChange)
        {
            // Signal skeleton tracking change from device and the skeleton itself.
            PROFILE(KinectDevice_EmitSkeletonStateChanged)
            kinectDevice_->EmitSkeletonStateChanged(skeleton);
            PROFILE(KinectSkeleton_EmitTrackingChanged)
            skeleton->EmitTrackingChanged();
        }

        if (skeleton->IsUpdated())
        {
            // Signal skeleton bone positions updated from device and skeleton itself.
            PROFILE(KinectDevice_EmitSkeletonUpdate)
            kinectDevice_->EmitSkeletonUpdate(skeleton);
            PROFILE(KinectSkeleton_EmitUpdated)
            skeleton->EmitUpdated();
        }
    }
}

void KinectModule::OnStartStopToggled()
{
    if (!IsStarted())
        StartKinect();
    else
        StopKinect();
}

void KinectModule::ShowElevationWidget()
{
    if (!kinectWidget_ || !kinectWidget_->isVisible())
        return;

    // Show existing if alive
    if (kinectElevationWidget_)
        SAFE_DELETE(kinectElevationWidget_);

    LONG elevationNow = 0;
    HRESULT result = NuiCameraElevationGetAngle(&elevationNow);
    if (FAILED(result))
    {
        LogError("Failed to read Kinect elevation angle, cannot show or set elevation angle.");
        return;
    }

    kinectElevationWidget_ = new QWidget();
    kinectElevationWidget_->setAttribute(Qt::WA_DeleteOnClose, true);
    
    QSlider *slider = new QSlider(Qt::Vertical, kinectElevationWidget_);
    slider->setObjectName("kinectElevationSlider");
    slider->setMinimum((int)NUI_CAMERA_ELEVATION_MINIMUM);
    slider->setMaximum((int)NUI_CAMERA_ELEVATION_MAXIMUM);
    slider->setSingleStep(1);
    slider->setPageStep(1);
    slider->setValue((int)elevationNow);

    connect(slider, SIGNAL(sliderReleased()), this, SLOT(OnElevationChanged()));

    QLabel *labelDegrees = new QLabel(QString("%1 degrees").arg((int)elevationNow), kinectElevationWidget_);
    labelDegrees->setObjectName("kinectElevationLabel");
    labelDegrees->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    labelDegrees->setAlignment(Qt::AlignLeft|Qt::AlignBottom);
    labelDegrees->setFont(QFont("Arial", 18));

    QHBoxLayout *layout = new QHBoxLayout(kinectElevationWidget_);
    layout->addWidget(slider);
    layout->addWidget(labelDegrees);

    kinectElevationWidget_->setWindowTitle("Kinect Elevation");
    kinectElevationWidget_->setWindowFlags(Qt::Tool);
    kinectElevationWidget_->setLayout(layout);

    kinectElevationWidget_->resize(150, 150);
    kinectElevationWidget_->show();
}

void KinectModule::OnElevationChanged()
{
    if (!kinectElevationWidget_)
        return;

    QSlider *slider = kinectElevationWidget_->findChild<QSlider*>("kinectElevationSlider");
    QLabel *labelDegrees = kinectElevationWidget_->findChild<QLabel*>("kinectElevationLabel");
    if (slider && labelDegrees)
    {
        int value = slider->value();
        if (value < (int)NUI_CAMERA_ELEVATION_MINIMUM)
            value = (int)NUI_CAMERA_ELEVATION_MINIMUM;
        if (value > (int)NUI_CAMERA_ELEVATION_MAXIMUM)
            value = (int)NUI_CAMERA_ELEVATION_MAXIMUM;

        labelDegrees->setText(QString("%1 degrees").arg(value));

        HRESULT result = NuiCameraElevationSetAngle((LONG)value);
        if (FAILED(result))
            labelDegrees->setText("<span style=\"color:red;\">Error setting degrees</span>");
    }
}

void KinectModule::OnShowControlPanel()
{
    if (!kinectWidget_)
        return;

    if (kinectWidget_->isVisible())
    {
        QApplication::setActiveWindow(kinectWidget_);
        kinectWidget_->setFocus();
        return;
    }

    kinectWidget_->show();
    UpdateControlPanelState();
}

void KinectModule::UpdateControlPanelState()
{
    if (!kinectWidget_)
        return;

    kinectUi_.buttonAdjustElevation->setEnabled(false);

    if (!HasKinect())
    {
        kinectUi_.labelStatus->setText("<span style=\"color:red;\">No Kinect devices detected on the machine</span>");
        kinectUi_.buttonStartStop->setText("Start Kinect");
        kinectUi_.buttonStartStop->setEnabled(false);    
        ResetControlPanelRendering();
    }
    else
    {
        if (!IsStarted())
        {
            kinectUi_.labelStatus->setText("Kinect device available");
            kinectUi_.buttonStartStop->setText("Start Kinect");
            ResetControlPanelRendering();
        }
        else
        {
            kinectUi_.labelStatus->setText("Kinect device running");
            kinectUi_.buttonStartStop->setText("Stop Kinect");
            kinectUi_.buttonAdjustElevation->setEnabled(true);
        }
        kinectUi_.buttonStartStop->setEnabled(true);
    }
}

void KinectModule::ResetControlPanelRendering()
{
    if (!kinectWidget_)
        return;

    // Depth and skeleton are the same size.
    QPixmap blank(kinectUi_.widgetDepth->size());
    blank.fill(Qt::black);
    kinectUi_.widgetDepth->setPixmap(blank);
    
    // Draw some text to indicate no skeletons have been detected.
    QPainter p(&blank);
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 10));
    p.drawText(blank.rect(), "No Skeletons Detected", QTextOption(Qt::AlignCenter));
    p.end();
    kinectUi_.widgetSkeletons->setPixmap(blank);

    // Video is a bit bigger so create separate pixmap for it.
    QPixmap blankVideo(kinectUi_.widgetColor->size());
    blankVideo.fill(Qt::black);
    kinectUi_.widgetColor->setPixmap(blankVideo);
}

void KinectModule::DrawSkeletons()
{
    PROFILE(Update_Control_Panel_Skeleton_Image)
    if (!kinectWidget_ || !kinectWidget_->isVisible())
        return;  

    // Check from the ui settings if we should draw only tracked bones or also inferred bones.
    int boneRenderingMode = kinectUi_.comboBoneRendering != 0 ? kinectUi_.comboBoneRendering->currentIndex() : 0;

    // Prepare canvas image
    QRect imgRect(0, 0, kinectUi_.widgetSkeletons->width(), kinectUi_.widgetSkeletons->height());
    QImage skeletonImage(imgRect.size(), QImage::Format_RGB32);
    skeletonImage.fill(Qt::black);

    // Prepare brush and pen
    QBrush brush(Qt::SolidPattern);

    QPen pen(Qt::SolidLine);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setWidth(1);
    pen.setColor(Qt::white);

    QPainter p(&skeletonImage);
    

    {
        QMutexLocker lock(&mutexSkeleton_);
        foreach(KinectSkeleton *skeleton, skeletons_)
        {
            if (!skeleton || !skeleton->IsTracking())
                continue;

            // Support different colors for 6 unique skeletons
            // that is the maximum for the Kinect.
            switch(skeleton->EnrollmentIndex())
            {
                case 0: 
                    brush.setColor(Qt::blue);
                    break;
                case 1: 
                    brush.setColor(Qt::red);
                    break;
                case 2: 
                    brush.setColor(Qt::green);
                    break;
                case 3: 
                    brush.setColor(Qt::yellow);
                    break;
                case 4: 
                    brush.setColor(Qt::magenta);
                    break;
                case 5: 
                    brush.setColor(Qt::cyan);
                    break;
                default:
                    brush.setColor(Qt::white);
                    break;
            }
            pen.setColor(brush.color());
            pen.setWidth(3);
            p.setPen(pen);

            // Draw bone lines
            BoneDataMap boneData = skeleton->BoneData();
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-head", "bone-shoulder-center");
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-shoulder-center", "bone-spine");
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-hip-center", "bone-spine");
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-shoulder-left", "bone-shoulder-center");
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-shoulder-right", "bone-shoulder-center");
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-hip-center", "bone-hip-left");
            ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-hip-center", "bone-hip-right");

            foreach(QString side, drawBoneSides_)
                for(int i=0; i < drawBonePairs_.length(); ++i)
                    ConnectBones(&p, imgRect, boneRenderingMode, boneData, "bone-" + QString(drawBonePairs_[i].first + side).toStdString(), 
                        "bone-" + QString(drawBonePairs_[i].second + side).toStdString());
            
            p.setBrush(brush);
            p.setPen(Qt::transparent);

            for(BoneDataMap::const_iterator iter = boneData.begin(); iter != boneData.end(); ++iter)
            {
                const BoneDataPair &dataPair = (*iter).second;
                if (boneRenderingMode == 0 && dataPair.first != NUI_SKELETON_POSITION_TRACKED)
                    continue;
                else if (boneRenderingMode == 1 && dataPair.first == NUI_SKELETON_POSITION_NOT_TRACKED)
                    continue;

                float4 vec = dataPair.second * 200;
                QPointF pos(vec.x, vec.y);

                // Make the head a bit bigger
                if ((*iter).first == "bone-head")
                    p.drawEllipse(imgRect.center() + pos, 20, 23);
                else
                    p.drawEllipse(imgRect.center() + pos, 5, 5);
            }
        }
    }

    p.end();

    // Flip image and draw to the widget
    kinectUi_.widgetSkeletons->setPixmap(QPixmap::fromImage(skeletonImage.mirrored(false, true)));
}

RGBQUAD KinectModule::ConvertDepthShortToQuad(USHORT s)
{
    USHORT RealDepth = (s & 0xfff8) >> 3;
    USHORT Player = s & 7;

    // transform 13-bit depth information into an 8-bit intensity appropriate
    // for display (we disregard information in most significant bit)
    BYTE l = 255 - (BYTE)(256*RealDepth/0x0fff);

    RGBQUAD q;
    q.rgbRed = q.rgbBlue = q.rgbGreen = 0;

    switch( Player )
    {
        case 0:
        {
            q.rgbRed = l / 2;
            q.rgbBlue = l / 2;
            q.rgbGreen = l / 2;
            break;
        }
        case 1:
        {
            q.rgbRed = l;
            break;
        }
        case 2:
        {
            q.rgbGreen = l;
            break;
        }
        case 3:
        {
            q.rgbRed = l / 4;
            q.rgbGreen = l;
            q.rgbBlue = l;
            break;
        }
        case 4:
        {
            q.rgbRed = l;
            q.rgbGreen = l;
            q.rgbBlue = l / 4;
            break;
        }
        case 5:
        {
            q.rgbRed = l;
            q.rgbGreen = l / 4;
            q.rgbBlue = l;
            break;
        }
        case 6:
        {
            q.rgbRed = l / 2;
            q.rgbGreen = l / 2;
            q.rgbBlue = l;
            break;
        }
        case 7:
        {    
            q.rgbRed = 255 - ( l / 2 );
            q.rgbGreen = 255 - ( l / 2 );
            q.rgbBlue = 255 - ( l / 2 );
            break;
        }
    }

    return q;
}

void KinectModule::ConnectBones(QPainter *p, const QRectF &pRect, const int &boneRenderingMode, BoneDataMap &boneData, const std::string &boneName1, const std::string &boneName2)
{
    BoneDataMap::iterator iter1 = boneData.find(boneName1);
    if (iter1 == boneData.end())
        return;
    if (boneRenderingMode == 0 && (*iter1).second.first != NUI_SKELETON_POSITION_TRACKED)
        return;
    else if (boneRenderingMode == 1 && (*iter1).second.first == NUI_SKELETON_POSITION_NOT_TRACKED)
        return;

    BoneDataMap::iterator iter2 = boneData.find(boneName2);
    if (iter2 == boneData.end())
        return;
    if (boneRenderingMode == 0 && (*iter2).second.first != NUI_SKELETON_POSITION_TRACKED)
        return;
    else if (boneRenderingMode == 1 && (*iter2).second.first == NUI_SKELETON_POSITION_NOT_TRACKED)
        return;

    float4 vec1 = (*iter1).second.second * 200;
    QPointF pos1 = pRect.center() + QPointF(vec1.x, vec1.y);

    float4 vec2 = (*iter2).second.second * 200;
    QPointF pos2 = pRect.center() + QPointF(vec2.x, vec2.y);

    p->drawLine(pos1, pos2);
}

void KinectModule::OnScriptEngineCreated(QScriptEngine *engine)
{
    qScriptRegisterQObjectMetaType<KinectDevice*>(engine);
    qScriptRegisterQObjectMetaType<KinectSkeleton*>(engine);
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
