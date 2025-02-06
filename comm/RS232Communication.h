#ifndef RS232COMMUNICATION_H
#define RS232COMMUNICATION_H

#include "HardwareCommunication.h"
#include <string>
#include <thread>
#include <random>
#include <iostream>
#include <mutex>

class RS232Communication : public HardwareCommunication {
public:
    RS232Communication(const std::string& port, int intervalMs = 500);
    ~RS232Communication();

    void sendData();  // 데이터 송신 함수
    void receiveData();  // 데이터 수신 함수
    void monitorConnection();  // 연결 상태 모니터링 함수

protected:
    void run() override;  // 통신 루프

private:
    std::string port;  // RS232 포트
    int intervalMs;  // 송신 주기 (ms)
    bool connectionStatus;  // 연결 상태
    std::mutex dataMutex;  // 수신된 데이터 보호용 뮤텍스
    std::string lastReceivedTime;  // 마지막 수신 시간
    std::chrono::system_clock::time_point lastReceivedTimestamp; // 마지막 수신 타임스탬프
    std::vector<std::string> receivedData;  // 수신된 데이터 저장

    std::string generateNMEAData();  // NMEA 데이터 생성 함수
    std::string generateGPGGA();  // GPGGA 포맷 데이터 생성
    std::string generateGPHDT();  // GPHDT 포맷 데이터 생성
    std::string generateGPVTG();  // GPVTG 포맷 데이터 생성
    std::string formatDouble(double val);               // 실수 포맷팅 함수
    int generateRandomInt(int min, int max);
    double generateRandomDouble(double min, double max);
    std::string getCurrentTimestamp();  // 현재 시간 타임스탬프 생성
    void printNMEAMessage(const std::string&); // 메시지 포맷 변경 후 출력
};

#endif  // RS232COMMUNICATION_H
