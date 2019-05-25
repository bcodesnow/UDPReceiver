import QtQuick 2.10
import QtQuick.Controls 2.3
import "."
import QImageToQml 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("UDP Video Stream")

    QImageToQml {
        id: testImg
        width: 320
        height: 240
        anchors.centerIn: parent
    }


    Connections
    {
        target: udpReceiver
        onLiveImgChanged:
        {
            testImg.setImage(img);
        }
    }

}
