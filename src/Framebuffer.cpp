#include "Framebuffer.h"
#include <QOpenGLContext>

Framebuffer::Framebuffer(size_t _w, size_t _h,GLuint _defaultFBO) :
  m_width(_w),m_height(_h),m_defaultFBO(_defaultFBO)
{

  glGenFramebuffers(1, &m_fboID);
  glGenRenderbuffers(1, &m_fboID);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rboID);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _w, _h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboID);
  unbind();
}
Framebuffer::~Framebuffer()
{
  glDeleteFramebuffers(1,&m_fboID);
  glDeleteRenderbuffers(1,&m_rboID);
}
void Framebuffer::bind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
}

void Framebuffer::unbind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());

}
