#ifndef CANCOMMUNICATION_H
#define CANCOMMUNICATION_H

#include "HardwareCommunication.h"
#include <string>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cstring>
#include <unistd.h>
#include <random>
#include <vector>
#include <map>
#include <mutex>
#include <unordered_map>

class CANCommunication : public HardwareCommunication {
public:
    explicit CANCommunication(const std::string& interfaceName);
    ~CANCommunication();

    void sendData(const can_frame& frame);

protected:
    void run() override;

private:
    std::string interfaceName;
    int socket_fd{-1};
    std::default_random_engine randomEngine;
    std::map<int, std::pair<float, float>> idDataRanges;
    std::mutex dataMutex;

    std::unordered_map<int, std::string> idToDataType;
    std::atomic<int> connectionStatus{0};  // 0: 끊김, 1: 미흡, 2: 양호

    std::atomic<bool> canSendEnabled{true}; // CAN 송신 활성화 여부
    std::atomic<int> sendPeriodMs{2000}; // 송신 주기 (기본 100ms)
    std::chrono::steady_clock::time_point lastReceiveTime;  // 마지막 수신 시간 기록

    void initializeSocket(int& socket_fd, struct sockaddr_can& addr, struct ifreq& ifr);
    float generateRandomValue(float minValue, float maxValue);
    void sendIMUData();
    int generateRandomCANID();
    void handleIncomingData();
    void processReceivedData(const can_frame& frame);
    void displayDataMeaning(const can_frame& frame);
    void updateConnectionStatus();
    void enableCANSend(bool);
    void setSendPeriod(int);
};

#endif // CANCOMMUNICATION_H
