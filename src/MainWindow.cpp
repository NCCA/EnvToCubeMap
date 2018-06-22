#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent), m_ui(new Ui::MainWindow)
{
  m_ui->setupUi(this);

  m_gl=new  NGLScene(this);

  m_ui->s_mainWindowGridLayout->addWidget(m_gl,0,0,4,2);
  connect(m_ui->m_selectFace,SIGNAL(currentIndexChanged(int)),m_gl,SLOT(changeFace(int)));
  connect(m_ui->m_textureSize,SIGNAL(currentIndexChanged(int)),m_gl,SLOT(changeTextureSize(int)));
  connect(m_ui->m_save,SIGNAL(clicked(bool)),m_gl,SLOT(saveImages()));
  connect(m_ui->m_saveFileBase,SIGNAL(textChanged(const QString &)),m_gl,SLOT(storeFileName(const QString &)));


  connect(m_ui->m_updateCamera ,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int _i)
          {
            m_gl->updateActiveCamera(_i);
            m_gl->update();
          }
      );

  connect(m_ui->m_viewMode ,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int _i)
          {
            m_gl->updateViewMode(_i);
            m_gl->update();
          }
      );

  connect(m_ui->m_saveType ,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int _i)
          {
            m_gl->updateSaveType(_i);
          }
      );


  connect(m_ui->m_loadImage,SIGNAL(clicked(bool)),m_gl,SLOT(loadImage()));
  connect(m_gl,&NGLScene::imageUpdated,
          [=](const QImage &image)
          {
            QIcon icon;
            QSize size;
            icon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::On);
            size = QSize(200,100);
            QPixmap pixmap = icon.pixmap(size, QIcon::Normal, QIcon::On);
            m_ui->m_imagePreview->setPixmap(pixmap);
          }
          );



}

MainWindow::~MainWindow()
{
    delete m_ui;
}
