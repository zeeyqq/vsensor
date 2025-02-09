#ifndef COMMSIMULATOR_H
#define COMMSIMULATOR_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QListWidget>
#include "CANCommunication.h"
#include "RS232Communication.h"

class CANCommunication;
class RS232Communication;

class CommSimulator : public QWidget {
    Q_OBJECT

public:
    explicit CommSimulator(QWidget *parent = nullptr);
    ~CommSimulator();

private slots:
    void setCANSendInterval();          // CAN 송신 주기 설정
    void setRS232SendInterval();        // RS232 송신 주기 설정
    void toggleCANCommunication();     // CAN 통신 온오프
    void toggleRS232Communication();   // RS232 통신 온오프
    // void handleDisconnection();        // 재연결 시도
    void dataReceived(const QString &data); // 데이터 수신 시그널 처리

private:
    bool canActive = false;
    bool rs232Active = false;

    QLabel *timestampLabel;             // 타임스탬프 라벨
    QLabel *canStatusLabel;             // CAN 상태 라벨
    QLabel *rs232StatusLabel;           // RS232 상태 라벨
    QLabel *receivedDataLabel;          // 수신 데이터 라벨
    QLabel *communicationStatusLabel;   // 통신 상태 라벨

    QPushButton *canToggleButton;       // CAN 토글 버튼
    QPushButton *rs232ToggleButton;     // RS232 토글 버튼
    QSpinBox *canSendIntervalSpinBox;   // CAN 송신 주기 설정 스핀 박스
    QSpinBox *rs232SendIntervalSpinBox; // RS232 송신 주기 설정 스핀 박스

    CANCommunication *canComm;     // CAN 통신 객체
    RS232Communication *rs232Comm; // RS232 통신 객체


    void setupUI();
};

#endif // COMMSIMULATOR_H
