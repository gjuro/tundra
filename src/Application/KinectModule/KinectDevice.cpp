// For conditions of distribution and use, see copyright notice in LICENSE

#include "KinectDevice.h"
#include "KinectModule.h"
#include "KinectSkeleton.h"

KinectDevice::KinectDevice(KinectModule *owner) :
    owner_(owner),
    isTrackingSkeletons_(false)
{
}

KinectDevice::~KinectDevice()
{
    owner_ = 0;
}

void KinectDevice::EmitVideoUpdate()
{
    emit VideoUpdate();
}

void KinectDevice::EmitDepthUpdate()
{
    emit DepthUpdate();
}

void KinectDevice::EmitSkeletonUpdate(KinectSkeleton *skeleton)
{
    if (!skeleton)
        return;
    if (!isTrackingSkeletons_)
        SetTrackingSkeletons(true);

    emit SkeletonStateChanged(skeleton->IsTracking(), skeleton);
}

void KinectDevice::SetTrackingSkeletons(bool tracking)
{
    if (isTrackingSkeletons_ == tracking)
        return;

    isTrackingSkeletons_ = tracking;
    emit TrackingSkeletons(tracking);
}

bool KinectDevice::IsTrackingSkeletons()
{
    return isTrackingSkeletons_;
}

QImage KinectDevice::VideoImage()
{
    return (owner_ != 0 ? owner_->VideoImage() : QImage());
}

QImage KinectDevice::DepthImage()
{
    return (owner_ != 0 ? owner_->DepthImage() : QImage());
}
