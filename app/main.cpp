#include "img_trans/TImgTrans.hpp"
#include "utils/TLog.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QRunnable>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>

#include "GTCommViewModel.hpp"
#include <exception>

#define T_LOG_TAG "[Gentau App] "

using namespace gentau;

// Define a scheduled job for starting the renderer during OpenGL sync.
class InitGLContex : public QRunnable
{
    TImgTrans::SharedPtr imgTrans;

  public:
    void run() override
    {
        if (!imgTrans) {
            tLogError("Img Trans module is invalid");
            return;
        }

        if (!imgTrans->renderer->play()) {
            tLogError("Failed to start the renderer");
            return;
        }
    }

    InitGLContex(TImgTrans::SharedPtr imgTrans) : imgTrans(imgTrans) {}
};

int main(int argc, char* argv[])
{
    TImgTrans::initContext(&argc, &argv);

    qputenv("QSG_RENDER_LOOP", "basic");
    qputenv("__GL_SYNC_TO_VBLANK", "0");
    qputenv("vblank_mode", "0");
    qputenv("_NET_WM_BYPASS_COMPOSITOR", "1");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");

    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
        qputenv("QT_QPA_PLATFORM", "wayland");
        qputenv("GST_GL_PLATFORM", "egl");
    }

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    TImgTrans::SharedPtr imgTrans;
    try {
        imgTrans = TImgTrans::create();
    } catch (const std::exception& e) {
        tLogCritical("Fatal error during img trans init: {}", e.what());
        return -1;
    }

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );
    auto viewModel = new GTCommViewModel(&engine);
    engine.rootContext()->setContextProperty("viewModel", viewModel);
    engine.loadFromModule("Gentau.UI", "Main");

    if (engine.rootObjects().isEmpty()) {
        qFatal("QML load failed (Root Objects empty).");
        return -1;
    }

    QQuickWindow* rootObject = static_cast<QQuickWindow*>(engine.rootObjects().first());
    QQuickItem* videoItem = rootObject->findChild<QQuickItem*>("videoItem");
    
    if (!videoItem) {
        qFatal("CRITICAL: Failed to find objectName 'videoItem' in QML!");
    } else {
        qDebug() << "SUCCESS: Found videoItem:" << videoItem;
    }
    imgTrans->renderer->linkSinkWidget(videoItem);

    rootObject->scheduleRenderJob(
        new InitGLContex(imgTrans), QQuickWindow::BeforeSynchronizingStage
    );

    imgTrans->receiver->start();

    return app.exec();
}
