#include "CANCommunication.h"
#include "RS232Communication.h"
#include "commSimulator.h"
#include <QApplication>
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CommSimulator w;  // CommSimulator 클래스 인스턴스 생성
    w.show();  // UI를 표시
    return a.exec();  // 이벤트 루프 시작
}

/*
int main() {
    // CAN 통신 시작
    CANCommunication canComm("vcan0");
    canComm.start();

    // RS232 통신 시작
    RS232Communication rs232Comm("/dev/pts/3", "/dev/pts/2", 1000);
    rs232Comm.start();  // RS232 시작

    // exec for 10 secs..
    std::this_thread::sleep_for(std::chrono::seconds(10));

    canComm.stop();
    rs232Comm.stop();

    std::cout << "END" << std::endl;

    return 0;
}
*/
