// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "KinectAPI.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QMutex>

#include "Win.h"
#include <NuiApi.h>

#include "Math/float3.h"
#include "Math/float4.h"

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

    QHash<QString, float4> BonePositions();

    // Called from the Kinect processing thread.
    void UpdateSkeleton(const NUI_SKELETON_DATA &skeletonData);

    // Called from the Kinect processing thread.
    void ResetSkeleton();

    // Must be called in the main thread!
    void EmitUpdated();

    // Must be called in the main thread!
    void EmitTrackingChanged();

public slots:
    float4 BonePosition(QString boneName);
    float4 BonePosition(int index);

    /// \note This function will return "unknown-<index>" if index is invalid.
    QString BoneIndexToName(int index);

signals:
    /// This signal is emitted when the tracking state of this skeleton changes.
    void TrackingChanged(bool tracking);

    // This signal is emitted when skeleton was updated. You can use BonePosition functions to request data.
    void Updated();

private:
    bool tracking_;
    QStringList boneNames_;

    uint trackingId_;
    uint enrollmentIndex_;
    uint userIndex_;

    float4 position_;

    QMutex mutexData_;

    QHash<QString, float4> bonePositions_;
};
