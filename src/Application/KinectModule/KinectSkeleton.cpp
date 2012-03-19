// For conditions of distribution and use, see copyright notice in LICENSE

#include "KinectSkeleton.h"
#include "LoggingFunctions.h"

#include <QMutexLocker>

KinectSkeleton::KinectSkeleton() :
    QObject(),
    tracking_(false),
    updated(false),
    emitTrackingChange(false),
    trackingId_(0),
    enrollmentIndex_(0),
    userIndex_(0),
    position_(float4(0,0,0,0))
{
    for (int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i)
    {
        QString boneName = BoneIndexToName(i);
        boneNames_.append(boneName);
        bonePositions_[boneName] = float4(0,0,0,0);
    }
}

KinectSkeleton::~KinectSkeleton()
{
    bonePositions_.clear();
    boneNames_.clear();
}

bool KinectSkeleton::IsTracking()
{
    QMutexLocker lock(&mutexData_);
    return tracking_;
}

bool KinectSkeleton::IsUpdated()
{
    QMutexLocker lock(&mutexData_);
    return updated;
}

QStringList KinectSkeleton::BoneNames()
{
    QMutexLocker lock(&mutexData_);
    return boneNames_;
}

uint KinectSkeleton::TrackingId()
{
    QMutexLocker lock(&mutexData_);
    return trackingId_;
}

uint KinectSkeleton::EnrollmentIndex()
{
    QMutexLocker lock(&mutexData_);
    return enrollmentIndex_;
}

uint KinectSkeleton::UserIndex()
{
    QMutexLocker lock(&mutexData_);
    return userIndex_;
}

float4 KinectSkeleton::Position()
{
    QMutexLocker lock(&mutexData_);
    return position_;
}

QHash<QString, float4> KinectSkeleton::BonePositions()
{
    QMutexLocker lock(&mutexData_);
    return bonePositions_;
}

void KinectSkeleton::UpdateSkeleton(const NUI_SKELETON_DATA &skeletonData)
{
    QMutexLocker lock(&mutexData_);

    if (trackingId_ != (unsigned int)skeletonData.dwTrackingID)
        trackingId_ = (unsigned int)skeletonData.dwTrackingID;
    if (enrollmentIndex_ != (unsigned int)skeletonData.dwEnrollmentIndex)
        enrollmentIndex_ = (unsigned int)skeletonData.dwEnrollmentIndex;
    if (userIndex_ != (unsigned int)skeletonData.dwUserIndex)
        userIndex_ = (unsigned int)skeletonData.dwUserIndex;

    position_.x = skeletonData.Position.x;
    position_.y = skeletonData.Position.y;
    position_.z = skeletonData.Position.z;
    position_.w = skeletonData.Position.w;

    for (int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i)
    {
        // Don't use BoneIndexToName(i) here as it creates a 
        // QString every single time. Use faster lookup from boneNames_.
        float4 &pos = bonePositions_[boneNames_[i]];

        if (skeletonData.eSkeletonPositionTrackingState[i] != NUI_SKELETON_POSITION_NOT_TRACKED)
        {
            pos.x = skeletonData.SkeletonPositions[i].x;
            pos.y = skeletonData.SkeletonPositions[i].y;
            pos.z = skeletonData.SkeletonPositions[i].z;
            pos.w = skeletonData.SkeletonPositions[i].w;
        }
        else
        {
            pos.x = 0;
            pos.y = 0;
            pos.z = 0;
            pos.w = 0;
        }
    }

    if (!updated)
        updated = true;

    if (!tracking_)
    {
        tracking_ = true;
        emitTrackingChange = true;
    }
}

void KinectSkeleton::ResetSkeleton()
{
    QMutexLocker lock(&mutexData_);

    if (updated)
        updated = false;

    if (tracking_)
    {
        tracking_ = false;
        emitTrackingChange = true;

        for (int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i)
        {
            float4 &pos = bonePositions_[BoneIndexToName(i)];
            pos.x = 0;
            pos.y = 0;
            pos.z = 0;
            pos.w = 0;
        }
    }
}

void KinectSkeleton::EmitUpdated()
{
    QMutexLocker lock(&mutexData_);

    if (updated)
    {
        emit Updated();
        updated = false;
    }
}

void KinectSkeleton::EmitTrackingChanged()
{
    QMutexLocker lock(&mutexData_);

    if (emitTrackingChange)
    {
        emit TrackingChanged(tracking_);
        emitTrackingChange = false;
    }
}

float4 KinectSkeleton::BonePosition(QString boneName)
{
    QMutexLocker lock(&mutexData_);

    if (!bonePositions_.contains(boneName))
    {
        LogError("[KinectSkeleton]: BonePosition() Bone with name " + boneName + " does not exist!");
        return float4(0,0,0,0);
    }
    return bonePositions_.value(boneName);
}

float4 KinectSkeleton::BonePosition(int index)
{
    return BonePosition(BoneIndexToName(index));
}

QString KinectSkeleton::BoneIndexToName(int index)
{
    QString base = "bone-";
    switch (index)
    {
        case NUI_SKELETON_POSITION_HIP_CENTER:
            return base + "hip-center";
        case NUI_SKELETON_POSITION_SPINE:
            return base + "spine";
        case NUI_SKELETON_POSITION_SHOULDER_CENTER:
            return base + "shoulder-center";
        case NUI_SKELETON_POSITION_HEAD:
            return base + "head";
        case NUI_SKELETON_POSITION_SHOULDER_LEFT:
            return base + "shoulder-left";
        case NUI_SKELETON_POSITION_ELBOW_LEFT:
            return base + "elbow-left";
        case NUI_SKELETON_POSITION_WRIST_LEFT:
            return base + "wrist-left";
        case NUI_SKELETON_POSITION_HAND_LEFT:
            return base + "hand-left";
        case NUI_SKELETON_POSITION_SHOULDER_RIGHT:
            return base + "shoulder-right";
        case NUI_SKELETON_POSITION_ELBOW_RIGHT:
            return base + "elbow-right";
        case NUI_SKELETON_POSITION_WRIST_RIGHT:
            return base + "wrist-right";
        case NUI_SKELETON_POSITION_HAND_RIGHT:
            return base + "hand-right";
        case NUI_SKELETON_POSITION_HIP_LEFT:
            return base + "hip-left";
        case NUI_SKELETON_POSITION_KNEE_LEFT:
            return base + "knee-left";
        case NUI_SKELETON_POSITION_ANKLE_LEFT:
            return base + "ankle-left";
        case NUI_SKELETON_POSITION_FOOT_LEFT:
            return base + "foot-left";
        case NUI_SKELETON_POSITION_HIP_RIGHT:
            return base + "hip-right";
        case NUI_SKELETON_POSITION_KNEE_RIGHT:
            return base + "knee-right";
        case NUI_SKELETON_POSITION_ANKLE_RIGHT:
            return base + "ankle-right";
        case NUI_SKELETON_POSITION_FOOT_RIGHT:
            return base + "foot-right";
        default:
            return base + "unknown-" + QString::number(index);
    }
}
