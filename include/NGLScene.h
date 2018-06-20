#ifndef NGLSCENE_H_
#define NGLSCENE_H_

#include <ngl/Transformation.h>
#include <ngl/Vec3.h>
#include <ngl/Text.h>
#include "WindowParams.h"
#include "ScreenQuad.h"
#include <QEvent>
#include <QResizeEvent>
#include <QOpenGLWidget>
#include <memory>


/// @file NGLScene.h
/// @brief a basic Qt GL window class for ngl demos
/// @author Jonathan Macey
/// @version 1.0
/// @date 10/10/10
/// Revision History :
/// Initial Version 10/10/10 (Binary day ;-0 )
/// @class GLWindow
/// @brief our main glwindow widget for NGL applications all drawing elements are
/// put in this file
class NGLScene : public QOpenGLWidget
{
Q_OBJECT        // must include this if you use Qt signals/slots
public :
  /// @brief Constructor for GLWindow
  //----------------------------------------------------------------------------------------------------------------------
  /// @brief Constructor for GLWindow
  /// @param [in] _parent the parent window to create the GL context in
  //----------------------------------------------------------------------------------------------------------------------
  NGLScene(QWidget *_parent );

		/// @brief dtor
	~NGLScene();
 public slots :
	/// @brief a slot to toggle wireframe mode
	/// @param[in] _mode the mode passed from the toggle
	/// button
	void toggleWireframe( bool _mode	 );

  void loadImage();
  void changeFace(int _index);

private :
	/// @brief m_wireframe mode
	bool m_wireframe;
  GLuint m_sourceEnvMapID;
  std::array<GLuint,6> m_cubeFaceTextureID;
  GLuint m_envCubemap;
  bool m_showQuad=false;
protected:
  //----------------------------------------------------------------------------------------------------------------------
  /// @brief the windows params such as mouse and rotations etc
  //----------------------------------------------------------------------------------------------------------------------
  WinParams m_win;
  /// @brief  The following methods must be implimented in the sub class
  /// this is called when the window is created
  void initializeGL();
  void createFBO();
  void createCubeMap();
  void captureCubeToTexture();

  /// @brief this is called whenever the window is re-sized
  /// @param[in] _w the width of the resized window
  /// @param[in] _h the height of the resized window
  void resizeGL(int _w , int _h);
  /// @brief this is the main gl drawing routine which is called whenever the window needs to
  // be re-drawn
  void paintGL();

  /// @brief our model position
  ngl::Vec3 m_modelPos;
  /// @brief our camera
  ngl::Mat4 m_view;
  ngl::Mat4 m_projection;
  /// @brief our transform for objects
	ngl::Transformation m_transform;
  std::unique_ptr<ScreenQuad> m_screenQuad;
private :
  /// @brief this method is called every time a mouse is moved
  /// @param _event the Qt Event structure

  void mouseMoveEvent (QMouseEvent * _event   );
  /// @brief this method is called everytime the mouse button is pressed
  /// inherited from QObject and overridden here.
  /// @param _event the Qt Event structure

  void mousePressEvent ( QMouseEvent *_event  );

  /// @brief this method is called everytime the mouse button is released
  /// inherited from QObject and overridden here.
  /// @param _event the Qt Event structure
  void mouseReleaseEvent (QMouseEvent *_event );
  void wheelEvent( QWheelEvent* _event );

  void loadMatricesToShader();

  ngl::Mat4 m_mouseGlobalTX;

  GLuint m_captureFBO;
  GLuint m_captureRBO;


  signals :
  void imageUpdated(const QImage &image);



};

#endif
