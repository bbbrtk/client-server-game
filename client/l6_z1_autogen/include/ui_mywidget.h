/********************************************************************************
** Form generated from reading UI file 'mywidget.ui'
**
** Created by: Qt User Interface Compiler version 5.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MYWIDGET_H
#define UI_MYWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MyWidget
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *timerBox;
    QHBoxLayout *horizontalLayout_2;
    QLCDNumber *timerNumber;
    QLabel *gameNumberLabelLabel;
    QLabel *gameNumberLabel;
    QGroupBox *connectGroup;
    QHBoxLayout *horizontalLayout;
    QLineEdit *hostLineEdit;
    QSpinBox *portSpinBox;
    QPushButton *conectBtn;
    QGroupBox *startGameBox;
    QHBoxLayout *horizontalLayout_3;
    QListWidget *chooseGameList;
    QPushButton *chooseGameBtn;
    QPushButton *startGameBtn;
    QGroupBox *talkGroup;
    QGridLayout *gridLayout;
    QPushButton *sendBtn;
    QLineEdit *msgLineEdit4;
    QLineEdit *msgLineEdit;
    QLineEdit *msgLineEdit3;
    QLineEdit *msgLineEdit2;
    QLabel *label2City;
    QLabel *label1Country;
    QLabel *label4Animal;
    QLabel *label3Name;
    QLabel *infoLabel;
    QTextEdit *msgsTextEdit;
    QLabel *letterLabelLabel;
    QLabel *letterLabel;
    QSpacerItem *verticalSpacer;
    QPushButton *quitBtn;

    void setupUi(QWidget *MyWidget)
    {
        if (MyWidget->objectName().isEmpty())
            MyWidget->setObjectName(QStringLiteral("MyWidget"));
        MyWidget->resize(455, 671);
        verticalLayout = new QVBoxLayout(MyWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        timerBox = new QGroupBox(MyWidget);
        timerBox->setObjectName(QStringLiteral("timerBox"));
        timerBox->setMinimumSize(QSize(0, 60));
        horizontalLayout_2 = new QHBoxLayout(timerBox);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        timerNumber = new QLCDNumber(timerBox);
        timerNumber->setObjectName(QStringLiteral("timerNumber"));
        timerNumber->setFrameShape(QFrame::NoFrame);

        horizontalLayout_2->addWidget(timerNumber);

        gameNumberLabelLabel = new QLabel(timerBox);
        gameNumberLabelLabel->setObjectName(QStringLiteral("gameNumberLabelLabel"));
        gameNumberLabelLabel->setMaximumSize(QSize(120, 16777215));
        QFont font;
        font.setFamily(QStringLiteral("Courier 10 Pitch"));
        font.setPointSize(14);
        font.setBold(true);
        font.setWeight(75);
        gameNumberLabelLabel->setFont(font);
        gameNumberLabelLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_2->addWidget(gameNumberLabelLabel);

        gameNumberLabel = new QLabel(timerBox);
        gameNumberLabel->setObjectName(QStringLiteral("gameNumberLabel"));
        gameNumberLabel->setMaximumSize(QSize(60, 16777215));
        QFont font1;
        font1.setFamily(QStringLiteral("Courier 10 Pitch"));
        font1.setPointSize(22);
        font1.setBold(true);
        font1.setWeight(75);
        gameNumberLabel->setFont(font1);
        gameNumberLabel->setTextFormat(Qt::RichText);
        gameNumberLabel->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(gameNumberLabel);


        verticalLayout->addWidget(timerBox);

        connectGroup = new QGroupBox(MyWidget);
        connectGroup->setObjectName(QStringLiteral("connectGroup"));
        horizontalLayout = new QHBoxLayout(connectGroup);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        hostLineEdit = new QLineEdit(connectGroup);
        hostLineEdit->setObjectName(QStringLiteral("hostLineEdit"));

        horizontalLayout->addWidget(hostLineEdit);

        portSpinBox = new QSpinBox(connectGroup);
        portSpinBox->setObjectName(QStringLiteral("portSpinBox"));
        portSpinBox->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        portSpinBox->setMinimum(1);
        portSpinBox->setMaximum(65535);
        portSpinBox->setValue(2000);

        horizontalLayout->addWidget(portSpinBox);

        conectBtn = new QPushButton(connectGroup);
        conectBtn->setObjectName(QStringLiteral("conectBtn"));

        horizontalLayout->addWidget(conectBtn);


        verticalLayout->addWidget(connectGroup);

        startGameBox = new QGroupBox(MyWidget);
        startGameBox->setObjectName(QStringLiteral("startGameBox"));
        startGameBox->setEnabled(false);
        startGameBox->setMinimumSize(QSize(0, 100));
        startGameBox->setLayoutDirection(Qt::LeftToRight);
        horizontalLayout_3 = new QHBoxLayout(startGameBox);
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        chooseGameList = new QListWidget(startGameBox);
        chooseGameList->setObjectName(QStringLiteral("chooseGameList"));
        chooseGameList->setMaximumSize(QSize(16777215, 100));

        horizontalLayout_3->addWidget(chooseGameList);

        chooseGameBtn = new QPushButton(startGameBox);
        chooseGameBtn->setObjectName(QStringLiteral("chooseGameBtn"));
        chooseGameBtn->setEnabled(false);

        horizontalLayout_3->addWidget(chooseGameBtn);


        verticalLayout->addWidget(startGameBox);

        startGameBtn = new QPushButton(MyWidget);
        startGameBtn->setObjectName(QStringLiteral("startGameBtn"));
        startGameBtn->setEnabled(false);

        verticalLayout->addWidget(startGameBtn);

        talkGroup = new QGroupBox(MyWidget);
        talkGroup->setObjectName(QStringLiteral("talkGroup"));
        talkGroup->setEnabled(false);
        talkGroup->setMaximumSize(QSize(16777215, 400));
        gridLayout = new QGridLayout(talkGroup);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        sendBtn = new QPushButton(talkGroup);
        sendBtn->setObjectName(QStringLiteral("sendBtn"));

        gridLayout->addWidget(sendBtn, 15, 1, 1, 1);

        msgLineEdit4 = new QLineEdit(talkGroup);
        msgLineEdit4->setObjectName(QStringLiteral("msgLineEdit4"));

        gridLayout->addWidget(msgLineEdit4, 7, 1, 1, 1);

        msgLineEdit = new QLineEdit(talkGroup);
        msgLineEdit->setObjectName(QStringLiteral("msgLineEdit"));

        gridLayout->addWidget(msgLineEdit, 3, 1, 1, 1);

        msgLineEdit3 = new QLineEdit(talkGroup);
        msgLineEdit3->setObjectName(QStringLiteral("msgLineEdit3"));

        gridLayout->addWidget(msgLineEdit3, 6, 1, 1, 1);

        msgLineEdit2 = new QLineEdit(talkGroup);
        msgLineEdit2->setObjectName(QStringLiteral("msgLineEdit2"));

        gridLayout->addWidget(msgLineEdit2, 4, 1, 1, 1);

        label2City = new QLabel(talkGroup);
        label2City->setObjectName(QStringLiteral("label2City"));

        gridLayout->addWidget(label2City, 4, 0, 1, 1);

        label1Country = new QLabel(talkGroup);
        label1Country->setObjectName(QStringLiteral("label1Country"));
        label1Country->setMaximumSize(QSize(16777215, 16777215));

        gridLayout->addWidget(label1Country, 3, 0, 1, 1);

        label4Animal = new QLabel(talkGroup);
        label4Animal->setObjectName(QStringLiteral("label4Animal"));

        gridLayout->addWidget(label4Animal, 7, 0, 1, 1);

        label3Name = new QLabel(talkGroup);
        label3Name->setObjectName(QStringLiteral("label3Name"));

        gridLayout->addWidget(label3Name, 6, 0, 1, 1);

        infoLabel = new QLabel(talkGroup);
        infoLabel->setObjectName(QStringLiteral("infoLabel"));
        infoLabel->setMaximumSize(QSize(16777215, 30));
        infoLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(infoLabel, 0, 0, 1, 2);

        msgsTextEdit = new QTextEdit(talkGroup);
        msgsTextEdit->setObjectName(QStringLiteral("msgsTextEdit"));
        msgsTextEdit->setMaximumSize(QSize(16777215, 150));
        msgsTextEdit->setReadOnly(true);

        gridLayout->addWidget(msgsTextEdit, 1, 0, 1, 2);

        letterLabelLabel = new QLabel(talkGroup);
        letterLabelLabel->setObjectName(QStringLiteral("letterLabelLabel"));

        gridLayout->addWidget(letterLabelLabel, 2, 0, 1, 1);

        letterLabel = new QLabel(talkGroup);
        letterLabel->setObjectName(QStringLiteral("letterLabel"));
        letterLabel->setFont(font);

        gridLayout->addWidget(letterLabel, 2, 1, 1, 1);


        verticalLayout->addWidget(talkGroup);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        quitBtn = new QPushButton(MyWidget);
        quitBtn->setObjectName(QStringLiteral("quitBtn"));

        verticalLayout->addWidget(quitBtn);

        QWidget::setTabOrder(portSpinBox, hostLineEdit);
        QWidget::setTabOrder(hostLineEdit, conectBtn);
        QWidget::setTabOrder(conectBtn, msgsTextEdit);

        retranslateUi(MyWidget);

        QMetaObject::connectSlotsByName(MyWidget);
    } // setupUi

    void retranslateUi(QWidget *MyWidget)
    {
        MyWidget->setWindowTitle(QApplication::translate("MyWidget", "Simple TCP client", nullptr));
        timerBox->setTitle(QString());
        gameNumberLabelLabel->setText(QApplication::translate("MyWidget", "GAME N.", nullptr));
        gameNumberLabel->setText(QApplication::translate("MyWidget", "-", nullptr));
        connectGroup->setTitle(QString());
        hostLineEdit->setText(QApplication::translate("MyWidget", "localhost", nullptr));
        conectBtn->setText(QApplication::translate("MyWidget", "Connect", nullptr));
        startGameBox->setTitle(QApplication::translate("MyWidget", "CHOOSE GAME", nullptr));
        chooseGameBtn->setText(QApplication::translate("MyWidget", "Choose Game", nullptr));
        startGameBtn->setText(QApplication::translate("MyWidget", "Start Game", nullptr));
        talkGroup->setTitle(QString());
        sendBtn->setText(QApplication::translate("MyWidget", "Send", nullptr));
        label2City->setText(QApplication::translate("MyWidget", "CITY", nullptr));
        label1Country->setText(QApplication::translate("MyWidget", "COUNTRY", nullptr));
        label4Animal->setText(QApplication::translate("MyWidget", "ANIMAL", nullptr));
        label3Name->setText(QApplication::translate("MyWidget", "NAME", nullptr));
        infoLabel->setText(QApplication::translate("MyWidget", "INFO PANEL", nullptr));
        letterLabelLabel->setText(QApplication::translate("MyWidget", "CURRENT LETTER: ", nullptr));
        letterLabel->setText(QString());
        quitBtn->setText(QApplication::translate("MyWidget", "Quit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MyWidget: public Ui_MyWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MYWIDGET_H
