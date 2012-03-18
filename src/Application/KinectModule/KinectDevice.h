// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "KinectAPI.h"
#include "KinectFwd.h"

#include <QObject>
#include <QString>
#include <QImage>

/*! KinectDevice is the way to get data from a Microsoft Kinect device.
    The KinectModule reads all available data from the device and emits the Tundra/Qt style data with this device.

    You can acquire the Kinect device via Frameworks dynamic object with name "kinect" and connecting to the signals specified in this class:
    VideoUpdate, DepthUpdate, SkeletonUpdate and TrackingSkeletons.

    \code
    C++:
        // In C++ framework properties are a bit inconvenient, but available non the less.
        QVariant kinectVariant = GetFramework()->property("kinect");
        if (kinectVariant.isNull())
        {
            LogWarning("Kinect module not present, oops!");
            return;
        }
        if (kinectVariant.canConvert<QObject*>())
        {
            QObject *kinectPtr = kinectVariant.value<QObject*>();
            if (kinectPtr)
            {
                QObject::connect(kinectPtr, SIGNAL(VideoUpdate(const QImage)), this, SLOT(OnKinectVideo(const QImage)));
                QObject::connect(kinectPtr, SIGNAL(SkeletonUpdate(const QVariantMap)), this, SLOT(OnKinectSkeleton(const QVariantMap)));
                QObject::connect(kinectPtr, SIGNAL(TrackingSkeletons(bool)), this, SLOT(OnKinectTrackingSkeleton(bool)));
            }
        }

    JavaScript:
        // All framework dynamic objects are automatically visible in JavaScript.
        if (kinect == null) {
            print("Kinect module not present, oops!");
            return;
        }
        var isTrackingRightNow = kinect.IsTrackingSkeletons();
        kinect.VideoUpdate.connect(onKinectVideo);
        kinect.SkeletonUpdate.connect(onKinectSkeleton);
        kinect.TrackingSkeletons.connect(onKinectTrackingSkeleton);

    Python:
        The KinectDevice class needs to be registered to PythonQt so the QObject is knows and can be used.
        This is not done yet so python access is not working yet.
    \endcode
 */
class KINECT_VOIP_MODULE_API KinectDevice : public QObject
{

Q_OBJECT

friend class KinectModule;

public:
    KinectDevice(KinectModule *owner);
    ~KinectDevice();

public slots:
    /// Get current skeleton tracking state.
    /// @return bool True if currently tracking and emitting SkeletonUpdate signal, false if we lost the skeleton(s).
    bool IsTrackingSkeletons();

    /// Return the last video image received from Kinect. Call this when you receive the VideoUpdate signal.
    QImage VideoImage();

    /// Return the last depth image received from Kinect. Call this when you receive the DepthUpdate signal.
    QImage DepthImage();

signals:
    /// Emitted when a new video image is received from Kinect.
    /// Call VideoImage function to get the image data if you are interested in it.
    void VideoUpdate();
    
    /// Emitted when a new depth image is received from Kinect.
    /// Call DepthImage function to get the image data if you are interested in it.
    void DepthUpdate();

    /// Emitted when a skeletons tracking state changes. Connect to the skeletons signals here if the tracking is true.
    /** \note Remember to track what skeleton ids you have already connected, this will fire multiple times with same skeleton!
        Or disconnect always when tracking is false. */
    void SkeletonStateChanged(bool tracking, KinectSkeleton *skeleton);

    /// Emitted when skeleton tracking is started or stopped. This can be handy for eg. reseting things when we lose skeleton tracking.
    /// @param bool True if currently tracking and emitting SkeletonUpdate signal, false if we lost the skeleton(s).
    void TrackingSkeletons(bool tracking);

protected:
    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitVideoUpdate();

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitDepthUpdate();

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitSkeletonUpdate(KinectSkeleton *skeleton);

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void SetTrackingSkeletons(bool tracking);

private:
    KinectModule *owner_;

    bool isTrackingSkeletons_;
};
