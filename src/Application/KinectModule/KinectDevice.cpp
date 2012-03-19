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
    emit SkeletonUpdate(skeleton);
}

void KinectDevice::EmitSkeletonStateChanged(KinectSkeleton *skeleton)
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

bool KinectDevice::HasKinect()
{
    return (owner_ != 0 ? owner_->HasKinect() : false);
}

bool KinectDevice::IsStarted()
{
    return (owner_ != 0 ? owner_->IsStarted() : false);
}

bool KinectDevice::StartKinect()
{
    return (owner_ != 0 ? owner_->StartKinect() : false);
}

void KinectDevice::StopKinect()
{
    return (owner_ != 0 ? owner_->StopKinect() : false);
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
