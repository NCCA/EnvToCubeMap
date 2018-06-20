#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent), m_ui(new Ui::MainWindow)
{
  m_ui->setupUi(this);

  m_gl=new  NGLScene(this);

  m_ui->s_mainWindowGridLayout->addWidget(m_gl,0,0,2,1);
  connect(m_ui->m_wireframe,SIGNAL(toggled(bool)),m_gl,SLOT(toggleWireframe(bool)));
  connect(m_ui->m_selectFace,SIGNAL(currentIndexChanged(int)),m_gl,SLOT(changeFace(int)));

  connect(m_ui->m_loadImage,SIGNAL(clicked(bool)),m_gl,SLOT(loadImage()));
  connect(m_gl,&NGLScene::imageUpdated,
          [=](const QImage &image)
          {

            QIcon icon;
            QSize size;
            icon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::On);
            size = QSize(300,200);
            QPixmap pixmap = icon.pixmap(size, QIcon::Normal, QIcon::On);
            m_ui->m_imagePreview->setPixmap(pixmap);
          }
          );



}

MainWindow::~MainWindow()
{
    delete m_ui;
}
