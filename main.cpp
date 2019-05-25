#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QImage>
#include <QObject>
#include <QQmlContext>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <ctime>
#include <QDebug>

#include <iostream>
#include <pthread.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "udpreceiver.h"
#include "qimagetoqml.h"

#define SERVERPORT 8887
#define BUFFLEN 1452

using namespace std;
using namespace cv;


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    //Q_UNUSED(argc);Q_UNUSED(argv);
    bool capture_is_running = false;

    UDPReceiver *rec;
    QImageToQml itq;
    // creating this object starts a thread and is ready to receive payloads
    rec = new UDPReceiver(nullptr, &capture_is_running);
    qmlRegisterType<QImageToQml>("QImageToQml", 1, 0, "QImageToQml");
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("udpReceiver", rec);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
