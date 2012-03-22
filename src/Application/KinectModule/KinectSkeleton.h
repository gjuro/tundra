// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "KinectAPI.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QMutex>

#include <map>
#include <vector>
#include <utility>

#include "Win.h"
#include <NuiApi.h>

#include "Math/float3.h"
#include "Math/float4.h"

typedef std::pair<NUI_SKELETON_POSITION_TRACKING_STATE, float4> BoneDataPair;
typedef std::map<std::string, BoneDataPair > BoneDataMap;

/** KinectSkeleton is accessed by both the Kinect processing thread and the main thread
    so its data getters and setters are protected with mutexes. */
class KINECT_VOIP_MODULE_API KinectSkeleton : public QObject
{

Q_OBJECT

/// True if tracking and this object has skeleton positions etc.
Q_PROPERTY(bool tracking READ IsTracking)

/// True is skeleton has been updated. This is true when Updated signal fires and changes to false after it.
Q_PROPERTY(bool updated READ IsUpdated)

/// All valid bone names. You can use these to fetch bone float4 object.
Q_PROPERTY(QStringList boneNames READ BoneNames)

/// Skeleton tracking id.
Q_PROPERTY(uint trackingId READ TrackingId)

/// Skeleton enrollment index.
Q_PROPERTY(uint enrollmentIndex READ EnrollmentIndex)

/// User index.
Q_PROPERTY(uint userIndex READ UserIndex)

/// Main position of the skeleton.
Q_PROPERTY(float4 position READ Position)

public:
    KinectSkeleton();
    ~KinectSkeleton();

    bool emitTrackingChange;
    bool updated;

    bool IsTracking();
    bool IsUpdated();
    QStringList BoneNames();

    uint TrackingId();
    uint EnrollmentIndex();
    uint UserIndex();

    float4 Position();

    BoneDataMap BoneData();

    // Called from the Kinect processing thread.
    void UpdateSkeleton(const NUI_SKELETON_DATA &skeletonData);

    // Called from the Kinect processing thread.
    void ResetSkeleton();

    // Must be called in the main thread!
    void EmitUpdated();

    // Must be called in the main thread!
    void EmitTrackingChanged();

public slots:
    /// Get if bone is tracked with name.
    bool IsBoneTracked(const QString &boneName);
    
    /// Get if bone is tracked with index.
    bool IsBoneTracked(int boneIndex);

    /// Get if bone position is inferred with name.
    /** Inferred means that the position was calculated but 
        it has no real time position from the sensor. */
    bool IsBoneInferred(const QString &boneName);
    
    /// Get if bone position is inferred with index.
    /** Inferred means that the position was calculated but 
        it has no real time position from the sensor. */
    bool IsBoneInferred(int boneIndex);

    /// Get bone position with name.
    /** \see BonePosition(int boneIndex) IsBoneTracked, IsBoneInferred */
    float4 BonePosition(const QString &boneName);

    /// Get bone position with index.
    /** \see BonePosition(QString boneName) IsBoneTracked, IsBoneInferred */
    float4 BonePosition(int boneIndex);

    /// Returns bone name for a index. Return empty string if not valid bone index.
    QString BoneIndexToName(int boneIndex);

signals:
    /// This signal is emitted when the tracking state of this skeleton changes.
    void TrackingChanged(bool tracking);

    // This signal is emitted when skeleton was updated. You can use BonePosition functions to request data.
    void Updated();

private:
    bool tracking_;

    uint trackingId_;
    uint enrollmentIndex_;
    uint userIndex_;

    float4 position_;

    QMutex mutexData_;

    std::vector<std::string> boneNames_;
    BoneDataMap boneData_;
};
