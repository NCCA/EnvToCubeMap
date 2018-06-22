#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_
#include <ngl/Types.h>
class Framebuffer
{
  public :
    Framebuffer(size_t _w, size_t _h);
    ~Framebuffer();
    void bind() const ;
    void unbind() const;
  private :

    GLuint m_fboID;
    GLuint m_rboID;
    GLuint m_textureID;
    size_t m_width;
    size_t m_height;


};

#endif
