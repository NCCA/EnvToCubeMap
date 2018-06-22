#include "NGLScene.h"
#include <iostream>
#include <ngl/Vec3.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Image.h>
#include <QByteArray>
#include <QColorDialog>
#include <QFileDialog>
#include <QImage>
#include <ngl/Texture.h>
#include <algorithm>
#include <OpenImageIO/imageio.h>


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
  QString filename = QFileDialog::getOpenFileName( nullptr, tr("load texture"),QDir::currentPath(),tr("*.*") );
  size_t width, height, channels;

  if( !filename.isNull() )
  {
    if(m_sourceEnvMapID !=0)
    {
      glDeleteTextures(1,&m_sourceEnvMapID);
    }

    OpenImageIO::ImageInput *in = OpenImageIO::ImageInput::open (filename.toStdString());
    const OpenImageIO::ImageSpec &spec = in->spec();
    width = static_cast<size_t>(spec.width);
    height = static_cast<size_t>(spec.height);
    channels = static_cast<size_t>(spec.nchannels);
    GLenum format=GL_RGB;
    if(channels==4) format=GL_RGBA;

    std::unique_ptr<float[]> data=std::make_unique<float[]>(width*height*channels);
    size_t scanlinesize = width * channels * sizeof(float);

    in->read_image (OpenImageIO::TypeDesc::FLOAT,(char *)data.get() + (height-1)*scanlinesize, OpenImageIO::AutoStride, -scanlinesize, OpenImageIO::AutoStride);
    in->close ();

    glGenTextures(1, & m_sourceEnvMapID );
    glBindTexture(GL_TEXTURE_2D, m_sourceEnvMapID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, format, GL_FLOAT, data.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    auto size=width*height*channels;
    QByteArray imagedata(size,0);
    for (int i = 0; i<size; ++i)
    {
      imagedata[i] = std::max(0.0f, std::min((data[i] * 255.0f), 255.0f)); //std::clamp((data[i] * 255),0,255);
    }
    QImage image((const unsigned char*)imagedata.constData(), width, height, QImage::Format_RGB888);
    QTransform myTransform;
    myTransform.rotate(180);
    image = image.transformed(myTransform);
    emit(imageUpdated(image));
  }
  //m_mapsGenerated=false;
  captureCubeToTexture();
  captureIrradianceToTexture();
 // update();
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::initializeGL()
{

  ngl::NGLInit::instance();
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  /// create our camera
  ngl::Vec3 eye(2.0f,2.0f,2.0f);
  ngl::Vec3 look(0.0f,0.0f,0.0f);
  ngl::Vec3 up(0.0f,1.0f,0.0f);

  m_view[0]=ngl::lookAt(eye,look,up);
  m_view[1]=ngl::lookAt({0.0f,0.0f,0.0f},{0.0f,0.0f,1.0f},{0.0f,1.0f,0.0f});
  m_projection=ngl::perspective(45,float(1024/720),0.1f,30.0f);
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->loadShader("EnvMapProjection","shaders/EnvMapProjectVertex.glsl","shaders/EnvMapProjectFragment.glsl");
  shader->use("EnvMapProjection");
  shader->setUniform("equirectangularMap",0);
  shader->setUniform("irradiance",1);

  shader->loadShader("irradiance","shaders/EnvMapProjectVertex.glsl","shaders/irradianceFragment.glsl");

  shader->loadShader("ScreenQuad","shaders/TextureVertex.glsl","shaders/TextureFragment.glsl");

  m_screenQuad.reset( new ScreenQuad("ScreenQuad"));

  m_envFramebuffer=std::make_unique<Framebuffer>(m_textureSize,m_textureSize);
  m_irradianceFramebuffer=std::make_unique<Framebuffer>(m_textureSize,m_textureSize);
  createCubeMap();

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



//----------------------------------------------------------------------------------------------------------------------
//This virtual function is called whenever the widget needs to be painted.
// this is our main drawing routine
void NGLScene::paintGL()
{
  auto *shader=ngl::ShaderLib::instance();


  if(!m_showQuad)
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
    m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
    m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
    m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,m_win.width,m_win.height);
    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    shader->use("EnvMapProjection");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m_sourceEnvMapID);
    shader->setUniform("VP",m_projection*m_view[m_activeView]*m_mouseGlobalTX);
    prim->draw("cube");
  }
  else
  {
    //m_envFramebuffer->bind();
//    glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());
  //  captureCubeToTexture();
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);

    glViewport(0,0,m_win.width,m_win.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //shader->use("ScreenQuad");
    m_screenQuad->draw();
    //m_envFramebuffer->unbind();

  }

  if(m_saveFile)
  {
    saveImagesToFile();
    m_saveFile=false;
  }

}


void NGLScene::saveImagesToFile()
{
  // we are going to render to a framebuffer and extract
   GLuint fboID,rboID;
   glGenFramebuffers(1, &fboID);
   glBindFramebuffer(GL_FRAMEBUFFER, fboID);
   // create a renderbuffer object to store depth info
   glGenRenderbuffers(1, &rboID);
   glBindRenderbuffer(GL_RENDERBUFFER, rboID);

   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_textureSize, m_textureSize);
   // bind
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   // attatch the texture we created earlier to the FBO
   GLuint textureID;
   glGenTextures(1, &textureID);
   // bind it to make it active
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, textureID);
   // set params
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   //glGenerateMipmapEXT(GL_TEXTURE_2D);  // set the data size but just set the buffer to 0 as we will fill it with the FBO
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_textureSize, m_textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

   // now attach a renderbuffer to depth attachment point
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboID);
   // now got back to the default render context
   // were finished as we have an attached RB so delete it
   glDeleteRenderbuffers(1,&rboID);
   auto *shader=ngl::ShaderLib::instance();

   shader->use("ScreenQuad");
   shader->setUniform("tex",0);
   const std::array<const char *,6> prefix={{
     "PlusX","MinusX","PlusY","MinusY","PlusZ","MinusZ"
   }};

   const std::array<const char *,5> extensions={{"png","tif","exr","hdr","jpg" }};

   // loop for all the faces
   for(size_t i=0; i<6; ++i )
   {
     // clear the screen and set viewport to texture size
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glViewport(0,0,m_textureSize,m_textureSize);
     shader->setUniform("face",static_cast<int>(i));
     m_screenQuad->draw();
     QString fname = QString("%1/%2-%3.%4").arg(m_saveFilePath,prefix[i],m_saveFileName,extensions[m_saveType]);
     std::cout<<"saving "<<fname.toStdString()<<'\n';
     glReadBuffer(GL_COLOR_ATTACHMENT0);

     int size=3;
     std::unique_ptr<unsigned char []> data( new unsigned char [m_textureSize * m_textureSize *size]);
     glReadPixels(0,0,m_textureSize,m_textureSize,GL_RGB,GL_UNSIGNED_BYTE,data.get());
     OpenImageIO::ImageOutput *out = OpenImageIO::ImageOutput::create (fname.toStdString().c_str());
     OpenImageIO::ImageSpec spec (m_textureSize, m_textureSize, size, OpenImageIO::TypeDesc::UINT8);
     int scanlinesize = m_textureSize * size;

     out->open (fname.toStdString(), spec);
     // note this flips the image vertically on writing
     // (see http://www.openimageio.org/openimageio.pdf pg 20 for details)
     out->write_image (OpenImageIO::TypeDesc::UINT8,
                       data.get() + (m_textureSize-1)*scanlinesize,
                       OpenImageIO::AutoStride,
                       -scanlinesize,OpenImageIO::AutoStride);
     out->close ();


   }

   glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
   // remove the FB and textures we created.
   glDeleteFramebuffers(1,&fboID);
   glDeleteTextures(1,&textureID);
}




void NGLScene::changeFace(int _index)
{
  auto *shader = ngl::ShaderLib::instance();
  shader->use("ScreenQuad");
  shader->setUniform("face",_index);
  update();
}

void NGLScene::changeTextureSize(int _size)
{
  switch(_size)
  {
    case 0 : m_textureSize=512; break;
    case 1 : m_textureSize=1024; break;
    case 2 : m_textureSize=2048; break;
    case 3 : m_textureSize=4096; break;
  }

  //createFBO();
  m_envFramebuffer=std::make_unique<Framebuffer>(m_textureSize,m_textureSize,defaultFramebufferObject());
  m_irradianceFramebuffer=std::make_unique<Framebuffer>(m_textureSize,m_textureSize,defaultFramebufferObject());

  createCubeMap();
//  glBindFramebuffer(GL_FRAMEBUFFER,0);
  update();

}

void NGLScene::captureCubeToTexture()
{
  std::cout<<"DOING CAPTURE\n";
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

  glViewport(0,0,m_textureSize,m_textureSize);
  //glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
  m_envFramebuffer->bind();
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  (*shader)["EnvMapProjection"]->use();

  for( size_t i = 0; i < 6; ++i)
  {
    shader->setUniform("VP",captureProjection*captureViews[i]);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                            m_envCubemap, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    prim->draw("cube");
  }
  m_envFramebuffer->unbind();
//  glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());
//  std::cout<<"DEFAULT FBO "<<defaultFramebufferObject()<<'\n';

}


void NGLScene::captureIrradianceToTexture()
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

  glViewport(0,0,m_textureSize,m_textureSize);
  //glBindFramebuffer(GL_FRAMEBUFFER, m_irradianceCaptureFBO);
  m_irradianceFramebuffer->bind();
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["irradiance"]->use();
  shader->setUniform("cubeMap",0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP,m_envCubemap);
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();

  for( size_t i = 0; i < 6; ++i)
  {
    shader->setUniform("VP",captureProjection*captureViews[i]);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceCubemap, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    prim->draw("cube");
  }
  m_irradianceFramebuffer->unbind();

}


void NGLScene::createCubeMap()
{
  if(m_envCubemap !=0)
  {
    glDeleteTextures(1, &m_envCubemap);
  }
  glGenTextures(1, &m_envCubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
  for (unsigned int i = 0; i < 6; ++i)
  {
    // note that we store each face with 16 bit floating point values
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                 m_textureSize, m_textureSize, 0, GL_RGB, GL_FLOAT, nullptr);
}
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if(m_irradianceCubemap !=0)
  {
    glDeleteTextures(1, &m_irradianceCubemap);
  }
  glGenTextures(1, &m_irradianceCubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceCubemap);
  for (unsigned int i = 0; i < 6; ++i)
  {
    // note that we store each face with 16 bit floating point values
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                 m_textureSize, m_textureSize, 0, GL_RGB, GL_FLOAT, nullptr);
}
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}



void NGLScene::saveImages()
{
   m_saveFilePath = QFileDialog::getExistingDirectory(  0, tr("Save cubemaps"),QDir::currentPath() );
  if(!m_saveFilePath.isNull())
  {
    m_saveFile=true;
    // note we need to force a draw of each of the frames to capture them.
    update();
  }
}


NGLScene::~NGLScene()
{
}

void NGLScene::toggleWireframe(bool _mode	 )
{
  m_showQuad=_mode;
	update();
}

