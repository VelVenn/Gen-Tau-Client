#include <qquickwindow.h>
#include <climits>
#include <cstdint>

#include "conf/version.hpp"
#include "utils/TLog.hpp"
#include "vid_render/TVidRender.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRunnable>

#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace std;

#define T_LOG_TAG_IMG "[Render Test Appsrc] "

class MockSender
{
  private:
	ifstream                       vidSrc;
	shared_ptr<gentau::TVidRender> renderer;
	jthread                        senderThread;

  public:
	void run();
	void stop();

  public:
	MockSender(string const& filePath, shared_ptr<gentau::TVidRender> renderer) : renderer(renderer)
	{
		vidSrc.open(filePath, ios::binary);
		if (!vidSrc.is_open()) { throw runtime_error("Failed to open video source file."); }
	}

	~MockSender() = default;
};

void MockSender::run()
{
	if (!renderer) {
		cerr << "Renderer not set. Cannot run sender." << endl;
		return;
	}

	renderer->getSignalView().onStateChanged.connect([](gentau::TVidRender::StateType oState,
														gentau::TVidRender::StateType nState) {
		tImgTransLogInfo(
			"Sender checked: Renderer state changed from {} to {}",
			gentau::TVidRender::getStateLiteral(oState),
			gentau::TVidRender::getStateLiteral(nState)
		);
	});

	renderer->getSignalView().onPipeError.connect([](gentau::TVidRender::IssueType iType,
													 const string&                 src,
													 const string&                 msg,
													 const string&                 debug) {
		tImgTransLogError(
			"Sender checked: Renderer Pipe Error | Type: {} | Source: {} | Message: {} | Debug: {}",
			gentau::TVidRender::getIssueTypeLiteral(iType),
			src,
			msg,
			debug
		);
	});

	renderer->play();

	senderThread = jthread([this](stop_token stoken) {
		const size_t       bufferSize = 10 * 1024;  // Must over 1K
		vector<gentau::u8> buffer(bufferSize);

		size_t bytesReadTotal = 1;
		int    sendLessOrNot  = 0;
		int    sendLessInv    = 10;
		int    sendLess       = 0;

		int postErrOrNot = 0;
		int postErrInv   = 50;

		while (!stoken.stop_requested() &&
			   (vidSrc.read(reinterpret_cast<char*>(buffer.data()), bufferSize) ||
				vidSrc.gcount() > 0)) {
			auto bytesRead = vidSrc.gcount();
			// tImgTransLogDebug("Read {} bytes from video source.", bytesRead);
			// bytesReadTotal += bytesRead;
			// tImgTransLogDebug("Total bytes read so far: {}", bytesReadTotal);

			unique_ptr<vector<gentau::u8>> frameData;

			if (++sendLessOrNot % sendLessInv == 0) {
				frameData = make_unique<vector<gentau::u8>>(
					buffer.begin(), buffer.begin() + bytesRead - sendLess
				);
			} else {
				frameData =
					make_unique<vector<gentau::u8>>(buffer.begin(), buffer.begin() + bytesRead);
			}

			if (++postErrOrNot % postErrInv == 0) { renderer->postTestError(); }

			renderer->tryPushFrame(std::move(frameData));
			// this_thread::sleep_for(chrono::milliseconds(33));
			// tImgTransLogDebug("Push down, trying to push next");
		}
		cout << "Sender thread finished." << endl;
	});
}

void MockSender::stop()
{
	senderThread.request_stop();
}

class RunningTask : public QRunnable
{
  public:
	RunningTask(shared_ptr<MockSender> sender) : m_sender(sender) {}
	~RunningTask() override = default;

	void run() override { m_sender->run(); }

  private:
	shared_ptr<MockSender> m_sender;
};

int main(int argc, char* argv[])
{
	gst_init(&argc, &argv);

	// qputenv("QSG_RENDER_TIMING", "1");  // 启用渲染时间测量
	// qputenv("QSG_RENDER_LOOP", "basic");                  // 强制基础渲染循环
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

	QGuiApplication app(argc, argv);
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	auto renderer = gentau::TVidRender::create();
	auto sender   = make_shared<MockSender>("./res/raw_sintel_720p_stream.h265", renderer);

	QQmlApplicationEngine engine;
	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection
	);
	engine.loadFromModule("Gentau.Test.Render.Appsrc", "RendTest");

	if (engine.rootObjects().isEmpty()) { qFatal("QML load failed (Root Objects empty)."); }

	QQuickWindow* rootObject = static_cast<QQuickWindow*>(engine.rootObjects().first());

	// 关键调试：打印一下，看看到底有没有找到
	QQuickItem* videoItem = rootObject->findChild<QQuickItem*>("videoItem");
	if (!videoItem) {
		qFatal("CRITICAL: Failed to find objectName 'videoItem' in QML!");
	} else {
		qDebug() << "SUCCESS: Found videoItem:" << videoItem;
	}

	renderer->linkSinkWidget(videoItem);

	rootObject->scheduleRenderJob(new RunningTask(sender), QQuickWindow::BeforeSynchronizingStage);

	return app.exec();
}