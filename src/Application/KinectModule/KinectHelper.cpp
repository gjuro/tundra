// For conditions of distribution and use, see copyright notice in LICENSE

#include "KinectHelper.h"

KinectHelper::KinectHelper()
{
}

KinectHelper::~KinectHelper()
{
}

RGBQUAD KinectHelper::ConvertDepthShortToQuad(USHORT s)
{
    USHORT RealDepth = (s & 0xfff8) >> 3;
    USHORT Player = s & 7;

    // transform 13-bit depth information into an 8-bit intensity appropriate
    // for display (we disregard information in most significant bit)
    BYTE l = 255 - (BYTE)(256*RealDepth/0x0fff);

    RGBQUAD q;
    q.rgbRed = q.rgbBlue = q.rgbGreen = 0;

    switch( Player )
    {
        case 0:
        {
            q.rgbRed = l / 2;
            q.rgbBlue = l / 2;
            q.rgbGreen = l / 2;
            break;
        }
        case 1:
        {
            q.rgbRed = l;
            break;
        }
        case 2:
        {
            q.rgbGreen = l;
            break;
        }
        case 3:
        {
            q.rgbRed = l / 4;
            q.rgbGreen = l;
            q.rgbBlue = l;
            break;
        }
        case 4:
        {
            q.rgbRed = l;
            q.rgbGreen = l;
            q.rgbBlue = l / 4;
            break;
        }
        case 5:
        {
            q.rgbRed = l;
            q.rgbGreen = l / 4;
            q.rgbBlue = l;
            break;
        }
        case 6:
        {
            q.rgbRed = l / 2;
            q.rgbGreen = l / 2;
            q.rgbBlue = l;
            break;
        }
        case 7:
        {    
            q.rgbRed = 255 - ( l / 2 );
            q.rgbGreen = 255 - ( l / 2 );
            q.rgbBlue = 255 - ( l / 2 );
            break;
        }
    }

    return q;
}

void KinectHelper::ConnectBones(QPainter *p, QRectF pRect, float4 vec1, float4 vec2)
{
    vec1 *= 200;
    QPointF pos1 = pRect.center() + QPointF(vec1.x, vec1.y);

    vec2 *= 200;
    QPointF pos2 = pRect.center() + QPointF(vec2.x, vec2.y);

    p->drawLine(pos1, pos2);
}
