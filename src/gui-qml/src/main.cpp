#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include "analytics.h"
#include "async-image-provider.h"
#include "functions.h"
#include "language-loader.h"
#include "loaders/gallery-search-loader.h"
#include "loaders/image-loader.h"
#include "loaders/tag-search-loader.h"
#include "main-screen.h"
#include "models/image.h"
#include "models/profile.h"
#include "models/qml-auth.h"
#include "models/qml-auth-setting-field.h"
#include "models/qml-image.h"
#include "models/qml-site.h"
#include "settings.h"
#include "share/share-utils.h"
#include "statusbar.h"
#include "syntax-highlighter-helper.h"
#include "update-checker.h"


#if defined(Q_OS_ANDROID)
	#include <QStandardPaths>
	#include <QtAndroid>
	#include "logger.h"

	bool checkPermission(const QString &perm)
	{
		auto already = QtAndroid::checkPermission(perm);
		if (already == QtAndroid::PermissionResult::Denied) {
			auto results = QtAndroid::requestPermissionsSync(QStringList() << perm);
			if (results[perm] == QtAndroid::PermissionResult::Denied) {
				return false;
			}
		}
		return true;
	}
#endif

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QGuiApplication app(argc, argv);
	app.setApplicationName("Grabber");
	app.setApplicationVersion(VERSION);
	app.setOrganizationName("Bionus");
	app.setOrganizationDomain("bionus.fr.cr");

	qRegisterMetaType<ImageLoader::Size>("ImageLoader::Size");

	qmlRegisterType<StatusBar>("StatusBar", 0, 1, "StatusBar");
	qmlRegisterType<SyntaxHighlighterHelper>("Grabber", 1, 0, "SyntaxHighlighterHelper");
	qmlRegisterType<GallerySearchLoader>("Grabber", 1, 0, "GallerySearchLoader");
	qmlRegisterType<TagSearchLoader>("Grabber", 1, 0, "TagSearchLoader");
	qmlRegisterType<ImageLoader>("Grabber", 1, 0, "ImageLoader");

	qRegisterMetaType<QSharedPointer<Image>>("QSharedPointer<Image>");
	qRegisterMetaType<Profile*>("Profile*");
	qRegisterMetaType<Settings*>("Settings*");
	qRegisterMetaType<QmlImage*>("QmlImage*");
	qRegisterMetaType<QList<QmlImage*>>("QList>QmlImage*>");
	qRegisterMetaType<QmlSite*>("QmlSite*");
	qRegisterMetaType<QList<QmlSite*>>("QList<QmlSite*>");
	qRegisterMetaType<QList<QmlAuth*>>("QList<QmlAuth*>");
	qRegisterMetaType<QList<QmlAuthSettingField*>>("QList<QmlAuthSettingField*>");

	// Copy settings files to writable directory
	const QStringList toCopy { "sites/", "themes/", "webservices/" };
	for (const QString &tgt : toCopy) {
		const QString from = savePath(tgt, true, false);
		const QString to = savePath(tgt, true, true);
		if (!QDir(to).exists() && QDir(from).exists()) {
			copyRecursively(from, to);
		}
	}
	const QStringList filesToCopy { "words.txt" };
	for (const QString &tgt : filesToCopy) {
		const QString from = savePath(tgt, true, false);
		const QString to = savePath(tgt, true, true);
		if (!QFile::exists(to) && QFile::exists(from)) {
			QFile::copy(from, to);
		}
	}

	Profile profile(savePath());
	QSettings *settings = profile.getSettings();

	// Analytics
	Analytics::getInstance().setTrackingID("UA-22768717-6");
	Analytics::getInstance().setEnabled(settings->value("send_usage_data", true).toBool());
	Analytics::getInstance().startSession();
	Analytics::getInstance().sendEvent("lifecycle", "start");

	const QUrl url(QStringLiteral("qrc:/main-screen.qml"));

	QQmlApplicationEngine engine;
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
		if (!obj && url == objUrl) {
			QCoreApplication::exit(-1);
		}
	}, Qt::QueuedConnection);

	// Define a few globals
	engine.rootContext()->setContextProperty("VERSION", QString(VERSION));
	#ifdef NIGHTLY
		engine.rootContext()->setContextProperty("NIGHTLY", true);
		engine.rootContext()->setContextProperty("NIGHTLY_COMMIT", QString(NIGHTLY_COMMIT));
	#else
		engine.rootContext()->setContextProperty("NIGHTLY", false);
		engine.rootContext()->setContextProperty("NIGHTLY_COMMIT", QString());
	#endif

	engine.addImageProvider("async", new AsyncImageProvider(&profile));

	ShareUtils shareUtils(nullptr);
	engine.rootContext()->setContextProperty("shareUtils", &shareUtils);

	MainScreen mainScreen(&profile, &shareUtils, &engine);
	engine.setObjectOwnership(&mainScreen, QQmlEngine::CppOwnership);
	engine.rootContext()->setContextProperty("backend", &mainScreen);

	Settings qmlSettings(settings);
	engine.rootContext()->setContextProperty("settings", &qmlSettings);

	UpdateChecker updateChecker(settings);
	engine.rootContext()->setContextProperty("updateChecker", &updateChecker);

	// Load translations
	LanguageLoader languageLoader(savePath("languages/", true, false));
	languageLoader.install(qApp);
	languageLoader.setLanguage(profile.getSettings()->value("language", "English").toString(), profile.getSettings()->value("useSystemLocale", true).toBool());
	engine.rootContext()->setContextProperty("languageLoader", &languageLoader);
	#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
		QObject::connect(&languageLoader, &LanguageLoader::languageChanged, &engine, &QQmlEngine::retranslate);
	#endif

	#if defined(Q_OS_ANDROID)
		if (!checkPermission("android.permission.WRITE_EXTERNAL_STORAGE")) {
			log(QStringLiteral("Android write permission not granted"), Logger::Error);
		}

		if (settings->value("Save/path").toString().isEmpty()) {
			settings->setValue("Save/path", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first() + "/Grabber");
		}
	#endif

	engine.load(url);

	#if defined(Q_OS_ANDROID)
		QtAndroid::hideSplashScreen(250);
	#endif

	return app.exec();
}
