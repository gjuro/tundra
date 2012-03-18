// For conditions of distribution and use, see copyright notice in LICENSE

#include "KinectSkeleton.h"
#include "LoggingFunctions.h"

KinectSkeleton::KinectSkeleton() :
    QObject(),
    tracking_(false),
    updated_(false),
    emitTrackingChange_(false),
    trackingId_(0),
    enrollmentIndex_(0),
    userIndex_(0),
    position_(float4(0,0,0,0))
{
    for (int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i)
    {
        QString boneName = BoneIndexToName(i);
        boneNames_.append(boneName);
        data_[boneName] = float4(0,0,0,0);
    }
}

KinectSkeleton::~KinectSkeleton()
{
    data_.clear();
    boneNames_.clear();
}

bool KinectSkeleton::IsTracking()
{
    return tracking_;
}

QStringList KinectSkeleton::BoneNames()
{
    return boneNames_;
}

uint KinectSkeleton::TrackingId()
{
    return trackingId_;
}

uint KinectSkeleton::EnrollmentIndex()
{
    return enrollmentIndex_;
}

uint KinectSkeleton::UserIndex()
{
    return userIndex_;
}

float4 KinectSkeleton::Position()
{
    return position_;
}

void KinectSkeleton::UpdateSkeleton(const NUI_SKELETON_DATA &skeletonData)
{
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
        float4 &pos = data_[boneNames_[i]];

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

    if (!updated_)
        updated_ = true;

    if (!tracking_)
    {
        tracking_ = true;
        emitTrackingChange_ = true;
    }
}

void KinectSkeleton::ResetSkeleton()
{
    if (updated_)
        updated_ = false;

    if (tracking_)
    {
        tracking_ = false;
        emitTrackingChange_ = true;

        for (int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i)
        {
            float4 &pos = data_[BoneIndexToName(i)];
            pos.x = 0;
            pos.y = 0;
            pos.z = 0;
            pos.w = 0;
        }
    }
}

void KinectSkeleton::EmitIfUpdated()
{
    if (updated_)
    {
        updated_ = false;
        emit Updated();
    }
}

void KinectSkeleton::EmitIfTrackingChanged()
{
    if (emitTrackingChange_)
    {
        emitTrackingChange_ = false;
        emit TrackingChanged(tracking_);
    }
}

float4 KinectSkeleton::BonePosition(QString boneName)
{
    if (!data_.contains(boneName))
    {
        LogError("[KinectSkeleton]: BonePosition() Bone with name " + boneName + " does not exist!");
        return float4(0,0,0,0);
    }
    return data_.value(boneName);
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
