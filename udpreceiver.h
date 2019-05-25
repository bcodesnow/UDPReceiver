#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QImage>
#include <QDebug>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <ctime>
#include <pthread.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <qimagetoqml.h>

#define SERVERPORT 8887
#define BUFFLEN 1452

using namespace std;
using namespace cv;

class UDPReceiver: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QImage liveImg READ getLiveImg NOTIFY liveImgChanged)

signals:
    void liveImgChanged (QImage img);

public:
    QImage getLiveImg()
    {
        return m_qimage;
    }
    // SWAP BGR TO RGB
    QImage mat_to_qimage_ref(const cv::Mat3b &src) {
        QImage dest(src.cols, src.rows, QImage::Format_ARGB32);
        for (int y = 0; y < src.rows; ++y) {
            const cv::Vec3b *srcrow = src[y];
            QRgb *destrow = (QRgb*)dest.scanLine(y);
            for (int x = 0; x < src.cols; ++x) {
                destrow[x] = qRgba(srcrow[x][2], srcrow[x][1], srcrow[x][0], 255);
            }
        }
        return dest;
    }

    // this is crazy shit that is possible to call thread create in a constructor on a non static member function!
    UDPReceiver(QObject *parent = nullptr, bool* t_start_flag = nullptr) : QObject(parent),
        start_flag(t_start_flag) {
        start_flag = t_start_flag;
        if (pthread_create(&this->thread_rec, NULL, (THREADFUNCPTR) &UDPReceiver::receiverThread, this))
            qDebug()<<"now we should quit"; //quit("\n--> pthread_create failed.", 1);
        if (pthread_create(&this->thread_proc, NULL, (THREADFUNCPTR) &UDPReceiver::processingThread, this))
            qDebug()<<"now we should quit"; //quit("\n--> pthread_create failed.", 1);
    }

private:
    int                 clientSock;
    int                 key;
    pthread_t           thread_rec;
    pthread_t           thread_proc;
    bool*               start_flag;
    QImage              m_qimage;
    vector<uchar>       m_jpgBuf;
    bool                m_lastMat_is_fresh;
    Mat                 m_lastMat;
    pthread_mutex_t     m_lastMat_mutex = PTHREAD_MUTEX_INITIALIZER;
    QImageToQml*        m_refToImageProvider;

    typedef void * (*THREADFUNCPTR)(void *);

    void* processingThread(void* arg)
    {
        qDebug()<<"Processing Starts";
        /* make this thread cancellable using pthread_cancel() */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        Q_UNUSED(arg);
        struct timespec nsleep;
        nsleep.tv_nsec = 750000;
        Mat someLocalMat;
        while (1)
        {
            pthread_mutex_lock(&m_lastMat_mutex);
            if (m_lastMat_is_fresh)
            {
                someLocalMat = m_lastMat;
                m_lastMat_is_fresh = 0;
            }
            pthread_mutex_unlock(&m_lastMat_mutex);
            nanosleep(0, &nsleep);
        }
    }

    void* receiverThread(void* arg)
    {
        Q_UNUSED(arg);
        struct sockaddr_in receive_addr;
        int trueflag = 1;
        int fd;

        qDebug()<<"Receiver Starts";

        if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            qDebug("ERROR: socket!"); //errno_abort("socket");

        /* make this thread cancellable using pthread_cancel() */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &trueflag, sizeof trueflag) < 0)
        {
            qDebug()<<"ERROR: setsockopt!"; //errno_abort("setsockopt"); // one of them..
        }

        memset(&receive_addr, 0, sizeof receive_addr);
        receive_addr.sin_family = AF_INET;

        //htons() converts the unsigned short integer hostshort from host byte order to network byte order.
        receive_addr.sin_port = (in_port_t) htons(SERVERPORT);

        //only to receive
        receive_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        //inet_aton() converts the Internet host address cp from the IPv4 numbers-and-dots notation into binary form (in network byte order) and stores it in the structure that inp points to.
        if (inet_aton("192.168.8.204", &receive_addr.sin_addr) == 0)
            //if (inet_aton("127.0.0.1", &send_addr.sin_addr) == 0)
            qDebug()<<"Inet Aton Failed"
                      ;
        int slen=sizeof(receive_addr);
        socklen_t socksize = sizeof(receive_addr);

        char buf[BUFFLEN];
        if( bind(fd , (struct sockaddr*) &receive_addr, slen ) == -1)
        {
            qDebug("Bind!");
        }

        struct timespec nsleep;
        nsleep.tv_nsec = 750000;

        *start_flag = true;

        while(1)
        {
            int i = 0;
            int recSize = recvfrom(fd, buf, BUFFLEN, 0, (struct sockaddr*) &receive_addr, &socksize );
            if (recSize < 0)
            {
                qDebug("ERROR RECEIVE");
            }
            else if ( recSize == 1452 )
            {
                m_jpgBuf.insert(m_jpgBuf.end(), buf, buf+recSize);
            }
            else if (recSize < 1452)
            {
                //last frame
                m_jpgBuf.insert(m_jpgBuf.end(), buf, buf+recSize); //use vector to receive and only insert the part of buff which arrived..
                Mat decodedImage  =  imdecode( m_jpgBuf, 1 );
                if ( !decodedImage.data == NULL )
                {
                    if (pthread_mutex_trylock(&m_lastMat_mutex))
                    {
                        m_lastMat = decodedImage;
                        m_lastMat_is_fresh = true;
                        pthread_mutex_unlock(&m_lastMat_mutex);
                    }
                    m_qimage = mat_to_qimage_ref(decodedImage);
                    emit liveImgChanged(m_qimage);
                }
                else
                {
                    qDebug()<<"Error";
                }
                m_jpgBuf.clear();
            }
            /* we did it.. */
            /* have we terminated yet? */
            pthread_testcancel();
            /* no, take a rest for a while */
            nanosleep(0, &nsleep);
        }
    }
};
#endif // UDPRECEIVER_H
