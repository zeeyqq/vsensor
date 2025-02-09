#include "commSimulator.h"
#include "CANCommunication.h"
#include "RS232Communication.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QRandomGenerator>
#include <QListWidgetItem>

CommSimulator::CommSimulator(QWidget *parent)
    : QWidget(parent) , canComm(nullptr), rs232Comm(nullptr){

    setupUI();

    // CAN 통신 객체 생성 및 시그널 연결
    canComm = new CANCommunication("vcan0");
    connect(canComm, &CANCommunication::dataReceived, this, &CommSimulator::dataReceived);

    // RS232 통신 객체 생성 및 시그널 연결
    rs232Comm = new RS232Communication("/dev/pts/3", "/dev/pts/2");
    connect(rs232Comm, &RS232Communication::dataReceived, this, &CommSimulator::dataReceived);

}

CommSimulator::~CommSimulator() {
    if (canComm) {
        delete canComm;  // 소멸자에서 메모리 해제
    }
    if (rs232Comm) {
        delete rs232Comm;  // 소멸자에서 메모리 해제
    }
}

void CommSimulator::setupUI() {
    timestampLabel = new QLabel("Last Data Timestamp: Not Available", this);

    // 송신 주기 설정 버튼 (CAN)
    QPushButton *canSendIntervalButton = new QPushButton("Set CAN Send Interval (ms)", this);
    canSendIntervalSpinBox = new QSpinBox(this);
    canSendIntervalSpinBox->setRange(100, 5000);
    canSendIntervalSpinBox->setValue(2000);  // 기본 2000ms
    connect(canSendIntervalButton, &QPushButton::clicked, this, &CommSimulator::setCANSendInterval);

    // 송신 주기 설정 버튼 (RS232)
    QPushButton *rs232SendIntervalButton = new QPushButton("Set RS232 Send Interval (ms)", this);
    rs232SendIntervalSpinBox = new QSpinBox(this);
    rs232SendIntervalSpinBox->setRange(100, 5000);
    rs232SendIntervalSpinBox->setValue(2000);  // 기본 2000ms
    connect(rs232SendIntervalButton, &QPushButton::clicked, this, &CommSimulator::setRS232SendInterval);

    // CAN 통신 토글 버튼
    canToggleButton = new QPushButton("Start CAN Communication", this);
    connect(canToggleButton, &QPushButton::clicked, this, &CommSimulator::toggleCANCommunication);

    // RS232 통신 토글 버튼
    rs232ToggleButton = new QPushButton("Start RS232 Communication", this);
    connect(rs232ToggleButton, &QPushButton::clicked, this, &CommSimulator::toggleRS232Communication);

    canStatusLabel = new QLabel("CAN Status: Disconnected", this);
    rs232StatusLabel = new QLabel("RS232 Status: Disconnected", this);
    receivedDataLabel = new QLabel("Received Data: None", this);
    communicationStatusLabel = new QLabel("Communication Status: Unknown", this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(timestampLabel);
    mainLayout->addWidget(canSendIntervalButton);
    mainLayout->addWidget(canSendIntervalSpinBox);
    mainLayout->addWidget(rs232SendIntervalButton);
    mainLayout->addWidget(rs232SendIntervalSpinBox);
    mainLayout->addWidget(canToggleButton);
    mainLayout->addWidget(rs232ToggleButton);
    mainLayout->addWidget(canStatusLabel);
    mainLayout->addWidget(rs232StatusLabel);
    mainLayout->addWidget(receivedDataLabel);
    mainLayout->addWidget(communicationStatusLabel);

    setLayout(mainLayout);
}

void CommSimulator::setCANSendInterval() {
    int intervalMs = canSendIntervalSpinBox->value();
    if (canComm) {
        canComm->setSendPeriod(intervalMs);  // CAN 송신 주기 설정
        communicationStatusLabel->setText("CAN Send Interval Updated");
    }
}

void CommSimulator::setRS232SendInterval() {
    int intervalMs = rs232SendIntervalSpinBox->value();
    if (rs232Comm) {
        rs232Comm->setSendPeriod(intervalMs);  // RS232 송신 주기 설정
        communicationStatusLabel->setText("RS232 Send Interval Updated");
    }
}

void CommSimulator::toggleCANCommunication() {
    try {
        if (canComm) {
            bool canSendEnabledState = canComm->canSendEnabled.load();
            // qDebug() << "Current CAN send state: " << canSendEnabledState;

            if (canSendEnabledState) {
                canComm->enableCANSend(false);
                canComm->stop();
                canToggleButton->setText("Start CAN Communication");
                canStatusLabel->setText("CAN Status: Disconnected");
            } else {
                canComm->enableCANSend(true);
                canComm->start();
                canToggleButton->setText("Stop CAN Communication");
                canStatusLabel->setText("CAN Status: Connected");
            }
        } else {
            qDebug() << "canComm is nullptr!";
        }
    } catch (const std::exception &e) {
        qDebug() << "Exception caught: " << e.what();
    }
}


void CommSimulator::toggleRS232Communication() {
    if (rs232Comm) {
        if (rs232Comm->rs232SendEnabled) {
            rs232Comm->enableRS232Send(false);
            rs232Comm->stop();
            rs232ToggleButton->setText("Start RS232 Communication");
            rs232StatusLabel->setText("RS232 Status: Disconnected");
        } else {
            rs232Comm->enableRS232Send(true);
            rs232Comm->start();
            rs232ToggleButton->setText("Stop RS232 Communication");
            rs232StatusLabel->setText("RS232 Status: Connected");
        }
    }
}

void CommSimulator::dataReceived(const QString &data) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    timestampLabel->setText("Last Data Timestamp: " + timestamp);

    QString formattedData = QString("Received: %1 @ %2").arg(data).arg(timestamp);
    receivedDataLabel->setText(formattedData);
}
