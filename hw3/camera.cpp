#include "camera.h"

Camera::Camera(int width, int height) {
  mFovY = 45.0f;
  mAspect = (float) width / (float) height;
  mNear = 0.1f;
  mFar = 200.0f;
  
  mEye = glm::vec3(0.0f, 0.0f, 65.0f);
  mRef = glm::vec3(0.0f, 0.0f, 0.0f);
  mUp = glm::vec3(0.0f, 1.0f, 0.0f);

  mProjectionMatrix = glm::perspective(mFovY, mAspect, mNear, mFar);
  mViewMatrix = glm::lookAt(mEye, mRef, mUp);
}

Camera::~Camera() {
}

void Camera::dollyIn() {
  glm::vec3 direction = mRef - mEye;

  if (glm::length(direction) > 20.0f) {
    glm::vec3 v = glm::normalize(direction);
    mEye.x += v.x;
    mEye.y += v.y;
    mEye.z += v.z;
  }
}

void Camera::dollyOut() {
  glm::vec3 direction = mRef - mEye;
 
  if (glm::length(direction) < 65.0f) {
    glm::vec3 v = glm::normalize(direction);
    mEye.x -= v.x;
    mEye.y -= v.y;
    mEye.z -= v.z;
  }
}

void Camera::zoomIn() {
  if (mFovY > 44.25f) mFovY -= 0.025f;
}

void Camera::zoomOut() {
  if (mFovY < 45.75f) mFovY += 0.025f;
}

void Camera::pan(int direction) {
  glm::vec3 v = glm::normalize(glm::cross(mRef - mEye, mUp));

  switch (direction) {
    case DIRECTION_UP:
      mEye.x += mUp.x;
      mEye.y += mUp.y;
      mEye.z += mUp.z;
      mRef.x += mUp.x;
      mRef.y += mUp.y;
      mRef.z += mUp.z;
      break;
    case DIRECTION_DOWN:
      mEye.x -= mUp.x;
      mEye.y -= mUp.y;
      mEye.z -= mUp.z;
      mRef.x -= mUp.x;
      mRef.y -= mUp.y;
      mRef.z -= mUp.z;
      break;
    case DIRECTION_LEFT:
      mEye.x -= v.x;
      mEye.y -= v.y;
      mEye.z -= v.z;
      mRef.x -= v.x;
      mRef.y -= v.y;
      mRef.z -= v.z;
      break;
    case DIRECTION_RIGHT:
      mEye.x += v.x;
      mEye.y += v.y;
      mEye.z += v.z;
      mRef.x += v.x;
      mRef.y += v.y;
      mRef.z += v.z;
      break;
  }
}

void Camera::showAll() {
}

void Camera::seek(int x, int y) {
  GLint viewport[4];
  GLdouble projectionMatrix[16];
  GLdouble viewMatrix[16];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
  glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);

  GLfloat winX, winY, winZ;
  GLdouble posX, posY, posZ;
  winX = (float) x;
  winY = (float) viewport[3] - (float) y;
  glReadPixels(x, (int) winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
  gluUnProject(winX, winY, winZ, viewMatrix, projectionMatrix, viewport, &posX, &posY, &posZ);

  mRef.x = posX;
  mRef.y = posY;
  mRef.z = posZ;
}

void Camera::changeAspect(float aspect) {
  mAspect = aspect; 
}

glm::mat4 Camera::calculateProjectionMatrix() {
  mProjectionMatrix = glm::perspective(mFovY, mAspect, mNear, mFar);
  return mProjectionMatrix;
}

glm::mat4 Camera::calculateViewMatrix() {
  mViewMatrix = glm::lookAt(mEye, mRef, mUp);
  return mViewMatrix;
}
