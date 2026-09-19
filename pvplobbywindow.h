#ifndef PVPLOBBYWINDOW_H
#define PVPLOBBYWINDOW_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

// 联机对战大厅 —— 局域网双人对战的房间窗口。
// 主机侧：QTcpServer 监听并显示本机 IP；客户端侧：输入主机 IP 连接。
// 连接成功后发出 pvpConnected(isHost)，自身的 TCP 连接由 Synera 接管。
class PvpLobbyWindow : public QWidget {
    Q_OBJECT

public:
    static constexpr quint16 PVP_PORT = 45454;

    explicit PvpLobbyWindow(QWidget* parent = nullptr);
    ~PvpLobbyWindow() override;

    // 自动化验证钩子：直接以指定角色建立/等待连接
    void autoHost();
    void autoJoin(const QString& hostAddr);

    bool isHost() const { return m_isHost; }
    QTcpSocket* takeSocket();   // 移交连接所有权（连接成功后调用）

signals:
    void pvpConnected(bool isHost);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCreateRoom();
    void onJoinRoom();
    void onHostNewConnection();
    void onSocketConnected();
    void onSocketError();

private:
    void buildUi();
    void setStatus(const QString& text, const QColor& color);

    QTcpServer* m_server = nullptr;
    QTcpSocket* m_socket = nullptr;   // 已建立的连接（host: 来自 nextPendingConnection）
    bool m_isHost = false;

    QLineEdit* m_ipEdit = nullptr;
    QPushButton* m_createBtn = nullptr;
    QPushButton* m_joinBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_addrLabel = nullptr;
};
#endif // PVPLOBBYWINDOW_H
