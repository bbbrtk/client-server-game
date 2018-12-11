#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QLineEdit>
#include <unistd.h>

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
    /* TODO: Dodać zmienną reprezentująca gniazdo - wskaźnik na  QTcpSocket */
    QTimer * connTimeoutTimer;
    QTimer * countingTimer; // clock
    QTcpSocket * socket;

    void connectBtnHit();
    void sendBtnHit();
    void chooseGameBtnHit();
    void startGameBtnHit();
    void quitBtnHit();

    void socketDisconnected();

    void socketReadyRead();
    void analyzeRead(QByteArray);

    void socketError();
    
    void socketConnected();

    void createTimer(bool);
    void clearTimerCounting(int, bool);
    void startTimerCounting();

    QString showAndJoinInput(QList<QLineEdit*>);
    void clearInputFields(QList<QLineEdit*>);

private:
    Ui::MyWidget * ui;
    int count;
    QString speedState; // F - first, S - second, T - third, X - answers sent
    bool isMaster;


};

#endif // MYWIDGET_H
