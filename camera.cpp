#include "camera.h"

Camera::Camera(){

}

Camera::Camera(QMatrix4x4&& tf, int&& fovy) : _tf(tf), _fovy(fovy)
{
    float a = 1. / qTan(0.5 * qDegreesToRadians(_fovy));
    this->_persp = QMatrix4x4(a, 0, 0, 0, 0, a, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0);
    this->_tf_inv = tf.inverted();
    this->_compound = _persp * _tf_inv;
}

Camera::~Camera() {

}

QPointF Camera::shot(vec3 &v, float& z) {
    QVector3D t = this->_tf_inv.map(QVector3D(v.x, v.y, v.z));
    QVector3D r = this->_persp.map(t);
    z = t.z();
    return QPointF((r.x() + 1.) * 500, (r.y() + 1.) * 500);
}

void Camera::transform(QMatrix4x4&& tf) {
    this->_tf = tf * this->_tf;
    this->_tf_inv = this->_tf.inverted();
    this->_compound = _persp * _tf_inv;
}
