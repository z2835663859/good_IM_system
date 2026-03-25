#include "im_client_network.hpp"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>

// Codec implementation
QByteArray Codec::encode(const Packet& p) {
    QByteArray result;
    
    // Body
    QByteArray body = p.body.toUtf8();
    uint32_t body_len = body.size();
    
    // Header: magic(4) + type(2) + length(4)
    result.resize(kHeaderSize);
    
    // Write magic at [0-3]
    uint8_t* data = (uint8_t*)result.data();
    data[0] = (kMagic >> 24) & 0xFF;
    data[1] = (kMagic >> 16) & 0xFF;
    data[2] = (kMagic >> 8) & 0xFF;
    data[3] = kMagic & 0xFF;
    
    // Write type at [4-5]
    data[4] = (p.type >> 8) & 0xFF;
    data[5] = p.type & 0xFF;
    
    // Write length at [6-9]
    data[6] = (body_len >> 24) & 0xFF;
    data[7] = (body_len >> 16) & 0xFF;
    data[8] = (body_len >> 8) & 0xFF;
    data[9] = body_len & 0xFF;
    
    // Append body
    result.append(body);
    return result;
}

bool Codec::try_decode(QByteArray& in, Packet& out) {
    if (in.size() < kHeaderSize) {
        return false;
    }
    
    uint32_t magic = read_u32_be((uint8_t*)in.data());
    if (magic != kMagic) {
        in.clear();
        return false;
    }
    
    uint16_t type = read_u16_be((uint8_t*)in.data() + 4);
    uint32_t body_len = read_u32_be((uint8_t*)in.data() + 6);
    
    if (in.size() < kHeaderSize + body_len) {
        return false;
    }
    
    out.type = type;
    out.body = QString::fromUtf8(in.data() + kHeaderSize, body_len);
    
    in.remove(0, kHeaderSize + body_len);
    return true;
}

uint16_t Codec::read_u16_be(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

uint32_t Codec::read_u32_be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

void Codec::write_u16_be(QByteArray& out, uint16_t v) {
    ((uint8_t*)out.data())[out.size() - 2] = (v >> 8);
    ((uint8_t*)out.data())[out.size() - 1] = v & 0xFF;
}

void Codec::write_u32_be(QByteArray& out, uint32_t v) {
    int offset = out.size() - 4;
    ((uint8_t*)out.data())[offset] = (v >> 24);
    ((uint8_t*)out.data())[offset + 1] = (v >> 16) & 0xFF;
    ((uint8_t*)out.data())[offset + 2] = (v >> 8) & 0xFF;
    ((uint8_t*)out.data())[offset + 3] = v & 0xFF;
}

// ============================================
// IMClientNetwork implementation
// ============================================

IMClientNetwork::IMClientNetwork(QObject* parent)
    : QObject(parent), socket_(nullptr), user_id_(0) {
    socket_ = new QTcpSocket(this);
    
    connect(socket_, &QTcpSocket::connected, this, &IMClientNetwork::on_connected);
    connect(socket_, &QTcpSocket::disconnected, this, &IMClientNetwork::on_disconnected);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &IMClientNetwork::on_error);
    connect(socket_, &QTcpSocket::readyRead, this, &IMClientNetwork::on_ready_read);
}

IMClientNetwork::~IMClientNetwork() {
    disconnect_from_server();
}

void IMClientNetwork::connect_to_server(const QString& host, int port) {
    if (socket_->state() != QTcpSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
    socket_->connectToHost(host, port);
}

void IMClientNetwork::disconnect_from_server() {
    if (socket_->isOpen()) {
        socket_->disconnectFromHost();
        if (!socket_->waitForDisconnected(1000)) {
            socket_->abort();
        }
    }
}

bool IMClientNetwork::is_connected() const {
    return socket_ && socket_->state() == QTcpSocket::ConnectedState;
}

void IMClientNetwork::send_login(const QString& username, const QString& password) {
    username_ = username;
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::LOGIN;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_logout() {
    Packet pkt;
    pkt.type = (uint16_t)MsgType::LOGOUT;
    pkt.body = "{}";
    send_packet(pkt);
}

void IMClientNetwork::send_chat_to(uint32_t to_user_id, const QString& text) {
    QJsonObject json;
    json["to_user_id"] = (int)to_user_id;
    json["text"] = text;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::CHAT_TO;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_create_group(const QString& group_name, const QString& description) {
    QJsonObject json;
    json["group_name"] = group_name;
    json["description"] = description;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::CREATE_GROUP;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_join_group(uint32_t group_id) {
    QJsonObject json;
    json["group_id"] = (int)group_id;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::JOIN_GROUP;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_leave_group(uint32_t group_id) {
    QJsonObject json;
    json["group_id"] = (int)group_id;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::LEAVE_GROUP;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_list_groups() {
    Packet pkt;
    pkt.type = (uint16_t)MsgType::LIST_GROUPS;
    pkt.body = "{}";
    
    send_packet(pkt);
}

void IMClientNetwork::send_group_members(uint32_t group_id) {
    QJsonObject json;
    json["group_id"] = (int)group_id;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::GROUP_MEMBERS;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_chat_group(uint32_t group_id, const QString& text) {
    QJsonObject json;
    json["group_id"] = (int)group_id;
    json["text"] = text;
    
    Packet pkt;
    pkt.type = (uint16_t)MsgType::CHAT_GROUP;
    pkt.body = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    
    send_packet(pkt);
}

void IMClientNetwork::send_heartbeat() {
    Packet pkt;
    pkt.type = (uint16_t)MsgType::HEARTBEAT;
    pkt.body = "ping";
    
    send_packet(pkt);
}

void IMClientNetwork::send_packet(const Packet& pkt) {
    if (!is_connected()) {
        qWarning() << "Not connected to server";
        return;
    }
    
    QByteArray data = Codec::encode(pkt);
    socket_->write(data);
    socket_->flush();
}

void IMClientNetwork::on_connected() {
    qInfo() << "Connected to server";
    emit connected();
}

void IMClientNetwork::on_disconnected() {
    qInfo() << "Disconnected from server";
    user_id_ = 0;
    username_.clear();
    emit disconnected();
}

void IMClientNetwork::on_error(QAbstractSocket::SocketError error) {
    QString error_msg = socket_->errorString();
    qWarning() << "Network error:" << error_msg;
    emit connection_error(error_msg);
}

void IMClientNetwork::on_ready_read() {
    QByteArray data = socket_->readAll();
    recv_buffer_.append(data);
    
    Packet pkt;
    while (Codec::try_decode(recv_buffer_, pkt)) {
        MsgType type = (MsgType)pkt.type;
        
        switch (type) {
            case MsgType::LOGIN: {
                // Response from server
                QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
                QJsonObject obj = doc.object();
                
                if (obj["status"].toString() == "ok") {
                    user_id_ = obj["data"].toString().toUInt();
                    emit login_success(user_id_, username_);
                } else {
                    emit login_failed(obj["message"].toString());
                }
                break;
            }
            
            case MsgType::LOGOUT: {
                user_id_ = 0;
                username_.clear();
                emit logout_success();
                break;
            }
            
            case MsgType::CHAT_TO:
                handle_chat_to(pkt);
                break;
                
            case MsgType::CHAT_GROUP:
                handle_chat_group(pkt);
                break;
                
            case MsgType::CREATE_GROUP: {
                QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
                QJsonObject obj = doc.object();
                if (obj["status"].toString() == "ok") {
                    uint32_t group_id = obj["group_id"].toInt();
                    QString group_name = obj["group_name"].toString();
                    emit group_created(group_id, group_name);
                }
                break;
            }
            
            case MsgType::JOIN_GROUP: {
                // Parse response: {"status":"ok","group_id":123,"group_name":"GroupName"}
                QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
                QJsonObject obj = doc.object();
                if (obj["status"].toString() == "ok") {
                    uint32_t group_id = obj["group_id"].toInt();
                    emit group_joined(group_id);
                }
                break;
            }
            
            case MsgType::LEAVE_GROUP: {
                emit group_left(0);
                break;
            }
            
            case MsgType::LIST_GROUPS: {
                QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
                emit groups_list_received(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
                break;
            }
            
            case MsgType::GROUP_MEMBERS: {
                QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
                emit group_members_received(0, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
                break;
            }
            
            case MsgType::NOTIFY:
                handle_notify(pkt);
                break;
                
            case MsgType::HEARTBEAT:
                emit heartbeat_response();
                break;
                
            default:
                qWarning() << "Unknown message type:" << pkt.type;
                break;
        }
    }
}

void IMClientNetwork::handle_notify(const Packet& pkt) {
    QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
    QJsonObject obj = doc.object();
    
    if (obj["status"].toString() == "error") {
        emit notify(obj["message"].toString());
    } else {
        emit notify(obj["data"].toString());
    }
}

void IMClientNetwork::handle_chat_to(const Packet& pkt) {
    QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
    QJsonObject obj = doc.object();
    
    uint32_t from_user_id = obj["from_user_id"].toInt();
    QString from_username = obj["from_username"].toString();
    QString text = obj["text"].toString();
    
    emit message_received(from_user_id, from_username, text);
}

void IMClientNetwork::handle_chat_group(const Packet& pkt) {
    QJsonDocument doc = QJsonDocument::fromJson(pkt.body.toUtf8());
    QJsonObject obj = doc.object();
    
    uint32_t group_id = obj["group_id"].toInt();
    uint32_t from_user_id = obj["from_user_id"].toInt();
    QString from_username = obj["from_username"].toString();
    QString text = obj["text"].toString();
    
    emit group_message_received(group_id, from_user_id, from_username, text);
}
