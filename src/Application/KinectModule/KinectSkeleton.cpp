// For conditions of distribution and use, see copyright notice in LICENSE

#include "KinectSkeleton.h"
#include "LoggingFunctions.h"

#include <QMutexLocker>
#include <QDebug>

KinectSkeleton::KinectSkeleton() :
    QObject(),
    tracking_(false),
    updated(false),
    emitTrackingChange(false),
    trackingId_(0),
    enrollmentIndex_(0),
    userIndex_(0),
    position_(float4::zero)
{
    for (int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i)
    {
        std::string boneName = BoneIndexToName(i).toStdString();
        if (boneName.empty())
        {
            LogError("[KinectSkeleton]: Error initializing skeleton bone data, invalid bone index " + QString::number(i));
            continue;
        }
        boneNames_.push_back(boneName);
        boneData_[boneName] = std::make_pair(NUI_SKELETON_POSITION_NOT_TRACKED, float4::zero);
    }
}

KinectSkeleton::~KinectSkeleton()
{
    QMutexLocker lock(&mutexData_);
    boneData_.clear();
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
    QStringList qBoneNames;
    {
        QMutexLocker lock(&mutexData_);
        for(int i=0; i<boneNames_.size(); ++i)
            qBoneNames << QString::fromStdString(boneNames_[i]);
    }
    return qBoneNames;
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

BoneDataMap KinectSkeleton::BoneData()
{
    QMutexLocker lock(&mutexData_);
    return boneData_;
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
        BoneDataPair &dataPair = boneData_[boneNames_[i]];
        dataPair.first = skeletonData.eSkeletonPositionTrackingState[i];

        if (dataPair.first != NUI_SKELETON_POSITION_NOT_TRACKED)
        {
            dataPair.second.x = skeletonData.SkeletonPositions[i].x;
            dataPair.second.y = skeletonData.SkeletonPositions[i].y;
            dataPair.second.z = skeletonData.SkeletonPositions[i].z;
            dataPair.second.w = skeletonData.SkeletonPositions[i].w;
        }
        else
            dataPair.second = float4::zero;
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
            BoneDataPair &dataPair = boneData_[boneNames_[i]];
            dataPair.first = NUI_SKELETON_POSITION_NOT_TRACKED;
            dataPair.second = float4::zero;
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

bool KinectSkeleton::IsBoneTracked(const QString &boneName)
{
    if (boneName.isEmpty())
        return false;

    QMutexLocker lock(&mutexData_);
    BoneDataMap::const_iterator iter = boneData_.find(boneName.toStdString());
    if (iter == boneData_.end())
    {
        LogError("[KinectSkeleton]: IsBoneTracked() Bone with name " + boneName + " does not exist!");
        return false;
    }
    return ((*iter).second.first != NUI_SKELETON_POSITION_NOT_TRACKED);
}

bool KinectSkeleton::IsBoneTracked(int boneIndex)
{
    QString boneName = BoneIndexToName(boneIndex);
    if (boneName.isEmpty())
    {
        LogError("[KinectSkeleton]: IsBoneTracked() Bone with index " + QString::number(boneIndex) + " does not exist!");
        return false;
    }
    return IsBoneTracked(boneName);
}

bool KinectSkeleton::IsBoneInferred(const QString &boneName)
{
    QMutexLocker lock(&mutexData_);
    BoneDataMap::const_iterator iter = boneData_.find(boneName.toStdString());
    if (iter == boneData_.end())
    {
        LogError("[KinectSkeleton]: IsBoneTracked() Bone with name " + boneName + " does not exist!");
        return false;
    }
    return ((*iter).second.first == NUI_SKELETON_POSITION_INFERRED);
}

bool KinectSkeleton::IsBoneInferred(int boneIndex)
{
    QString boneName = BoneIndexToName(boneIndex);
    if (boneName.isEmpty())
    {
        LogError("[KinectSkeleton]: IsBoneInferred() Bone with index " + QString::number(boneIndex) + " does not exist!");
        return false;
    }
    return IsBoneInferred(boneName);
}

float4 KinectSkeleton::BonePosition(const QString &boneName)
{
    QMutexLocker lock(&mutexData_);
    BoneDataMap::const_iterator iter = boneData_.find(boneName.toStdString());
    if (iter == boneData_.end())
    {
        LogError("[KinectSkeleton]: BonePosition() Bone with name " + boneName + " does not exist!");
        return float4::zero;
    }
    return (*iter).second.second;
}

float4 KinectSkeleton::BonePosition(int boneIndex)
{
    QString boneName = BoneIndexToName(boneIndex);
    if (boneName.isEmpty())
    {
        LogError("[KinectSkeleton]: BonePosition() Bone with index " + QString::number(boneIndex) + " does not exist!");
        return float4::zero;
    }
    return BonePosition(BoneIndexToName(boneIndex));
}

QString KinectSkeleton::BoneIndexToName(int boneIndex)
{
    QString base = "bone-";
    switch (boneIndex)
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
            return "";
    }
}
