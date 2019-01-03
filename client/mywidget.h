#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QLineEdit>
#include <unistd.h>
#include <time.h>

namespace Ui {
class MyWidget;
}

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget *parent = 0);
    ~MyWidget();

protected:
    QTimer * connTimeoutTimer; // connection timeout
    QTimer * countingTimer; // game clock
    QTcpSocket * socket;

    void connectBtnHit();
    void reloadGameBtnHit();
    void chooseGameBtnHit();
    void startGameBtnHit();
    void sendBtnHit();
    void quitBtnHit();

    void socketDisconnected();
    void socketReadyRead();
    void socketError(); // unused
    void socketConnected();
    void tryToConnect();

    void createTimer(bool, int);
    void clearTimerCounting(int);
    void startTimerCounting();

    void analyzeRead(QByteArray);
    void handleListOfGames(QString);
    void handleGameNumber(QString);
    void handleLateClient(QString);
    void handleNewRound(QString);
    void handleLateClientNewRound(QString);
    void handle10secTimer();
    void handleWantMyScore();
    void handleShowMyScore(QString);
    void handleShowMyRank(QString);
    void handleRemovingMaster();

    QString showAndJoinInput(QList<QLineEdit*>);
    void clearInputFields(QList<QLineEdit*>);

private:
    Ui::MyWidget * ui;
    int count;
    QString speedState; // F - first, S - second, T - third, X - answers sent
    bool isMaster;


};

#endif // MYWIDGET_H
