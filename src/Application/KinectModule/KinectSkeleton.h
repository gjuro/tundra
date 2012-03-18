// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>

#include "Win.h"
#include <NuiApi.h>

#include "Math/float3.h"
#include "Math/float4.h"

class KinectSkeleton : public QObject
{

Q_OBJECT

/// This property is true if tracking and this object has skeleton positions etc.
Q_PROPERTY(bool tracking READ IsTracking)

/// All valid bone names. You can use these to fetch bone float4 object.
Q_PROPERTY(QStringList boneNames READ BoneNames)

/// Various id information about the skeleton.
Q_PROPERTY(uint trackingId READ TrackingId)
Q_PROPERTY(uint enrollmentIndex READ EnrollmentIndex)
Q_PROPERTY(uint userIndex READ UserIndex)

/// Main position of the skeleton.
Q_PROPERTY(float4 position READ Position)

public:
    KinectSkeleton();
    ~KinectSkeleton();

    bool emitTrackingChange_;
    bool updated_;

    bool IsTracking();
    QStringList BoneNames();

    uint TrackingId();
    uint EnrollmentIndex();
    uint UserIndex();

    float4 Position();

    // Called from the Kinect processing thread.
    void UpdateSkeleton(const NUI_SKELETON_DATA &skeletonData);

    // Called from the Kinect processing thread.
    void ResetSkeleton();

    // Must be called in the main thread!
    void EmitIfUpdated();

    // Must be called in the main thread!
    void EmitIfTrackingChanged();

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

    QHash<QString, float4> data_;
};
