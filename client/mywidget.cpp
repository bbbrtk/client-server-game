#include "mywidget.h"
#include "ui_mywidget.h"

#include <QLabel>
#include <QMessageBox>



MyWidget::MyWidget(QWidget *parent) : QWidget(parent), ui(new Ui::MyWidget) {
    ui->setupUi(this);

    connect(ui->conectBtn, &QPushButton::clicked, this, &MyWidget::connectBtnHit);
    connect(ui->hostLineEdit, &QLineEdit::returnPressed, ui->conectBtn, &QPushButton::click);
    // element, (wskaźnik na funkcje), this(?) - referencja na obiekty, patrz niżej (ui->sendBtn), zaimplementowana metoda
    connect(ui->sendBtn, &QPushButton::clicked, this, &MyWidget::sendBtnHit);
    connect(ui->msgLineEdit, &QLineEdit::returnPressed, ui->sendBtn, &QPushButton::click);

    connect(ui->chooseGameBtn, &QPushButton::clicked, this, &MyWidget::chooseGameBtnHit);
    connect(ui->startGameBtn, &QPushButton::clicked, this, &MyWidget::startGameBtnHit);
    connect(ui->quitBtn, &QPushButton::clicked, this, &MyWidget::quitBtnHit);
    // konwersja tablicy bajtów na stringi
    //char buffer[255] => QByteArray ba
    //auto txt = QString::from(fromUtf8(ba))

    //include <QLabeL>
    //QLabel *q = new QLabel(text, referencja_do_rodzica) - zeby korzystaz z automatycznego zwalniania pamieci

    //error - zajrzec do dokumentacji
    //laczenie sygnalu z funckja, a potem connect
}


/*sprawdzenie czy gniazda sieciowe są prawidłowo zamknięte */
MyWidget::~MyWidget() {
    socket->disconnectFromHost();
    delete ui;
}


/* obsługa odłączenia od servera */
void MyWidget::socketDisconnected() {
    socket->disconnectFromHost();
    ui->msgsTextEdit->append("<span style=\"color: red\">Disconnected from server</span>");
}


/* aktualizacja GUI po podłączeniu do serwera */
void MyWidget::socketConnected(){
    connTimeoutTimer->stop();
    connTimeoutTimer->deleteLater();
    ui->msgsTextEdit->append("<b>Connected</b>");
    ui->talkGroup->setEnabled(true);
}


/* odczytywanie sygnału z serwera */
void MyWidget::socketReadyRead() {
    QByteArray ba;
    ba = socket->readAll();
    ui->msgsTextEdit->append(QString::fromUtf8(ba).trimmed() ); // wyświetlanie odebranego tekstu w INFO PANEL
    analyzeRead(ba);
}


/* wyświetlenie danych */
QString MyWidget::showAndJoinInput(QList<QLineEdit*> qLineEditList){
    QString text, mainText, mainStr;
    for (int i=0; i<4; i++){

        if ( qLineEditList.at(i)->text().isEmpty() ) text = "-";
        else text = qLineEditList.at(i)->text().trimmed();

        mainText += " | " + text;
        mainStr += text.at(0);
    }
    ui->msgsTextEdit->append("<span style=\"color: blue\">" + mainText + "</span>");
    //ui->msgsTextEdit->setAlignment(Qt::AlignRight);
    return mainStr;
}


/* wyczyszczenie pól do wpisywania danych */
void MyWidget::clearInputFields(QList<QLineEdit*> qLineEditList){
    for (int i=0; i<4; i++){
        qLineEditList.at(i)->clear();
        qLineEditList.at(i)->setFocus();
    }
}

/* wyzerowanie timera */
void MyWidget::clearTimerCounting(int seconds, bool isFirstTimer){
    if( countingTimer->isActive() && !isFirstTimer){
        countingTimer->stop();
        countingTimer->deleteLater();
    }
    count = seconds;
    ui->timerNumber->display(count);
}


/* stworzenie timera */
void MyWidget::createTimer(bool isFirstTimer){
    if (isFirstTimer){
        countingTimer = new QTimer(this);
        clearTimerCounting(30, true);
        connect(countingTimer, &QTimer::timeout, this, &MyWidget::startTimerCounting);
        countingTimer->start(1000);
    }
    else{
        clearTimerCounting(10, false);
        countingTimer = new QTimer(this);
        connect(countingTimer, &QTimer::timeout, this, &MyWidget::startTimerCounting);
        countingTimer->start(1000);
    }
}


/* odliczanie timerem do zera */
void MyWidget::startTimerCounting(){
    count--;
    ui->timerNumber->display(count);
    if (count==0){
        countingTimer->stop();
        countingTimer->deleteLater();
        // dezaktywuj pole wpisywania tekstu
        ui->talkGroup->setEnabled(false);
    }
}


/* analiza komunikatu wysłanego przez serwer */
/* (!)TODO: coś ładniejszego zamiast if-else */
void MyWidget::analyzeRead(QByteArray ba) {
    QChar signalStr = QString::fromUtf8(ba).trimmed().at(0);

    // DO USUNIECIA
    if (signalStr == 'A'){
        ui->msgsTextEdit->clear();
        ui->msgsTextEdit->append("some default text");
    }

    // odbiera listę dostępnych gier
    if (signalStr == 'C'){
        ui->startGameBox->setEnabled(true);
        ui->chooseGameBtn->setEnabled(true);
        ui->chooseGameList->addItem("New Game");
        ui->chooseGameList->setCurrentRow(0);

        QString str = QString::fromUtf8(ba).trimmed();
        QString gNumber = "";

        for (int i=1; i<str.length(); i++){
            QChar n1 = QString::fromUtf8(ba).trimmed().at(i);
            if (n1 != ',') {
                gNumber += QString(n1);
            }
            else{
                ui->chooseGameList->addItem(gNumber);
                gNumber = "";
            }
        }
    }

    // potwierdzenie wyboru gry
    else if (signalStr == 'G'){
        QChar n1 = QString::fromUtf8(ba).trimmed().at(1);
        QChar n2 = QString::fromUtf8(ba).trimmed().at(2);
        QChar n3 = QString::fromUtf8(ba).trimmed().at(3);

        QString number = QString(n1)+QString(n2);
        ui->gameNumberLabel->setText(number);

        ui->startGameBox->setEnabled(false);
        // if client is Game Master
        if(n3=='M') ui->startGameBtn->setEnabled(true);

        ui->msgsTextEdit->clear();
        ui->msgsTextEdit->append("Game n." + number + " is ready to start!");
    }

    // numer gry
    else if (signalStr == 'N'){
        QChar n1 = QString::fromUtf8(ba).trimmed().at(1);
        QChar n2 = QString::fromUtf8(ba).trimmed().at(2);
        QString number = QString(n1)+QString(n2);

        ui->msgsTextEdit->append("Game number " + number + " selected.");
        ui->gameNumberLabel->setText(number);
    }

    // wyswietl punkty i wyniki
    else if (signalStr == 'P'){
        ui->msgsTextEdit->append("Check your score.");
        QChar correct = QString::fromUtf8(ba).trimmed().at(1);
        QChar points = QString::fromUtf8(ba).trimmed().at(2);
        QChar rank = QString::fromUtf8(ba).trimmed().at(3);

        QString infoBox = "Correct: " + QString(correct) + "/4, points: " + QString(points) + ", rank: " + QString(rank) + ".";

        QMessageBox::information(this, "Score", infoBox);
    }

    // start rundy i otrzymanie litery
    else if (signalStr == 'R'){
        QChar letter = QString::fromUtf8(ba).trimmed().at(1);

        ui->startGameBtn->setEnabled(false);
        ui->msgsTextEdit->clear();
        ui->msgsTextEdit->append("Round started! Your letter is " + QString(letter) + ".");
        ui->letterLabel->setText(letter);

        createTimer(true);
    }

    // 10 sekund do końca
    else if (signalStr == 'T'){
        ui->msgsTextEdit->append("10 sec left...");
        createTimer(false);
    }

    // obsluga niestandardowych komunikatów
    else if (signalStr == 'X'){

    }
}


/* obsługa błę10dów */
//void MyWidget::socketError(QAbstractSocket::SocketError socketError) {}


/* obsługa przycisku Connect */
void MyWidget::connectBtnHit(){
    ui->connectGroup->setEnabled(false);
    ui->msgsTextEdit->append("<b>Connecting to " + ui->hostLineEdit->text() + ":" + QString::number(ui->portSpinBox->value())+"</b>");

    /* zostało zrobione:
     *  - stworzyć gniazdo
     *  - połączyć zdarzenia z funkcjami je obsługującymi:
     *     • zdarzenie połączenia     (do funkcji socketConnected)
     *     • zdarzenie odbioru danych (stworzyć własną funkcję)
     *     • zdarzenie rozłączenia    (stworzyć własną funkcję)
     *     • zdarzenie błędu          (stworzyć własną funkcję) (uwaga: sprawdź składnię w pomocy)
     *  - zażądać (asynchronicznego) nawiązania połączenia
     */

    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &MyWidget::socketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &MyWidget::socketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &MyWidget::socketReadyRead);
    //connect(socket, &QTcpSocket::error ,this, &MyWidget::socketError);

    socket->connectToHost(ui->hostLineEdit->text(),ui->portSpinBox->value(),QIODevice::ReadWrite,QAbstractSocket::IPv4Protocol);
    
    connTimeoutTimer = new QTimer(this);
    connTimeoutTimer->setSingleShot(true);
    connect(connTimeoutTimer, &QTimer::timeout, [&]{
        /* TODO: przerwać próbę łączenia się do gniazda */
        connTimeoutTimer->deleteLater();
        ui->connectGroup->setEnabled(true);
        ui->msgsTextEdit->append("<b>Connect timed out</b>");
        QMessageBox::critical(this, "Error", "Connect timed out");
    });
    connTimeoutTimer->start(3000);
}


/* obsługa przycisku Choose Game */
void MyWidget::chooseGameBtnHit(){
    QString str = ui->chooseGameList->currentItem()->text();
    if(str=="New Game"){
        str = "G00";
        socket->write(str.toUtf8());
    }
    else{
        str = "G" + str;
        socket->write(str.toUtf8());
    }
}


/* obsługa przycisku Start Game (uruchamiany tylko przez mastera) */
void MyWidget::startGameBtnHit(){
    QString str = "S";
    socket->write(str.toUtf8());
}


/* obsługa przycisku Send */
void MyWidget::sendBtnHit(){
    QList<QLineEdit*> lineEditList;
    lineEditList.append(ui->msgLineEdit); lineEditList.append(ui->msgLineEdit2);
    lineEditList.append(ui->msgLineEdit3); lineEditList.append(ui->msgLineEdit4);

    QString str = showAndJoinInput(lineEditList);

    /* wysłanie danych do serwera i wyczyszczenie pól */
    socket->write(str.toUtf8());

    clearInputFields(lineEditList);
}


/* obsługa przycisku Quit */
/* (!)TODO: co należy zamknąć? */
void MyWidget::quitBtnHit(){
    MyWidget::close();
}

