// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "KinectAPI.h"
#include "KinectFwd.h"

#include "Win.h"
#include <NuiApi.h>

#include "ui_KinectWidget.h"
#include "Math/float4.h"

#include <QObject>
#include <QMutex>
#include <QList>
#include <QSize>
#include <QPainter>
#include <QRectF>
#include <QImage>
#include <QScriptEngine>

/** KinectModule reads Kinect with Microsoft Kinect SDK. A dynamic object 'kinect' 
    will emit the data with signals that it gathers from the kinect device. 
    In addition to that you can acquire the KinectDevice from the Kinect function in this module.
 
    See KinectDevice documentation for the specific signals and instructions 
    how to acquire KinectDevice in C++ and JavaScript.

    KinectModule implements reading the Kinect device in a separate thread
    and uses Qt signals and slots to send that data to the main thread. Main
    thread makes KinectDevice emit notification signals that new data is available.
    3rd party code can listen to these signals and fetch the data with designated
    getters if they are interested in the data.
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

public slots:
    /// Returns the KinectDevice for Kinect data and interaction.
    KinectDevice* Kinect();

/// @cond PRIVATE
    /// Do not call these from outside KinectModule. Use KinectDevice instead from the Kinect function.
    bool HasKinect();
    bool IsStarted();
    bool StartKinect();
    void StopKinect();
    QImage VideoImage();
    QImage DepthImage();

private slots:
    /// Callback from Kinect processing thread.
    /// \note Do not call this from outside KinectModule.
    void OnVideoReady();

    /// Callback from Kinect processing thread.
    /// \note Do not call this from outside KinectModule.
    void OnDepthReady();

    /// Callback from Kinect processing thread.
    /// \note Do not call this from outside KinectModule.
    void OnSkeletonsReady();

    /// Callback from Kinect processing thread.
    /// \note Do not call this from outside KinectModule.
    void OnSkeletonTracking(bool tracking);

    /// Shows control panel widget.
    void OnShowControlPanel();

    /// Updates control panel ui state.
    void UpdateControlPanelState();
    
    /// Paints all the control panel rendering widgets black.
    void ResetControlPanelRendering();

    void OnScriptEngineCreated(QScriptEngine *engine);

signals:
    /// Signal that is emitted from the Kinect processing thread.
    /// \note Do not connect to this signal, use KinectDevice instead.
    void VideoAlert();

    /// Signal that is emitted from the Kinect processing thread.
    /// \note Do not connect to this signal, use KinectDevice instead.
    void DepthAlert();

    /// Signal that is emitted from the Kinect processing thread.
    /// \note Do not connect to this signal, use KinectDevice instead.
    void SkeletonsAlert();

    /// Signal that is emitted from the Kinect processing thread.
    /// \note Do not connect to this signal, use KinectDevice instead.
    void SkeletonTracking(bool tracking);

/// @endcond

private:
    /// Kinect processing thread that listens to the Kinect SDK events.
    static DWORD WINAPI KinectProcessThread(LPVOID pParam);

    /// Callback for when video is available.
    /// \note We are inside the Kinect processing thread when this function is called. Do not call this outside KinectModule.
    void GetVideo();
    
    /// Callback for when depth is available.
    /// \note We are inside the Kinect processing thread when this function is called. Do not call this outside KinectModule.
    void GetDepth();
    
    /// Callback for when skeleton is available.
    /// \note We are inside the Kinect processing thread when this function is called. Do not call this outside KinectModule.
    void GetSkeleton();

    /// Debug draw skeleton data (if available) into a QWidget.
    /// \note This function does nothing if debugging_ is false.
    void DrawSkeletons();

    /// Convert depth data to a rgb pixel data.
    /// @param USHORT pixel to convert.
    /// @return RGBQUAD converted pixel.
    RGBQUAD ConvertDepthShortToQuad(USHORT s);

    /// Connect two bones with a line in the given painter.
    /// @param QPainter* Painter to draw the bone line.
    /// @param QRectF Rect of the drawing surface.
    /// @param float4 Bone 1 position.
    /// @param float4 Bone 2 position.
    void ConnectBones(QPainter *p, QRectF pRect, float4 pos1, float4 pos2);

    QString LC;

    KinectDevice *kinectDevice_;
    
    QWidget *kinectWidget_;
    Ui::KinectWidget kinectUi_;

    QSize videoSize_;
    QSize depthSize_;

    QMutex mutexVideo_;
    QMutex mutexDepth_;
    QMutex mutexSkeleton_;

    QImage video_;
    QImage depth_;
    QList<KinectSkeleton*> skeletons_;

    INuiSensor *kinectSensor_;
    BSTR kinectName_;

    HANDLE kinectProcess_;
    HANDLE kinectDepthStream_;
    HANDLE kinectVideoStream_;

    HANDLE eventKinectProcessStop_;
    HANDLE eventDepthFrame_;
    HANDLE eventVideoFrame_;
    HANDLE eventSkeletonFrame_;

    QStringList drawBoneSides_;
    QList<QPair<QString, QString> > drawBonePairs_;
};
