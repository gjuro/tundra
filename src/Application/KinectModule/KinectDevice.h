// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "KinectAPI.h"
#include "KinectFwd.h"

#include <QObject>
#include <QString>
#include <QImage>

/** KinectDevice is the way to get data from a connected Microsoft Kinect device on the machine.
    
    \section KinectDeviceFeatures KinectDevice features

    In assistance with KinectModule KinectModule reads and processes the Kinect device data in a separate thread.
    Once the data hits the main thread this KinectDevice emits notification signals VideoUpdate, DepthUpdate,
    SkeletonStateChanged and TrackingSkeletons. If you are interested in the particular signals data you can 
    retrieve it with the slots VideoImage, DepthImage and the slots in the KinectSkeleton class.

    Also see these slots HasKinect, IsStarted, StartKinect and StopKinect.

    Not emitting the data with the signals is design decision that was made keeping and eye for Tundra scripting.
    These signals will fire often and we want to avoid resolving and casting any data in the signals as it can
    get fairly heavy, depending on the data. In our case it would be QImages and KinectSkeleton class with high FPS.
    It is better to have designated slots to only get the data when your script is interested in it.

    \section KinectDeviceUsage Acquiring and using KinectDevice

    You can acquire the Kinect device via Frameworks dynamic object with name "kinect".
    This object is auto exposed to JavaScript as kinect. It is not recommended to get the 
    KinectModule directly and interact with it even if it is a QObject, it is for internal use only.

    \code
    JavaScript:
        var debugLabel = new QLabel();
        // All framework dynamic objects are automatically visible in JavaScript.
        if (kinect == null) 
        {
            console.LogWarning("Kinect module not present, oops!");
            return;
        }
        if (kinect.HasKinect())
        {
            kinect.VideoUpdate.connect(OnKinectVideoUpdate);
            kinect.DepthUpdate.connect(OnKinectDepthUpdate);
            if (!kinect->StartKinect())
                console.LogError("Failed to start Kinect device");
            else
                debugLabel.visible = true;
        }
        else
            console.LogError("No Kinect device available on the machine!");


        function OnKinectVideoUpdate()
        {
            debugLabel.pixmap = QPixmap.fromImage(kinect.VideoImage());
        }

    C++:
        KinectModule *kinectModule = GetFramework()->GetModule<KinectModule*>().get();
        if (!kinectModule)
        {
            LogWarning("Kinect module not present, oops!");
            return;
        }
        KinectDevice *kinect = kinectModule->Kinect();
        if (kinect->HasKinect())
        {
            connect(kinect, SIGNAL(VideoUpdate()), this, SLOT(OnKinectVideoUpdate()));
            connect(kinect, SIGNAL(DepthUpdate()), this, SLOT(OnKinectDepthUpdate()));
            if (!kinect->StartKinect())
                LogError("Failed to start Kinect device");
        }
        else
            LogError("No Kinect device available on the machine!");
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
    /// Returns if a Kinect device is available.
    bool HasKinect();

    /// Returns if Kinect has been started with StartKinect function.
    bool IsStarted();

    /// Starts Kinect if not running.
    bool StartKinect();

    /// Stops Kinect if running.
    void StopKinect();

    /// Get current skeleton tracking state.
    /** @return bool True if currently tracking any skeletons. Use SkeletonStateChanged 
        and its KinectSkeleton parameter to hook to its signals */
    bool IsTrackingSkeletons();

    /// Return the last video image received from Kinect. Call this when you receive the VideoUpdate signal.
    QImage VideoImage();

    /// Return the last depth image received from Kinect. Call this when you receive the DepthUpdate signal.
    QImage DepthImage();

    /// Return the maximum bone count for a skeleton. Treat this as for(i=0;i<BoneCount();++i).
    int BoneCount();

signals:
    /// Emitted when a new video image is received from Kinect.
    /// Call VideoImage function to get the image data if you are interested in it.
    void VideoUpdate();
    
    /// Emitted when a new depth image is received from Kinect.
    /// Call DepthImage function to get the image data if you are interested in it.
    void DepthUpdate();

    /// Emitted when skeleton received an update.
    void SkeletonUpdate(KinectSkeleton *skeleton);

    /// Emitted when a skeletons tracking state changes. Connect to the skeletons signals here if the tracking is true.
    /** \note Remember to track what skeleton ids you have already connected, this will fire multiple times with same skeleton!
        Or disconnect always when tracking is false. */
    void SkeletonStateChanged(bool tracking, KinectSkeleton *skeleton);

    /// Emitted when skeleton tracking is started or stopped. This can be handy for eg. reseting things when we lose skeleton tracking.
    /// @param bool True if currently tracking and emitting SkeletonUpdate signal, false if we lost the skeleton(s).
    void TrackingSkeletons(bool tracking);

/// @cond PRIVATE

protected:
    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitVideoUpdate();

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitDepthUpdate();

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitSkeletonUpdate(KinectSkeleton *skeleton);

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void EmitSkeletonStateChanged(KinectSkeleton *skeleton);

    /// This function gets called by KinectModule when its outside the Kinect processing thread, do not call this from 3rd party code!
    void SetTrackingSkeletons(bool tracking);

/// @endcond

private:
    KinectModule *owner_;

    bool isTrackingSkeletons_;
};
