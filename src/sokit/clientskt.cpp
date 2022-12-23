#include <QTcpSocket>
#include "toolkit.h"
#include "clientskt.h"

#define MAXBUFFER 1024*1024

ClientSkt::ClientSkt(QObject *parent)
: QObject(parent),m_port(0)
{
}

ClientSkt::~ClientSkt()
{
}

bool ClientSkt::plug(const QHostAddress& ip, quint16 port, quint16 count, quint16 interval)
{
	m_ip   = ip;
	m_port = port;

	m_error.clear();

    return open(count, interval);
}

void ClientSkt::unplug()
{
	close();

	emit unpluged();
}

void ClientSkt::setError(const QString& err)
{
	m_error = err;
}

void ClientSkt::recordRecv(qint32 bytes)
{
	emit countRecv(bytes);
}

void ClientSkt::recordSend(qint32 bytes)
{
	emit countSend(bytes);
}

void ClientSkt::send(const QString& data)
{
	QString err;
	QByteArray bin;

	if (!TK::ascii2bin(data, bin, err))
	{
		show("bad data format to send: "+err);
		return;
	}

	send(bin);
}

void ClientSkt::dump(const char* buf, qint32 len, bool isSend)
{
	emit dumpbin(QString("DAT %1").arg(isSend?"<---":"--->"), buf, (quint32)len);
}

void ClientSkt::show(const QString& msg)
{
	emit message(msg);
}

ClientSktTcp::ClientSktTcp(QObject *parent)
:ClientSkt(parent)
{
}

ClientSktTcp::~ClientSktTcp()
{
}

bool ClientSktTcp::open(quint16 count, quint16 interval)
{
    for(quint16 i=0; i<count; i++){
        QTcpSocket *s = new QTcpSocket();
        connect(s, SIGNAL(readyRead()), this, SLOT(newData()));
        connect(s, SIGNAL(disconnected()), this, SLOT(closed()));
        connect(s, SIGNAL(connected()), this, SLOT(asynConn()));
        connect(s, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

        s->connectToHost(addr(), port());
        m_socket_list.append(s);
    }
//    connect(&m_socket, SIGNAL(readyRead()), this, SLOT(newData()));
//    connect(&m_socket, SIGNAL(disconnected()), this, SLOT(closed()));
//    connect(&m_socket, SIGNAL(connected()), this, SLOT(asynConn()));
//    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

//    m_socket.connectToHost(addr(), port());

	return true;
}

void ClientSktTcp::close()
{
//	m_socket.close();
//	m_socket.disconnect(this);
    for(QList<QTcpSocket*>::iterator i = m_socket_list.begin(); i != m_socket_list.end(); ++i) {
        (**i).close();
        (**i).disconnect(this);
        delete *i;
    }
    m_socket_list.clear();
}

void ClientSktTcp::error()
{
	QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());

	show(QString("TCP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	unplug();
}

void ClientSktTcp::asynConn()
{
	show(QString("TCP connection to %1:%2 opened!")
		.arg(addr().toString()).arg(port()));
}

void ClientSktTcp::closed()
{
	show(QString("TCP connection closed!"));
}

void ClientSktTcp::newData()
{
	QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
	if (!s) return;

	qint64 bufLen = s->bytesAvailable();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	qint64 readLen = 0;
	qint64 ioLen = s->read(buf, bufLen);

	while (ioLen > 0)
	{
		readLen += ioLen;
		ioLen = s->read(buf+readLen, bufLen-readLen);
	}

	if (ioLen >= 0)
	{
		recordRecv(readLen);
		dump(buf, readLen, false);
	}

	TK::releaseBuffer(buf);
}

void ClientSktTcp::send(const QByteArray& bin)
{
	const char *  src = bin.constData(); 
	qint64 srcLen = bin.length();

    for(QList<QTcpSocket*>::iterator i = m_socket_list.begin(); i != m_socket_list.end(); ++i) {
        qint64 writeLen = 0;
        qint64 ioLen = (*i)->write(src, srcLen);

        while (ioLen > 0)
        {
            writeLen += ioLen;
            ioLen = (*i)->write(src+writeLen, srcLen-writeLen);
        }

        if (writeLen != srcLen)
        {
            show(QString("failed to send data to %1:%2 [%3]")
                .arg(addr().toString()).arg(port()).arg(writeLen));
            return;
        }

        recordSend(writeLen);
        dump(src, srcLen, true);
    }
//	qint64 writeLen = 0;
//	qint64 ioLen = m_socket.write(src, srcLen);

//	while (ioLen > 0)
//	{
//		writeLen += ioLen;
//		ioLen = m_socket.write(src+writeLen, srcLen-writeLen);
//	}

//	if (writeLen != srcLen)
//	{
//		show(QString("failed to send data to %1:%2 [%3]")
//			.arg(addr().toString()).arg(port()).arg(writeLen));
//		return;
//	}

//	recordSend(writeLen);
//	dump(src, srcLen, true);
}

ClientSktUdp::ClientSktUdp(QObject *parent)
:ClientSkt(parent)
{
}

ClientSktUdp::~ClientSktUdp()
{
}

void ClientSktUdp::asynConn()
{
	show(QString("UDP channel to %1:%2 opened!")
		.arg(addr().toString()).arg(port()));
}

void ClientSktUdp::closed()
{
	show(QString("UDP channel closed!"));
}

void ClientSktUdp::close()
{
	m_socket.close();
	m_socket.disconnect(this);

    for(QList<QUdpSocket*>::iterator i = m_socket_list.begin(); i != m_socket_list.end(); ++i) {
        (**i).close();
        (**i).disconnect(this);
        delete *i;
    }
    m_socket_list.clear();
}

void ClientSktUdp::error()
{
	QUdpSocket* s = qobject_cast<QUdpSocket*>(sender());

	show(QString("UDP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	unplug();
}

bool ClientSktUdp::open(quint16 count, quint16 interval)
{
    for(quint16 i=0; i<count; i++){
        QUdpSocket *s = new QUdpSocket();
        connect(s, SIGNAL(readyRead()), this, SLOT(newData()));
        connect(s, SIGNAL(disconnected()), this, SLOT(closed()));
        connect(s, SIGNAL(connected()), this, SLOT(asynConn()));
        connect(s, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

        s->connectToHost(addr(), port());
        m_socket_list.append(s);
    }
//	connect(&m_socket, SIGNAL(readyRead()), this, SLOT(newData()));
//	connect(&m_socket, SIGNAL(disconnected()), this, SLOT(closed()));
//	connect(&m_socket, SIGNAL(connected()), this, SLOT(asynConn()));
//	connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

//	m_socket.connectToHost(addr(), port());

	return true;
}

void ClientSktUdp::newData()
{
	QUdpSocket* s = qobject_cast<QUdpSocket*>(sender());
	if (!s) return;

	qint64 bufLen = s->bytesAvailable();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	qint64 readLen = 0;
	qint64 ioLen = s->read(buf, bufLen);

	while (ioLen > 0)
	{
		readLen += ioLen;
		ioLen = s->read(buf+readLen, bufLen-readLen);
	}

	if (ioLen >= 0)
	{
		recordRecv(readLen);
		dump(buf, readLen, false);
	}

	TK::releaseBuffer(buf);
}

void ClientSktUdp::send(const QByteArray& bin)
{
	const char *  src = bin.constData(); 
	qint64 srcLen = bin.length();

    for(QList<QUdpSocket*>::iterator i = m_socket_list.begin(); i != m_socket_list.end(); ++i) {
        qint64 writeLen = 0;
        qint64 ioLen = (*i)->write(src, srcLen);

        while (ioLen > 0)
        {
            writeLen += ioLen;
            ioLen = (writeLen >= srcLen) ? 0 :
                    (*i)->write(src+writeLen, srcLen-writeLen);
        }

        if (writeLen != srcLen)
        {
            show(QString("failed to send data to %1:%2 [%3]")
                .arg(addr().toString()).arg(port()).arg(writeLen));
            return;
        }

        recordSend(writeLen);
        dump(src, srcLen, true);
    }
}

