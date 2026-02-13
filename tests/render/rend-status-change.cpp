#include <qquickwindow.h>

#include "utils/TLog.hpp"
#include "vid_render/TVidRender.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRunnable>

#include <memory>
using namespace std;
using namespace gentau;

class VideoController : public QObject
{
	Q_OBJECT
  public:
	explicit VideoController(
		std::shared_ptr<gentau::TVidRender> renderer, QObject *parent = nullptr
	) :
		QObject(parent),
		m_renderer(renderer)
	{}

	Q_INVOKABLE void play()
	{
		if (m_renderer) m_renderer->play();
	}
	Q_INVOKABLE void pause()
	{
		if (m_renderer) m_renderer->pause();
	}
	Q_INVOKABLE void flush()
	{
		if (m_renderer) m_renderer->flush();
	}
	Q_INVOKABLE void reset()
	{
		if (m_renderer) m_renderer->reset();
	}
	Q_INVOKABLE void stop()
	{
		if (m_renderer) m_renderer->stopPipeline();
	}

  private:
	std::shared_ptr<gentau::TVidRender> m_renderer;
};

class RunningTask : public QRunnable
{
  public:
	RunningTask(shared_ptr<TVidRender> pipe_data) : m_sender(pipe_data) {}
	~RunningTask() override = default;

	void run() override { m_sender->play(); }

  private:
	shared_ptr<TVidRender> m_sender;
};

int main(int argc, char *argv[])
{
	gentau::TVidRender::initContext(&argc, &argv);

	// qputenv("QSG_RENDER_TIMING", "1");  // 启用渲染时间测量
	// qputenv("QSG_RENDER_LOOP", "basic");                  // 强制基础渲染循环
	// qputenv("__GL_SYNC_TO_VBLANK", "0");                  // 禁用NVIDIA VSync
	// qputenv("vblank_mode", "0");                          // 禁用Mesa VSync
	// qputenv("_NET_WM_BYPASS_COMPOSITOR", "1");            // 绕过X11合成器以减少延迟
	// qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");  // 禁用Wayland窗口装饰以减少延迟

	if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
		qputenv("QT_QPA_PLATFORM", "wayland");  // 强制QT使用Wayland平台
		qputenv("GST_GL_PLATFORM", "egl");      // 使用EGL作为GST的GL平台
	}

	// QSurfaceFormat format = QSurfaceFormat::defaultFormat();
	// format.setSwapInterval(0);  // 禁用 OpenGL 交换间隔
	// QSurfaceFormat::setDefaultFormat(format);

	QGuiApplication app(argc, argv);
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	auto vidRenderer = TVidRender::create("./res/raw_lg_4k_60fps.h265");

	QQmlApplicationEngine engine;
	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection
	);
	VideoController controller(vidRenderer);
	engine.rootContext()->setContextProperty("videoController", &controller);
	engine.loadFromModule("Gentau.Test.Render.StatusChange", "StatusTest");

	auto rootObject = static_cast<QQuickWindow *>(engine.rootObjects().first());
	auto videoItem  = rootObject->findChild<QQuickItem *>("videoItem");
	vidRenderer->linkSinkWidget(videoItem);

	rootObject->scheduleRenderJob(
		new RunningTask(vidRenderer), QQuickWindow::BeforeSynchronizingStage
	);

	return app.exec();
}

#include "rend-status-change.moc"