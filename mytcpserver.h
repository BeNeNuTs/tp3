#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDebug>

#include <QTimer>

class MyTcpServer : public QObject
{
    Q_OBJECT
public:
    explicit MyTcpServer(QObject *parent = 0);
    void start(int timeBetweenSeason);

signals:

public slots:
    void newConnection();
    void sendSeason();

private:
    QTcpServer* server;
    QTcpSocket* clients[4];
    int id;
    int season;

    QTimer* timer;
};

#endif // MYTCPSERVER_H
