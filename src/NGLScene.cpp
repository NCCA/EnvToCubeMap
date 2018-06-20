#include "NGLScene.h"
#include <iostream>
#include <ngl/Vec3.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <QByteArray>
#include <QColorDialog>
#include <QFileDialog>
#include <QImage>
#include <ngl/Texture.h>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


//----------------------------------------------------------------------------------------------------------------------
NGLScene::NGLScene( QWidget *_parent ) : QOpenGLWidget( _parent )
{

  // set this widget to have the initial keyboard focus
  setFocus();
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  this->resize(_parent->size());
	m_wireframe=false;
}


void NGLScene::loadImage()
{
  std::cout<<"load\n";
  QString filename = QFileDialog::getOpenFileName(
            0,
            tr("load texture"),
            QDir::currentPath(),
            tr("*.*") );
  int width, height, nrComponents;

  if( !filename.isNull() )
  {
    if(m_sourceEnvMapID !=0)
    {
      glDeleteTextures(1,&m_sourceEnvMapID);
    }
    stbi_set_flip_vertically_on_load(true);
    float *data = stbi_loadf(filename.toStdString().c_str(), &width, &height, &nrComponents, 0);
    std::cout<<"loaded image "<<filename.toStdString()<<" width "<<width<<" height "<<height<<" components "<<nrComponents<<'\n';
    glGenTextures(1, & m_sourceEnvMapID );
    glBindTexture(GL_TEXTURE_2D, m_sourceEnvMapID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    size_t size=width*height*nrComponents;
    QByteArray imagedata(size,0);
    for (int i = 0; i<size; ++i)
    {
      imagedata[i] = std::max(0.0, std::min((data[i] * 255.0), 255.0)); //std::clamp((data[i] * 255),0,255);
    }
    QImage image((const unsigned char*)imagedata.constData(), width, height, QImage::Format_RGB888);
    QTransform myTransform;
    myTransform.rotate(180);
    image = image.transformed(myTransform);
    emit(imageUpdated(image));
    stbi_image_free(data);
  }

  update();
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::initializeGL()
{

  ngl::NGLInit::instance();
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  /// create our camera
  ngl::Vec3 eye(2,2,2);
  ngl::Vec3 look(0,0,0);
  ngl::Vec3 up(0,1,0);

  m_view=ngl::lookAt(eye,look,up);
  m_projection=ngl::perspective(45,float(1024/720),0.1,300);
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->loadShader("EnvMapProjection","shaders/EnvMapProjectVertex.glsl","shaders/EnvMapProjectFragment.glsl");
  shader->use("EnvMapProjection");
  shader->setUniform("equirectangularMap",0);

  shader->loadShader("ScreenQuad","shaders/TextureVertex.glsl","shaders/TextureFragment.glsl");

  createFBO();
  createCubeMap();
  m_screenQuad.reset( new ScreenQuad("ScreenQuad"));


}

//----------------------------------------------------------------------------------------------------------------------
//This virtual function is called whenever the widget has been resized.
// The new size is passed in width and height.
void NGLScene::resizeGL( int _w, int _h )
{
  m_projection=ngl::perspective( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["EnvMapProjection"]->use();
  shader->setUniform("VP",m_projection*m_view*m_mouseGlobalTX);
}

//----------------------------------------------------------------------------------------------------------------------
//This virtual function is called whenever the widget needs to be painted.
// this is our main drawing routine
void NGLScene::paintGL()
{
  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX( m_win.spinXFace );
  rotY.rotateY( m_win.spinYFace );
  // multiply the rotations
  m_mouseGlobalTX = rotX * rotY;
  // add the translations
  m_mouseGlobalTX.m_m[ 3 ][ 0 ] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[ 3 ][ 1 ] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[ 3 ][ 2 ] = m_modelPos.m_z;
  captureCubeToTexture();



  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_win.width,m_win.height);
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  if(m_wireframe == true)
  {
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  }
	loadMatricesToShader();
  glBindTexture(GL_TEXTURE_2D, m_sourceEnvMapID);
  prim->draw("cube");

  if(m_showQuad)
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
    auto *shader=ngl::ShaderLib::instance();
    shader->use("ScreenQuad");
    shader->setUniform("tex",0);
    glGenerateMipmap(GL_TEXTURE_2D);
    m_screenQuad->draw();
  }
}

void NGLScene::changeFace(int _index)
{
  auto *shader = ngl::ShaderLib::instance();
  shader->use("ScreenQuad");
  shader->setUniform("face",_index);
  update();
}

void NGLScene::captureCubeToTexture()
{
  ngl::Mat4 captureProjection = ngl::perspective(90.0f, 1.0f, 0.1f, 10.0f);
  std::array<ngl::Mat4,6>captureViews =
  {{
     ngl::lookAt(ngl::Vec3(0.0f, 0.0f, 0.0f), ngl::Vec3( 1.0f,  0.0f,  0.0f), ngl::Vec3(0.0f, -1.0f,  0.0f)),
     ngl::lookAt(ngl::Vec3(0.0f, 0.0f, 0.0f), ngl::Vec3(-1.0f,  0.0f,  0.0f), ngl::Vec3(0.0f, -1.0f,  0.0f)),
     ngl::lookAt(ngl::Vec3(0.0f, 0.0f, 0.0f), ngl::Vec3( 0.0f,  1.0f,  0.0f), ngl::Vec3(0.0f,  0.0f,  1.0f)),
     ngl::lookAt(ngl::Vec3(0.0f, 0.0f, 0.0f), ngl::Vec3( 0.0f, -1.0f,  0.0f), ngl::Vec3(0.0f,  0.0f, -1.0f)),
     ngl::lookAt(ngl::Vec3(0.0f, 0.0f, 0.0f), ngl::Vec3( 0.0f,  0.0f,  1.0f), ngl::Vec3(0.0f, -1.0f,  0.0f)),
     ngl::lookAt(ngl::Vec3(0.0f, 0.0f, 0.0f), ngl::Vec3( 0.0f,  0.0f, -1.0f), ngl::Vec3(0.0f, -1.0f,  0.0f))
  }};

  glViewport(0,0,512,512);
  glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
//  glBindTexture(GL_TEXTURE_CUBE_MAP,m_envCubemap);
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  (*shader)["EnvMapProjection"]->use();

  for( size_t i = 0; i < 6; ++i)
  {
    shader->setUniform("VP",captureProjection*captureViews[i]);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_envCubemap, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    prim->draw("cube");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

}


void NGLScene::createFBO()
{
  glGenFramebuffers(1, &m_captureFBO);
  glGenRenderbuffers(1, &m_captureRBO);

  glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
  glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_captureRBO);
}


void NGLScene::createCubeMap()
{
  glGenTextures(1, &m_envCubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
  for (unsigned int i = 0; i < 6; ++i)
  {
    // note that we store each face with 16 bit floating point values
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
}
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


NGLScene::~NGLScene()
{
}

void NGLScene::toggleWireframe(bool _mode	 )
{
  //m_wireframe=_mode;
  m_showQuad=_mode;
	update();
}
