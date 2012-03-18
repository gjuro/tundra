// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Win.h"
#include <NuiApi.h>

#include "Math/float4.h"

#include <QPainter>
#include <QRectF>

class KinectHelper
{
    public:
        /// Constructor.
        KinectHelper();

        /// Deconstructor.
        virtual ~KinectHelper();

        /// Convert depth data to a rgb pixel data.
        /// @param USHORT pixel to convert.
        /// @return RGBQUAD converted pixel.
        RGBQUAD ConvertDepthShortToQuad(USHORT s);

        /// Connect two bones with a line in the given painter.
        /// @param QPainter* Painter to draw the bone line.
        /// @param QRectF Rect of the drawing surface.
        /// @param QVariant Bone 1 position.
        /// @param QVariant Bone 2 position.
        void ConnectBones(QPainter *p, QRectF pRect, float4 pos1, float4 pos2);
};
