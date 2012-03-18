// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "KinectAPI.h"
#include "KinectFwd.h"

#include "Win.h"
#include <NuiApi.h>
#include "KinectHelper.h"

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QList>
#include <QLabel>
#include <QImage>
#include <QSize>

/*! KinectModule reads Kinect with Microsoft Kinect SDK. A dynamic object 'kinect' 
    will emit the data with signals that it gathers from the kinect device.
 
    See KinectDevice for the specific signals and instructions how to acquire KinectDevice in C++, JavaScript and Python.
 */
class KINECT_VOIP_MODULE_API KinectModule : public IModule
{

Q_OBJECT

public:
    /// Constructor
    KinectModule();

    /// Deconstructor
    virtual ~KinectModule();

    /// IModule override
    void Initialize();

    /// IModule override
    void Uninitialize();

    bool HasKinect();
    bool IsStarted();

    bool StartKinect();
    void StopKinect();

    QImage VideoImage();
    QImage DepthImage();

private slots:
    /// Callback from Kinect processing thread.
    /// @note Do not call this from outside KinectModule.
    void OnVideoReady();

    /// Callback from Kinect processing thread.
    /// @note Do not call this from outside KinectModule.
    void OnDepthReady();

    /// Callback from Kinect processing thread.
    /// @note Do not call this from outside KinectModule.
    void OnSkeletonsReady();

    /// Callback from Kinect processing thread.
    /// @note Do not call this from outside KinectModule.
    void OnSkeletonTracking(bool tracking);

signals:
    /// Signal that is emitted from the Kinect processing thread.
    /// @note Do not connect to this signal, use KinectDevice instead.
    void VideoAlert();

    /// Signal that is emitted from the Kinect processing thread.
    /// @note Do not connect to this signal, use KinectDevice instead.
    void DepthAlert();

    /// Signal that is emitted from the Kinect processing thread.
    /// @note Do not connect to this signal, use KinectDevice instead.
    void SkeletonsAlert();

    /// Signal that is emitted from the Kinect processing thread.
    /// @note Do not connect to this signal, use KinectDevice instead.
    void SkeletonTracking(bool tracking);

private:
    /// Kinect processing thread that listens to the Kinect SDK events.
    static DWORD WINAPI KinectProcessThread(LPVOID pParam);

    /// Callback for when video is available.
    /// @note We are inside the Kinect processing thread when this function is called. Do not call this outside KinectModule.
    void GetVideo();
    
    /// Callback for when depth is available.
    /// @note We are inside the Kinect processing thread when this function is called. Do not call this outside KinectModule.
    void GetDepth();
    
    /// Callback for when skeleton is available.
    /// @note We are inside the Kinect processing thread when this function is called. Do not call this outside KinectModule.
    void GetSkeleton();

    /// Debug draw skeleton data (if available) into a QWidget.
    /// @note This function does nothing if debugging_ is false.
    void DrawDebugSkeletons();

    HANDLE kinectProcess_;
    HANDLE kinectDepthStream_;
    HANDLE kinectVideoStream_;

    HANDLE eventKinectProcessStop_;
    HANDLE eventDepthFrame_;
    HANDLE eventVideoFrame_;
    HANDLE eventSkeletonFrame_;

    INuiSensor *kinectSensor_;
    BSTR kinectName_;

    KinectHelper helper_;
    KinectDevice *tundraKinectDevice_;

    QLabel *videoPreviewLabel_;
    QLabel *depthPreviewLabel_;
    QLabel *skeletonPreviewLabel_;

    QSize videoSize_;
    QSize depthSize_;

    QMutex mutexVideo_;
    QMutex mutexDepth_;
    QMutex mutexSkeleton_;

    QImage video_;
    QImage depth_;
    QList<KinectSkeleton*> skeletons_;

    /// Set this to true if you want to see debug widgets and prints.
    bool debugging_;

    QString LC;
};
