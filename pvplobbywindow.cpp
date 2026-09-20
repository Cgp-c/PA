#include "pvplobbywindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QCloseEvent>
#include <QMessageBox>

static const char* DARK_QSS =
    "QWidget { background: #181822; color: #d8d8e8; font-size: 13px; }"
    "QLabel#title { font-size: 22px; font-weight: bold; color: #ffd240; }"
    "QLabel#hint { color: #a0a0b4; font-size: 11px; }"
    "QLineEdit { background: #242432; border: 1px solid #55556a; border-radius: 6px;"
    " padding: 6px; color: white; font-size: 15px; }"
    "QPushButton { border-radius: 6px; padding: 10px 16px; font-weight: bold; }"
    "QPushButton#create { background: #2a6e3c; color: #e0ffe8; }"
    "QPushButton#join   { background: #4a6ec8; color: #e0ecff; }"
    "QPushButton:disabled { background: #3a3a44; color: #8888a0; }";

PvpLobbyWindow::PvpLobbyWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("联机对战 - Synera"));
    setFixedSize(420, 300);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
    setStyleSheet(DARK_QSS);
    buildUi();
}

PvpLobbyWindow::~PvpLobbyWindow() = default;

void PvpLobbyWindow::buildUi()
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(12);

    QLabel* title = new QLabel(QString::fromUtf8("局域网联机对战"), this);
    title->setObjectName("title");
    lay->addWidget(title, 0, Qt::AlignHCenter);

    QLabel* hint = new QLabel(QString::fromUtf8("两台电脑连同一局域网（或同一台电脑用 127.0.0.1 测试）。\n一方创建房间，另一方输入对方 IP 加入。"), this);
    hint->setObjectName("hint");
    hint->setWordWrap(true);
    lay->addWidget(hint);

    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText(QString::fromUtf8("主机 IP 地址，如 192.168.1.100 或 127.0.0.1"));
    lay->addWidget(m_ipEdit);

    QHBoxLayout* btns = new QHBoxLayout;
    m_createBtn = new QPushButton(QString::fromUtf8("创建房间（我是主机）"), this);
    m_createBtn->setObjectName("create");
    m_joinBtn = new QPushButton(QString::fromUtf8("加入房间"), this);
    m_joinBtn->setObjectName("join");
    btns->addWidget(m_createBtn);
    btns->addWidget(m_joinBtn);
    lay->addLayout(btns);

    m_addrLabel = new QLabel(this);
    m_addrLabel->setWordWrap(true);
    lay->addWidget(m_addrLabel);

    m_statusLabel = new QLabel(this);
    lay->addWidget(m_statusLabel);

    lay->addStretch(1);

    connect(m_createBtn, &QPushButton::clicked, this, &PvpLobbyWindow::onCreateRoom);
    connect(m_joinBtn, &QPushButton::clicked, this, &PvpLobbyWindow::onJoinRoom);
}

void PvpLobbyWindow::setStatus(const QString& text, const QColor& color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color.name()));
}

void PvpLobbyWindow::onCreateRoom()
{
    if (m_socket) return;
    // 清理旧 server（同进程二次建房：释放端口 + 防止孤儿监听）
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    m_isHost = true;
    m_server = new QTcpServer(this);
    if (!m_server->listen(QHostAddress::AnyIPv4, PVP_PORT)) {
        setStatus(QString::fromUtf8("监听失败：") + m_server->errorString(), QColor(255, 90, 90));
        m_server->deleteLater();
        m_server = nullptr;
        return;
    }
    connect(m_server, &QTcpServer::newConnection, this, &PvpLobbyWindow::onHostNewConnection);

    // 枚举本机局域网 IPv4 地址展示给对方
    QString ips;
    const auto addrs = QNetworkInterface::allAddresses();
    for (const QHostAddress& a : addrs) {
        if (a.protocol() == QAbstractSocket::IPv4Protocol
            && a != QHostAddress::LocalHost && !a.isLoopback()) {
            if (!ips.isEmpty()) ips += " / ";
            ips += a.toString();
        }
    }
    if (ips.isEmpty()) ips = "127.0.0.1";
    m_addrLabel->setText(QString::fromUtf8("本机 IP：%1\n端口：%2\n把上面的 IP 告诉对方，等待加入…")
                             .arg(ips).arg(PVP_PORT));
    setStatus(QString::fromUtf8("房间已创建，等待对方加入…"), QColor(255, 210, 60));
    m_createBtn->setEnabled(false);
    m_joinBtn->setEnabled(false);
    m_ipEdit->setEnabled(false);
}

void PvpLobbyWindow::onJoinRoom()
{
    if (m_socket) return;
    QString ip = m_ipEdit->text().trimmed();
    if (ip.isEmpty()) {
        setStatus(QString::fromUtf8("请先输入主机 IP"), QColor(255, 160, 60));
        return;
    }
    m_isHost = false;
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &PvpLobbyWindow::onSocketConnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PvpLobbyWindow::onSocketError);
    m_socket->connectToHost(QHostAddress(ip), PVP_PORT);
    setStatus(QString::fromUtf8("正在连接 %1:%2 …").arg(ip).arg(PVP_PORT), QColor(255, 210, 60));
    m_createBtn->setEnabled(false);
    m_joinBtn->setEnabled(false);
    m_ipEdit->setEnabled(false);
}

void PvpLobbyWindow::onHostNewConnection()
{
    if (m_socket || !m_server) return;
    m_socket = m_server->nextPendingConnection();
    // 服务器保持监听（安全关闭由析构/closePvpConnection处理；
    // 此处立即关闭会影响已建立的socket连接）
    onSocketConnected();
}

void PvpLobbyWindow::onSocketConnected()
{
    setStatus(QString::fromUtf8("连接成功！进入对战准备阶段"), QColor(90, 230, 110));
    m_createBtn->setEnabled(false);
    m_joinBtn->setEnabled(false);
    hide();
    emit pvpConnected(m_isHost);
}

void PvpLobbyWindow::onSocketError()
{
    QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
    setStatus(QString::fromUtf8("连接失败：") + (s ? s->errorString() : QString("?")),
              QColor(255, 90, 90));
    if (s) { s->deleteLater(); if (m_socket == s) m_socket = nullptr; }
    m_joinBtn->setEnabled(true);
    m_ipEdit->setEnabled(true);
}

void PvpLobbyWindow::autoHost()
{
    onCreateRoom();
}

void PvpLobbyWindow::autoJoin(const QString& hostAddr)
{
    m_ipEdit->setText(hostAddr);
    onJoinRoom();
}

QTcpSocket* PvpLobbyWindow::takeSocket()
{
    QTcpSocket* s = m_socket;
    m_socket = nullptr;   // 所有权移交，避免双重释放
    return s;
}

void PvpLobbyWindow::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}
