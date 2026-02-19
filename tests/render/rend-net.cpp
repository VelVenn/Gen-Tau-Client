#include "img_trans/net/TReassembly.hpp"
#include "img_trans/net/TRecv.hpp"
#include "img_trans/vid_render/TVidRender.hpp"

#include "utils/TLog.hpp"

#include <qquickwindow.h>
#include <qrunnable.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRunnable>

#include <memory>

using namespace gentau;
using namespace std;

#define T_LOG_TAG "[Render Test Rend-Net] "

class TestImgTrans
{
  private:
	TVidRender::SharedPtr  renderer;
	TReassembly::SharedPtr reassembly;
	TRecv::UniPtr          recv;

  public:
	TestImgTrans()
	{
		renderer   = TVidRender::create();
		reassembly = make_shared<TReassembly>(renderer);
		recv       = TRecv::createUni(reassembly);
	}

	~TestImgTrans() = default;

	static void init(int* argc, char** argv[]) { TVidRender::initContext(argc, argv); }

	int startRecv() { return recv->start(); }

	void stopRecv() { recv->stop(); }

	bool renderPlay() { return renderer->play(); }

	bool renderPause() { return renderer->pause(); }

	bool renderFlush() { return renderer->flush(); }

	void renderLinkWidget(QQuickItem* item) { renderer->linkSinkWidget(item); }
};

class RunningTask : public QRunnable
{
  private:
	shared_ptr<TestImgTrans> testTrans;

  public:
	RunningTask(shared_ptr<TestImgTrans> testTrans) : testTrans(testTrans) {}
	~RunningTask() override = default;

	void run() override
	{
		if (!testTrans) {
			tLogError("The TestImgTrans ptr cannot be null");
			return;
		}

		if (!testTrans->renderPlay()) {
			tLogError("Failed to start rendering");
			return;
		}

		if (testTrans->startRecv() != 0) {
			tLogError("Failed to start receiving data");
			return;
		}
	}
};

int main(int argc, char* argv[])
{
	TestImgTrans::init(&argc, &argv);

	qputenv("QSG_RENDER_TIMING", "1");                    // 启用渲染时间测量
	qputenv("QSG_RENDER_LOOP", "basic");                  // 强制基础渲染循环
	qputenv("__GL_SYNC_TO_VBLANK", "0");                  // 禁用NVIDIA VSync
	qputenv("vblank_mode", "0");                          // 禁用Mesa VSync
	qputenv("_NET_WM_BYPASS_COMPOSITOR", "1");            // 绕过X11合成器以减少延迟
	qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");  // 禁用Wayland窗口装饰以减少延迟

	if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
		qputenv("QT_QPA_PLATFORM", "wayland");  // 强制QT使用Wayland平台
		qputenv("GST_GL_PLATFORM", "egl");      // 使用EGL作为GST的GL平台
	}

	QSurfaceFormat format = QSurfaceFormat::defaultFormat();
	format.setSwapInterval(0);  // 禁用 OpenGL 交换间隔
	QSurfaceFormat::setDefaultFormat(format);

	QGuiApplication app(argc, argv);  // QGuiApplication 必须最先创建
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	auto testTrans = make_shared<TestImgTrans>();

	QQmlApplicationEngine engine;
	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection
	);
	engine.loadFromModule("Gentau.Test.Render.Net", "RendTest");

	if (engine.rootObjects().isEmpty()) { qFatal("QML load failed (Root Objects empty)."); }

	QQuickWindow* rootObject = static_cast<QQuickWindow*>(engine.rootObjects().first());

	QQuickItem* videoItem = rootObject->findChild<QQuickItem*>("videoItem");
	if (!videoItem) {
		qFatal("CRITICAL: Failed to find objectName 'videoItem' in QML!");
	} else {
		qDebug() << "SUCCESS: Found videoItem:" << videoItem;
	}

	testTrans->renderLinkWidget(videoItem);

	rootObject->scheduleRenderJob(
		new RunningTask(testTrans), QQuickWindow::BeforeSynchronizingStage
	);

	return app.exec();
}