#ifndef __CLIENTSKT_H__
#define __CLIENTSKT_H__

#include <QTcpSocket>
#include <QUdpSocket>
#include <QList>

class ClientSkt : public QObject
{
	Q_OBJECT

public:
	ClientSkt(QObject *parent=0);
	virtual ~ClientSkt();

	virtual QString name() const { return "General"; };

    bool plug(const QHostAddress& ip, quint16 port, quint16 count = 1, quint16 interval = 0);
	void unplug();
	void send(const QString& data);

	const QHostAddress& addr() const { return m_ip; };
	quint16 port() const { return m_port; };

signals:
	void unpluged();

	void message(const QString& msg);
	void dumpbin(const QString& title, const char* data, quint32 len);

	void countRecv(qint32 bytes);
	void countSend(qint32 bytes);

protected:
	void dump(const char* buf, qint32 len, bool isSend);
	void show(const QString& msg);
	void setError(const QString& err);

	void recordRecv(qint32 bytes);
	void recordSend(qint32 bytes);

    virtual bool open(quint16 count, quint16 interval) =0;
	virtual void close() =0;
	virtual void send(const QByteArray& data) =0;

private:
	QHostAddress m_ip;
	quint16 m_port;

	QString m_error;
};

class ClientSktTcp : public ClientSkt
{
	Q_OBJECT

public:
	ClientSktTcp(QObject *parent=0);
	~ClientSktTcp();

	virtual QString name() const { return "TCP"; };

protected:
    virtual bool open(quint16 count, quint16 interval);
	virtual void close();
	virtual void send(const QByteArray& bin);

private slots:
	void asynConn();
	void newData();
	void closed();
	void error();

private:
	QTcpSocket m_socket;
    QList<QTcpSocket*> m_socket_list;
};

class ClientSktUdp : public ClientSkt
{
	Q_OBJECT

public:
	ClientSktUdp(QObject *parent=0);
	~ClientSktUdp();

	virtual QString name() const { return "UDP"; };

protected:
    virtual bool open(quint16 count, quint16 interval);
	virtual void close();
	virtual void send(const QByteArray& bin);

private slots:
	void asynConn();
	void newData();
	void closed();
	void error();

private:
	QUdpSocket m_socket;
    QList<QUdpSocket*> m_socket_list;
};

#endif // __CLIENTSKT_H__

