#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <cstdint>

// Protocol constants (same as server)
constexpr uint32_t kMagic = 0x494D3031;  // "IM01"
constexpr size_t kHeaderSize = 4 + 2 + 4;

// Message types
enum class MsgType : uint16_t {
    HEARTBEAT       = 1,
    CHAT            = 2,
    LOGIN           = 3,
    LOGOUT          = 4,
    CHAT_TO         = 5,
    NOTIFY          = 6,
    CREATE_GROUP    = 7,
    JOIN_GROUP      = 8,
    LEAVE_GROUP     = 9,
    CHAT_GROUP      = 10,
    LIST_GROUPS     = 11,
    GROUP_MEMBERS   = 12,
};

// Packet structure
struct Packet {
    uint16_t type = 0;
    QString body;
};

// Codec for encoding/decoding
class Codec {
public:
    static QByteArray encode(const Packet& p);
    static bool try_decode(QByteArray& in, Packet& out);

private:
    static uint16_t read_u16_be(const uint8_t* p);
    static uint32_t read_u32_be(const uint8_t* p);
    static void write_u16_be(QByteArray& out, uint16_t v);
    static void write_u32_be(QByteArray& out, uint32_t v);
};

// Main network client
class IMClientNetwork : public QObject {
    Q_OBJECT

public:
    explicit IMClientNetwork(QObject* parent = nullptr);
    ~IMClientNetwork();

    // Connection management
    void connect_to_server(const QString& host, int port);
    void disconnect_from_server();
    bool is_connected() const;

    // Authentication
    void send_login(const QString& username, const QString& password);
    void send_logout();

    // Single chat
    void send_chat_to(uint32_t to_user_id, const QString& text);

    // Group operations
    void send_create_group(const QString& group_name, const QString& description = "");
    void send_join_group(uint32_t group_id);
    void send_leave_group(uint32_t group_id);
    void send_list_groups();
    void send_group_members(uint32_t group_id);

    // Group chat
    void send_chat_group(uint32_t group_id, const QString& text);

    // Heartbeat
    void send_heartbeat();

    // Getters
    uint32_t get_user_id() const { return user_id_; }
    QString get_username() const { return username_; }

signals:
    // Connection signals
    void connected();
    void disconnected();
    void connection_error(const QString& error);

    // Authentication signals
    void login_success(uint32_t user_id, const QString& username);
    void login_failed(const QString& error);
    void logout_success();

    // Message signals
    void message_received(uint32_t from_user_id, const QString& from_username, const QString& text);
    void group_message_received(uint32_t group_id, uint32_t from_user_id, 
                               const QString& from_username, const QString& text);

    // Group operation signals
    void group_created(uint32_t group_id, const QString& group_name);
    void group_joined(uint32_t group_id);
    void group_left(uint32_t group_id);
    void groups_list_received(const QString& json_list);
    void group_members_received(uint32_t group_id, const QString& json_members);

    // Notification
    void notify(const QString& message);
    void heartbeat_response();

private slots:
    void on_connected();
    void on_disconnected();
    void on_error(QAbstractSocket::SocketError error);
    void on_ready_read();

private:
    void send_packet(const Packet& pkt);
    void handle_notify(const Packet& pkt);
    void handle_chat_to(const Packet& pkt);
    void handle_chat_group(const Packet& pkt);

    QTcpSocket* socket_;
    QByteArray recv_buffer_;
    uint32_t user_id_;
    QString username_;
};
