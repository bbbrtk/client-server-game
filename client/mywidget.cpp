#include "mywidget.h"
#include "ui_mywidget.h"

#include <QLabel>
#include <QMessageBox>

#define CONNECTION_TIMEOUT 3000 // 3 sec (timer)
#define TIMER_WAIT_B4_SEND 300000 // 0,3 sec (usleep)
#define NEXT_ROUND_AFTER 5000000 // 5 sec (usleep)

/*
    TODO
 *
 * zerowanie timera przy rozłączeniu od internetu jesli nie jest wlaczony - discon from server

*/

MyWidget::MyWidget(QWidget *parent) : QWidget(parent), ui(new Ui::MyWidget) {
    ui->setupUi(this);

    // element, (wskaźnik na funkcje), this(?) - referencja na obiekty, patrz niżej (ui->sendBtn), zaimplementowana metoda
    connect(ui->conectBtn, &QPushButton::clicked, this, &MyWidget::connectBtnHit);
    connect(ui->hostLineEdit, &QLineEdit::returnPressed, ui->conectBtn, &QPushButton::click);

    connect(ui->sendBtn, &QPushButton::clicked, this, &MyWidget::sendBtnHit);
    connect(ui->msgLineEdit, &QLineEdit::returnPressed, ui->sendBtn, &QPushButton::click);

    connect(ui->reloadGameBtn, &QPushButton::clicked, this, &MyWidget::reloadGameBtnHit);
    connect(ui->chooseGameBtn, &QPushButton::clicked, this, &MyWidget::chooseGameBtnHit);
    connect(ui->startGameBtn, &QPushButton::clicked, this, &MyWidget::startGameBtnHit);
    connect(ui->quitBtn, &QPushButton::clicked, this, &MyWidget::quitBtnHit);
}


//*******************************************
//                  SOCKETS
//*******************************************

/* check if sockets are closed */
MyWidget::~MyWidget() {
    socket->disconnectFromHost();
    delete ui;
}


/* handle: disconnect from server*/
void MyWidget::socketDisconnected() {
    socket->disconnectFromHost();
    clearTimerCounting(0);

    ui->msgsTextEdit->clear();
    ui->msgsTextEdit->append("<span style=\"color: red\">Disconnected from server</span>");

    ui->connectGroup->setEnabled(true);
    ui->gameNumberLabel->setText("-");
    ui->roundNumberLabel->setText("0");
    ui->letterLabel->setText("");
    ui->startGameBox->setEnabled(false);
    ui->startGameBtn->setEnabled(false);
    ui->talkGroup->setEnabled(false);
}


/* update GUI after socket connection */
void MyWidget::socketConnected(){
    connTimeoutTimer->stop();
    connTimeoutTimer->deleteLater();
    ui->msgsTextEdit->append("<b>Connected</b>");
    isMaster = false;
}


/* try to connect and show timeout if fail */
void MyWidget::tryToConnect(){
    connTimeoutTimer = new QTimer(this);
    connTimeoutTimer->setSingleShot(true);

    connect(
        connTimeoutTimer,
        &QTimer::timeout,
        [&]{
            connTimeoutTimer->deleteLater();
            ui->connectGroup->setEnabled(true);
            ui->msgsTextEdit->append("<b>Connect timed out</b>");
            QMessageBox::critical(this, "Error", "Connect timed out");
        }
    );
    connTimeoutTimer->start(CONNECTION_TIMEOUT); // try to connect to server for 3 sec
}


/* read from server */
void MyWidget::socketReadyRead() {
    QByteArray msg;
    msg = socket->readAll();
    analyzeRead(msg);
}


//*******************************************
//                   GUI
//*******************************************

/* modify and show user input from text fields */
QString MyWidget::showAndJoinInput(QList<QLineEdit*> qLineEditList){
    QString text, mainText, mainStr;
    for (int i=0; i<4; i++){
        if ( qLineEditList.at(i)->text().isEmpty() ) text = "-";
        else text = qLineEditList.at(i)->text().trimmed();
        mainText += " | " + text;
        mainStr += text.at(0); // only first letter
    }
    ui->msgsTextEdit->clear();
    ui->msgsTextEdit->append("<span style=\"color: blue\">" + mainText + "</span>");
    return mainStr;
}


/* clear text fields */
void MyWidget::clearInputFields(QList<QLineEdit*> qLineEditList){
    for (int i=0; i<4; i++){
        qLineEditList.at(i)->clear();
        qLineEditList.at(i)->setFocus();
    }
}


/* reset old timer and set new*/
void MyWidget::clearTimerCounting(int seconds){
    if(timerOn){
        if( countingTimer->isActive() ){
            countingTimer->stop();
            countingTimer->deleteLater();
        }
        count = seconds;
        ui->timerNumber->setText(QString::number(count));
    }
}


/* create and set timer */
void MyWidget::createTimer(bool isFirstTimer, int seconds){
    timerOn = true;
    // main timer
    if (isFirstTimer){
        countingTimer = new QTimer(this);
        clearTimerCounting(seconds);
    }
    // 10-sec timer
    else{
        clearTimerCounting(seconds);
        countingTimer = new QTimer(this);
    }

    connect(countingTimer, &QTimer::timeout, this, &MyWidget::startTimerCounting);
    countingTimer->start(1000);
}


/* timer counting to 0 */
void MyWidget::startTimerCounting(){
    count--;
    if (count>=0) ui->timerNumber->setText(QString::number(count));

    if (count==0){
        ui->talkGroup->setEnabled(false);
        if(speedState == "S") sendBtnHit(); // auto send unfinished answers
    }
    else if (count==-1){
        countingTimer->stop();
        countingTimer->deleteLater();

        if(isMaster){
            usleep(TIMER_WAIT_B4_SEND);
            QString str2 = "P";
            socket->write(str2.toUtf8());
        }
    }
}


//*******************************************
//                   HANDLE
//*******************************************

void MyWidget::handleListOfGames(QString str){
    ui->startGameBox->setEnabled(true);
    ui->chooseGameBtn->setEnabled(true);

    ui->chooseGameList->clear();
    ui->chooseGameList->addItem("New Game");
    ui->chooseGameList->setCurrentRow(0);
    QString gNumber = "";
    QChar n1;

    for (int i=1; i<str.length(); i++){
        n1 = str.at(i);
        if (n1 != ',') gNumber += QString(n1);
        else{
            ui->chooseGameList->addItem(gNumber);
            gNumber = "";
        }
    }
}


void MyWidget::handleGameNumber(QString str){
    QString number = QString(str.at(1))+QString(str.at(2));

    ui->gameNumberLabel->setText(number);
    ui->startGameBox->setEnabled(false);
    ui->msgsTextEdit->clear();
    ui->msgsTextEdit->append("Game n." + number + " is ready to start!");

    // if client is Game Master
    if(str.at(3)=='M') ui->startGameBtn->setEnabled(true);
}


void MyWidget::handleIncorrectGameNumber(){
    QString infoBox = "This game no longer exist";
    QMessageBox::information(this, "Wrong number", infoBox); // popup
}


void MyWidget::handleLateClient(QString str){
    QString letter = QString(str.at(1));
    QString clientFd = QString(str.at(2))+QString(str.at(3));

    int timerValue = ui->timerNumber->text().toInt() + 10;

    QString msg = "H" + letter + clientFd + QString::number(timerValue) + QString(str.at(4));
    socket->write(msg.toUtf8());
}


void MyWidget::handleNewRound(QString str){
    speedState = "S";
    QChar letter = str.at(1);
    QChar round = str.at(2);

    ui->startGameBtn->setEnabled(false);
    ui->talkGroup->setEnabled(true);

    ui->msgsTextEdit->append("Round started! Your letter is " + QString(letter) + ".");
    ui->letterLabel->setText(letter);
    ui->roundNumberLabel->setText(round);

    createTimer(true, 30);
}


void MyWidget::handleLateClientNewRound(QString str){
    ui->msgsTextEdit->append("Round IS ");
    speedState = "S";
    QChar letter = str.at(1);
    QChar round = str.at(4);

    ui->startGameBtn->setEnabled(false);
    ui->talkGroup->setEnabled(true);
    //ui->msgsTextEdit->clear();
    ui->msgsTextEdit->append("Round IS PENDING, be quick! Your letter is " + QString(letter) + ".");
    ui->letterLabel->setText(letter);
    ui->roundNumberLabel->setText(round);

    QString timerValue = QString(str.at(2))+QString(str.at(3));
    int timerIntValue = timerValue.toInt()-10;

    createTimer(true, timerIntValue);
}


void MyWidget::handle10secTimer(){
    //ui->msgsTextEdit->clear();
    if (speedState != "X") ui->msgsTextEdit->append("10 sec left...");

    createTimer(false, 10);
}


void MyWidget::handleWantMyScore(){
    ui->msgsTextEdit->clear();
    ui->msgsTextEdit->append("Time is up. New round and your score will show in 5 sec...");
    QString str = "W";
    socket->write(str.toUtf8());
}


void MyWidget::handleShowMyScore(QString str){
    QChar n1 = str.at(2);

    // conversion points=points-100 to maintain constant length of msg from server
    if  (n1 == '1') n1 = '0';

    QString points = QString(n1)+QString(str.at(3))+QString(str.at(4));
    QString infoBox = "<b>Last round - correct: " + QString(str.at(1)) + "/4, points: " + points + ".</b>";
    ui->msgsTextEdit->clear();
    ui->msgsTextEdit->append(infoBox);
    usleep(NEXT_ROUND_AFTER); // sleep

    if (isMaster){
        QString str = "B"; // start next round
        socket->write(str.toUtf8());
    }
}


void MyWidget::handleShowMyRank(QString str){
    QString rank = QString(str.at(1));
    QChar n1 = str.at(3);
    QChar n2 = str.at(4);
    QChar n3 = str.at(5);

    if (str.indexOf('R')!=2){
        QString rank = QString(str.at(1))+QString(str.at(2));
        QChar n1 = str.at(4);
        QChar n2 = str.at(5);
        QChar n3 = str.at(6);
    }

    if  (n1 == '1') n1 = '0';
    QString points = QString(n1)+QString(n2)+QString(n3);

    QString infoBox = "Your rank:  " + QString(rank) + " Total points: " + points;
    QMessageBox::information(this, "Rank", infoBox); // popup

    if (isMaster) isMaster = false;

    ui->roundNumberLabel->setText("0");
    ui->letterLabel->setText("");
    ui->connectGroup->setEnabled(false);
    ui->startGameBox->setEnabled(false);
    ui->startGameBtn->setEnabled(false);
    ui->talkGroup->setEnabled(false);

    // reload game list
    QString reload = "L";
    socket->write(reload.toUtf8());
    ui->gameNumberLabel->setText("-");
}


void MyWidget::handleRemovingMaster(){
    if (!isMaster){
        ui->msgsTextEdit->clear();
        ui->msgsTextEdit->append("<span style=\"color: red\">Your Game Master quit game. Game is finished.</span>");
        clearTimerCounting(0);
        ui->startGameBox->setEnabled(true);
        ui->gameNumberLabel->setText("-");
        ui->roundNumberLabel->setText("0");
        ui->letterLabel->setText("");
        ui->talkGroup->setEnabled(false);
    }
}


//*******************************************
//               ANALYZE READ
//*******************************************

/* decide what to do with server msg */
void MyWidget::analyzeRead(QByteArray ba) {
    QString str = QString::fromUtf8(ba).trimmed();
    QChar signalStr = str.at(0);

    //ui->msgsTextEdit->append(QString::fromUtf8(ba).trimmed());  // show msg from server

    // get list of games -> show
    if (signalStr == 'C') handleListOfGames(str);

    // get game number -> show
    else if (signalStr == 'G') handleGameNumber(str);

    // show popup
    else if(signalStr == 'E') handleIncorrectGameNumber();

    // late client begin -> send master timer value and current letter
    else if (signalStr == 'H') handleLateClient(str);

    // get round letter -> show and start timer
    else if (signalStr == 'R') handleNewRound(str);

    // let client get letter -> show and start timer
    else if (signalStr == 'V') handleLateClientNewRound(str);

    // get 'first successful sent' msg -> set timer to 10 sec left
    else if (signalStr == 'T') handle10secTimer();

    // get 'want score' msg -> send 'want my score' request
    else if (signalStr == 'W') handleWantMyScore();

    // get score -> show points, wait 5 sec, start new round
    else if (signalStr == 'P') handleShowMyScore(str);

    // get rank -> show rank and points, popup, exit game, reload
    else if (signalStr == 'K') handleShowMyRank(str);

    // show info about removing, show rank, reset GUI
    else if (signalStr == 'X') handleRemovingMaster();
}


//*******************************************
//                  BUTTONS
//*******************************************

void MyWidget::connectBtnHit(){
    ui->connectGroup->setEnabled(false);
    ui->msgsTextEdit->append("<b>Connecting to " + ui->hostLineEdit->text() + ":" + QString::number(ui->portSpinBox->value())+"</b>");

    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &MyWidget::socketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &MyWidget::socketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &MyWidget::socketReadyRead);
    //connect(socket, &QTcpSocket::error ,this, &MyWidget::socketError);
    socket->connectToHost(ui->hostLineEdit->text(),ui->portSpinBox->value(),QIODevice::ReadWrite,QAbstractSocket::IPv4Protocol);
    
    tryToConnect();
}


void MyWidget::reloadGameBtnHit(){
    QString str = "L";
    socket->write(str.toUtf8());
}


void MyWidget::chooseGameBtnHit(){
    QString str = ui->chooseGameList->currentItem()->text();
    QString rounds = QString::number(ui->roundSpinNumber->value());
    if(str=="New Game") str = "G00" + rounds;
    else str = "G" + str + rounds;
    socket->write(str.toUtf8());
}


void MyWidget::startGameBtnHit(){
    QString str = "S";
    isMaster = true;
    socket->write(str.toUtf8()); // send by master
}


void MyWidget::sendBtnHit(){
    QList<QLineEdit*> lineEditList;
    lineEditList.append(ui->msgLineEdit); lineEditList.append(ui->msgLineEdit2);
    lineEditList.append(ui->msgLineEdit3); lineEditList.append(ui->msgLineEdit4);

    if ( speedState == "S" && count == 0) speedState = "T"; // time passed and answers still not sent
    QString str = "A" + speedState + showAndJoinInput(lineEditList);
    str = str.toUpper();
    if (speedState != "X") socket->write(str.toUtf8());
    speedState = "X"; // answers sent, change state

    ui->talkGroup->setEnabled(false);
    clearInputFields(lineEditList);
}


void MyWidget::quitBtnHit(){
    MyWidget::close();
}


//void MyWidget::socketError(QAbstractSocket::SocketError socketError) {}
