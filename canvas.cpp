#define DELETE(x) if(nullptr!=x)delete x;

#include <QDebug>
#include <QMatrix4x4>
#include <vector>
#include <QtMath>
#include <QPainterPath>
#include <QPolygonF>
#include <QList>
#include "mygl.h"
#include "canvas.h"

Canvas::Canvas(QWidget *parent)
{
    setParent(parent);
    _move_flag = 0;
    image = new QImage(1000, 1000, QImage::Format_ARGB32);
    camera = new Camera(QMatrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 3, 0, 0, 0, 1), 45);
    z_buffer = new float[1000 * 1000];
    show();
}

Canvas::~Canvas()
{
    DELETE(image);
    DELETE(camera);
    DELETE(z_buffer);
}

void Canvas::mouseMoveEvent(QMouseEvent *e)
{
    x = e->position().x();
    y = e->position().y();
    if (e->buttons() == Qt::MouseButton::RightButton)
    {
        if (_move_flag == 1)
        {
            this->update();
        }
        else
        {
            _move_flag = 1;
            x_old = x;
            y_old = y;
        }
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *e)
{
    _move_flag = 0;
}

void Canvas::paintEvent(QPaintEvent *event)
{
    draw();
    x_old = x;
    y_old = y;
}

void Canvas::draw()
{
    Model* model = ((Mygl*)(parentWidget()->parentWidget()))->model();
    if (nullptr == model)
    {
        return;
    }
    quint32 *ptr;
    ptr = (quint32*)image->bits();
    for (int i = 0; i < 1000000; ++i)
    {
        ptr[i] = 0;
    }
    if (_move_flag)
    {
        auto delta = QMatrix4x4();
        auto _data = camera->tf().column(0);
        delta.rotate((float)(x_old - x) * .4, 0, 1, 0);
        delta.rotate((float)(y_old - y) * .4, _data[0], _data[1], _data[2]);
        camera->transform(std::move(delta));
    }
    QPainter painter(image);
    for (int i = 0; i < 1000 * 1000; ++i)
    {
        z_buffer[i] = -std::numeric_limits<float>::max();
    }
    for (int i = 0; i < model->nfaces(); ++i)
    {
        //// Drawing wire frame.
        //for (int j = 0; j < 3; ++j)
        //{
        //    int k = (j + 1) % 3;
        //    vec3 start = model->vert(i, j);
        //    vec3 end = model->vert(i, k);
        //    float z_s, z_e;
        //    QPointF _s = camera->shot(start, z_s);
        //    QPointF _e = camera->shot(end, z_e);
        //    int alpha = 255 - qMax(0, qMin(255, qFloor(-(z_s + z_e) * .5 * 255. / 4.)));
        //    painter.setPen(QColor(0, 0, 0, alpha));
        //    painter.drawLine(_s, _e);
        //}

        // Shading.
        float z0, z1, z2;
        QPointF p0 = camera->shot(model->vert(i, 0), z0);
        QPointF p1 = camera->shot(model->vert(i, 1), z1);
        QPointF p2 = camera->shot(model->vert(i, 2), z2);
        QPointF pts[] = { p0, p1, p2 };
        double zs[] = { z0, z1, z2 };
        // Set brush by normal.
        vec3 norm = model->normal(i, 0) + model->normal(i, 1) + model->normal(i, 2);
        norm = norm.normalized();
        int rgb = 255 * ((norm.z + 1.) * .5);
        painter.setPen(QColor(rgb, rgb, rgb));
        // Bounding box.
        float box_xmin(std::numeric_limits<float>::max());
        float box_ymin(std::numeric_limits<float>::max());
        float box_xmax(-std::numeric_limits<float>::max());
        float box_ymax(-std::numeric_limits<float>::max());
        for (int j = 0; j < 3; ++j)
        {
            box_xmin = qMin(box_xmin, pts[j].x());
            box_ymin = qMin(box_ymin, pts[j].y());
            box_xmax = qMax(box_xmax, pts[j].x());
            box_ymax = qMax(box_ymax, pts[j].y());
        }
        // Barycentric.
        float m00 = p0.x() - p2.x();
        float m01 = p1.x() - p2.x();
        float m10 = p0.y() - p2.y();
        float m11 = p1.y() - p2.y();
        float det = (m00 * m11 - m01 * m10);
        if (qAbs(det) < 1e-5) continue;
        float invdet = 1. / det;
        float n00 = invdet * m11;
        float n01 = invdet * -m01;
        float n10 = invdet * -m10;
        float n11 = invdet * m00;
        // Raster
        int bxmin = qFloor(box_xmin);
        int bymin = qFloor(box_ymin);
        int bxmax = qCeil(box_xmax);
        int bymax = qCeil(box_ymax);
        for (int yPos = bymin; yPos <= bymax; yPos++)
        {
            for (int xPos = bxmin; xPos <= bxmax; xPos++)
            {
                float u = n00 * (xPos - p2.x()) + n01 * (yPos - p2.y());
                float v = n10 * (xPos - p2.x()) + n11 * (yPos - p2.y());
                float w = 1. - u - v;
                float l[] { u, v, w };
                if (u < 0 || v < 0 || w < 0) continue;
                float pz = 0;
                for (int k = 0; k < 3; ++k)
                {
                    pz += l[k] * zs[k];
                }
                int px = (int)qMax(qMin(xPos, 999), 0);
                int py = (int)qMax(qMin(yPos, 999), 0);
                int index = px + py * 1000;
                if (z_buffer[index] < pz) {
                    z_buffer[index] = pz;
                    painter.drawPoint(px, py);
                }
            }
        }
    }
    image->mirror();
    QPainter painter1(this);
    painter1.drawImage(0, 0, *image);
}

void Canvas::changeFovy(int fovy)
{
    this->camera->setFovy(fovy);
    update();
}
