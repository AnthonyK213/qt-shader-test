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
    image = new QImage(690, 690, QImage::Format_ARGB32);
    camera = new Camera(QMatrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 3, 0, 0, 0, 1), 45);
    float r2 = qSqrt(2.);
    float r3 = qSqrt(3.);
    float r6 = qSqrt(6.);
    light = new Camera(QMatrix4x4(1./r2, -1./r6, 1./r3, r3, 0, 2./r6, 1./r3, r3, -1./r2, -1./r6, 1./r3, r3, 0, 0, 0, 1), 45);
    light->setView(1);
    z_buffer = new float[690 * 690];
    shadow_buffer = new float[690 * 690];
    show();
}

Canvas::~Canvas()
{
    DELETE(image);
    DELETE(camera);
    DELETE(light);
    DELETE(z_buffer);
    DELETE(shadow_buffer);
}

void Canvas::wheelEvent(QWheelEvent *e)
{
    QPoint delta = e->angleDelta();
    addFovy(-delta.y() * .03);
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

void Canvas::setShadow()
{
    Model* model = ((Mygl*)(parentWidget()->parentWidget()))->model();
    if (nullptr == model) return;
    for (int i = 0; i < 690 * 690; ++i)
    {
        shadow_buffer[i] = -std::numeric_limits<float>::max();
    }
    for (int i = 0; i < model->nfaces(); ++i)
    {
        // Shading.
        float z0, z1, z2;
        QVector3D m0 = model->vert(i, 0);
        QVector3D m1 = model->vert(i, 1);
        QVector3D m2 = model->vert(i, 2);
        QPointF p0 = light->shot(m0, z0);
        QPointF p1 = light->shot(m1, z1);
        QPointF p2 = light->shot(m2, z2);
        QPointF pts[] = { p0, p1, p2 };
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
        int bxmin = qMax(qFloor(box_xmin), 0);
        int bymin = qMax(qFloor(box_ymin), 0);
        int bxmax = qMin(qCeil(box_xmax), 690);
        int bymax = qMin(qCeil(box_ymax), 690);
        for (int yPos = bymin; yPos < bymax; yPos++)
        {
            for (int xPos = bxmin; xPos < bxmax; xPos++)
            {
                float u = n00 * (xPos - p2.x()) + n01 * (yPos - p2.y());
                float v = n10 * (xPos - p2.x()) + n11 * (yPos - p2.y());
                float w = 1. - u - v;
                if (u < 0 || v < 0 || w < 0) continue;
                float pz = u * z0 + v * z1 + w * z2;
                int index = xPos + yPos * 690;
                if (shadow_buffer[index] < pz) {
                    shadow_buffer[index] = pz;
                }
            }
        }
    }
}

void Canvas::draw()
{
    image->fill(Qt::black);
    //quint32 *ptr = (quint32*)image->bits();
    //for (int i = 0; i < 690 * 690; ++i) ptr[i] = 0;
    Model* model = ((Mygl*)(parentWidget()->parentWidget()))->model();
    if (nullptr == model)
    {
        QPainter painter1(this);
        painter1.drawImage(0, 0, *image);
        return;
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
    QVector3D _light = (QVector3D { -1, -1, -1 }).normalized();
    for (int i = 0; i < 690 * 690; ++i)
    {
        z_buffer[i] = -std::numeric_limits<float>::max();
    }
    for (int i = 0; i < model->nfaces(); ++i)
    {
        // Shading.
        //float x0, x1, x2;
        //float y0, y1, y2;
        float z0, z1, z2;
        QVector3D m0 = model->vert(i, 0);
        QVector3D m1 = model->vert(i, 1);
        QVector3D m2 = model->vert(i, 2);
        QPointF p0 = camera->shot(m0, z0);
        QPointF p1 = camera->shot(m1, z1);
        QPointF p2 = camera->shot(m2, z2);
        QPointF pts[] = { p0, p1, p2 };
        QVector3D n0 = model->normal(i, 0);
        QVector3D n1 = model->normal(i, 1);
        QVector3D n2 = model->normal(i, 2);
        if (this->_shade == 0)
        {
            QVector3D norm = (n0 + n1 + n2).normalized();
            int rgb = (1. - QVector3D::dotProduct(norm, _light)) * .5 * 200;
            painter.setPen(QColor(rgb, rgb, rgb));
        }
        // Diffuse
        const QImage& diffuse = model->diffuse();
        // Specular
        const QImage& specular = model->specular();
        // UV, tangent
        QVector2D uv0, uv1, uv2;
        QVector3D T_;
        if (diffuse.width() > 0)
        {
            uv0 = model->uv(i, 0);
            uv1 = model->uv(i, 1);
            uv2 = model->uv(i, 2);
            QVector2D e1 = uv1 - uv0;
            QVector2D e2 = uv2 - uv0;
            float _det = e1.x() * e2.y() - e1.y() * e2.x();
            float t00 = e2.y() / _det;
            float t01 = -e1.y() / _det;
            //float t10 = -e2.x / _det;
            //float t11 = e1.x / _det;
            QVector3D E1 = m1 - m0;
            QVector3D E2 = m2 - m0;
            T_.setX(t00 * E1.x() + t01 * E2.x());
            T_.setY(t00 * E1.y() + t01 * E2.y());
            T_.setZ(t00 * E1.z() + t01 * E2.z());
        }
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
        int bxmin = qMax(qFloor(box_xmin), 0);
        int bymin = qMax(qFloor(box_ymin), 0);
        int bxmax = qMin(qCeil(box_xmax), 690);
        int bymax = qMin(qCeil(box_ymax), 690);
        for (int yPos = bymin; yPos < bymax; yPos++)
        {
            for (int xPos = bxmin; xPos < bxmax; xPos++)
            {
                float u = n00 * (xPos - p2.x()) + n01 * (yPos - p2.y());
                float v = n10 * (xPos - p2.x()) + n11 * (yPos - p2.y());
                float w = 1. - u - v;
                if (u < 0 || v < 0 || w < 0) continue;
                float _u = u / z0;
                float _v = v / z1;
                float _w = w / z2;
                float _m = _u + _v + _w;
                float pz = (_u * z0 + _v * z1 + _w * z2) / _m;
                int index = xPos + yPos * 690;
                if (z_buffer[index] < pz) {
                    z_buffer[index] = pz;
                    // Smooth shading.
                    if (this->_shade == 1)
                    {
                        auto cam_z = camera->tf().column(2);
                        QVector3D n = (n0 * _u + n1 * _v + n2 * _w).normalized();
                        if (_m < 0) n = n * (-1.);
                        float light_depth;
                        auto sx = light->shot((_u * m0 + _v * m1 + _w * m2) / _m, light_depth);
                        float shadow_indensity = 1.;
                        float _l_depth = shadow_buffer[qMin(qMax(0, (int)sx.x()), 689) + 690 * qMin(qMax(0, (int)sx.y()), 689)];
                        if (qAbs(_l_depth - light_depth) > .01) // z-fighting
                        {
                            shadow_indensity = .86f;
                        }
                        if (diffuse.width() > 0)
                        {
                            QVector2D uv = (uv0 * _u + uv1 * _v + uv2 * _w) / _m;
                            QVector3D n0 = model->normal(uv).normalized();
                            QVector3D t_ = (T_ - QVector3D::dotProduct(T_, n) * n).normalized();
                            QVector3D b_ = QVector3D::crossProduct(n, t_);
                            QVector3D n_ = {
                                t_.x() * n0.x() + b_.x() * n0.y() + n.x() * n0.z(),
                                t_.y() * n0.x() + b_.y() * n0.y() + n.y() * n0.z(),
                                t_.z() * n0.x() + b_.z() * n0.y() + n.z() * n0.z(),
                            };
                            n_ = n_.normalized();
                            QVector3D r = -2. * n_ * QVector3D::dotProduct(n_, _light) + _light;
                            float diff = (1. - QVector3D::dotProduct(n_, _light)) * .5;
                            float spec = pow(qMax(r.z(), 0.0f), qRed(specular.pixel(uv.x() * specular.width(), uv.y() * specular.height())));
                            float indensity = (diff + .6 * spec) * shadow_indensity;
                            QRgb pixel = diffuse.pixel(uv.x() * diffuse.width(), uv.y() * diffuse.height());
                            painter.setPen(QColor(qMin(255., qRed(pixel) * indensity),
                                                  qMin(255., qGreen(pixel) * indensity),
                                                  qMin(255., qBlue(pixel) * indensity)));
                        }
                        else
                        {
                            QVector3D r = -2. * n * QVector3D::dotProduct(n, _light) + _light;
                            float diff = (1. - QVector3D::dotProduct(n, _light)) * .5;
                            if ((QVector3D::dotProduct(QVector3D { cam_z.x(), cam_z.y(), cam_z.z() }, r) + 1.) * 0.5 > 0.996 && shadow_indensity == 1.)
                            {
                                diff = 1.1f;
                            }
                            int rgb = 200 * diff * shadow_indensity;
                            painter.setPen(QColor(rgb, rgb, rgb));
                        }
                    }
                    painter.drawPoint(xPos, yPos);
                }
            }
        }

        //// Drawing wire frame.
        //for (int j = 0; j < 3; ++j)
        //{
        //    int k = (j + 1) % 3;
        //    QVector3D start = model->vert(i, j);
        //    QVector3D end = model->vert(i, k);
        //    float z_s, z_e;
        //    QPointF _s = camera->shot(start, z_s);
        //    QPointF _e = camera->shot(end, z_e);
        //    //int alpha = 255 - qMax(0, qMin(255, qFloor(-(z_s + z_e) * .5 * 255. / 4.)));
        //    //painter.setPen(QColor(0, 0, 0, alpha));
        //    painter.setPen(QColor(0, 0, 0));
        //    painter.drawLine(_s, _e);
        //}
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

void Canvas::addFovy(int delta)
{
    this->camera->addFovy(delta);
    update();
}

void Canvas::setView(int index)
{
    this->camera->setView(index);
    update();
}

void Canvas::setShade(int index)
{
    this->_shade = index;
    update();
}
