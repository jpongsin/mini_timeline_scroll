// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/VideoWidget.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QScreen>
#include <QWindow>
#include <QWheelEvent>

VideoWidget::VideoWidget(QWidget *parent) : QOpenGLWidget(parent) {}

VideoWidget::~VideoWidget() {
  if (m_mpv_gl) {
    makeCurrent();
    mpv_render_context_free(m_mpv_gl);
    m_mpv_gl = nullptr;
    doneCurrent();
  }
}

void VideoWidget::setMpvHandle(mpv_handle *handle) {
  //shorthand to assign widget to mpv handle
  m_mpv = handle;
}

void VideoWidget::on_update(void *ctx) {
  static_cast<VideoWidget *>(ctx)->update_cb();
}

void VideoWidget::update_cb() {
  QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

void VideoWidget::create_render_context() {
  if (m_mpv && !m_mpv_gl) {
    auto get_proc_address = [](void *, const char *name) {
      if (auto ctx = QOpenGLContext::currentContext())
        return (void *)ctx->getProcAddress(name);
      return (void *)nullptr;
    };

    mpv_opengl_init_params gl_params = {get_proc_address, nullptr};
    mpv_render_param render_params[] = {
      {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_OPENGL},
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params},
      {MPV_RENDER_PARAM_INVALID, nullptr}};

    if (mpv_render_context_create(&m_mpv_gl, m_mpv, render_params) < 0) {
      qDebug() << "Failed to create mpv render context";
    } else {
      mpv_render_context_set_update_callback(m_mpv_gl, VideoWidget::on_update,
                                             this);
    }
  }
}

void VideoWidget::initializeGL() {
  initializeOpenGLFunctions();
  create_render_context();
}

void VideoWidget::paintGL() {
  if (m_mpv_gl && width() > 0 && height() > 0) {
    mpv_opengl_fbo fbo = {.fbo = static_cast<int>(defaultFramebufferObject()),
                          .w = width(),
                          .h = height(),
                          .internal_format = 0};

    // Qt high dpi support
    fbo.w *= this->devicePixelRatioF();
    fbo.h *= this->devicePixelRatioF();

    int flip_y = 1;
    mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                                 {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                                 {MPV_RENDER_PARAM_INVALID, nullptr}};

    mpv_render_context_render(m_mpv_gl, params);
  }
}

void VideoWidget::resizeGL(int w, int h) {
  // mpv handles resize automatically in the next render call
  Q_UNUSED(w);
  Q_UNUSED(h);
}

void VideoWidget::loadFile(const QString &path) {
  load_file(m_mpv, path.toUtf8().constData());
}

VideoState VideoWidget::getMetadata() { return get_metadata(m_mpv); }

void VideoWidget::resetVideo() { mpv_command_string(m_mpv, "stop"); }

// Zoom / Pan Implementation
// 0.1 would be sturdy for zoom
// for finer zoom, 0.01
void VideoWidget::zoomPlus() {
  //some sort of an exponential increase
  m_zoom = (m_zoom + 0.01) * 1.05 - 0.01;

  //cap zoom limit
  if (m_zoom > 10.0) m_zoom = 10.0;

  set_zoom(m_mpv, m_zoom);
}

void VideoWidget::zoomMinus() {
  // Reduce by a percentage
  m_zoom = (m_zoom + 0.01) / 1.05 - 0.01;

  //cap zoom limit
  if (m_zoom < 0.0) m_zoom = 0.0;

  set_zoom(m_mpv, m_zoom);
}


// typically, 0.05 would be good for pan
// 0.01 is for finer pan
void VideoWidget::moveLeft() {
  double sensitivity = 0.005 / (m_zoom + 1.0);
  m_panX += sensitivity;

  if (m_panX > 1.0) m_panX = 1.0;
  set_pan(m_mpv, m_panX, m_panY);
}


void VideoWidget::moveRight() {
  double slick_panX = 0.005 / (m_zoom + 1.0);
  m_panX -= slick_panX;

  if (m_panX > 1.0) m_panX = 1.0;
  set_pan(m_mpv, m_panX, m_panY);
}

void VideoWidget::moveUp() {
  double slick_panY = 0.005 / (m_zoom + 1.0);
  m_panY += slick_panY;

  if (m_panY > 1.0) m_panY = 1.0;
  set_pan(m_mpv, m_panX, m_panY);
}

void VideoWidget::moveDown() {
  double slick_panY = 0.005 / (m_zoom + 1.0);
  m_panY -= slick_panY;

  if (m_panY > 1.0) m_panY = 1.0;
  set_pan(m_mpv, m_panX, m_panY);
}

void VideoWidget::resetView() {
  m_zoom = 0.0;
  m_panX = 0.0;
  m_panY = 0.0;
  set_zoom(m_mpv, m_zoom);
  set_pan(m_mpv, m_panX, m_panY);
}
