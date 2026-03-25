#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QColor>
#include <cstdint>

struct ChatMessage {
    uint32_t from_user_id;
    QString from_username;
    QString text;
    QDateTime timestamp;
    bool is_sender;  // true if this user sent the message
    uint32_t group_id;  // 0 for single chat, group_id for group chat
};

class ChatModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum MessageRole {
        UsernameRole = Qt::UserRole + 1,
        TextRole,
        TimestampRole,
        IsSenderRole,
        AvatarColorRole
    };

    explicit ChatModel(QObject* parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Message management
    void add_message(const ChatMessage& msg);
    void clear_messages();
    const ChatMessage* get_message(int index) const;

    // Getters
    int count() const { return messages_.size(); }
    const QList<ChatMessage>& messages() const { return messages_; }

private:
    QList<ChatMessage> messages_;
    QList<QColor> avatar_colors_;
    
    QColor get_avatar_color(uint32_t user_id) const;
};

// Group model
struct GroupInfo {
    uint32_t group_id;
    QString group_name;
    int member_count;
};

class GroupModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum GroupRole {
        GroupIdRole = Qt::UserRole + 1,
        GroupNameRole,
        MemberCountRole
    };

    explicit GroupModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void add_group(const GroupInfo& group);
    void clear_groups();
    void remove_group(uint32_t group_id);

    int count() const { return groups_.size(); }
    const GroupInfo* get_group(int index) const;
    const GroupInfo* find_group(uint32_t group_id) const;

private:
    QList<GroupInfo> groups_;
};

// User model for contacts
struct UserInfo {
    uint32_t user_id;
    QString username;
    bool is_online;
};

class UserModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum UserRole {
        UserIdRole = Qt::UserRole + 1,
        UsernameRole,
        IsOnlineRole,
        AvatarRole
    };

    explicit UserModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void add_user(const UserInfo& user);
    void clear_users();
    void update_user_status(uint32_t user_id, bool is_online);

    int count() const { return users_.size(); }
    const UserInfo* get_user(int index) const;
    const UserInfo* find_user(uint32_t user_id) const;

private:
    QList<UserInfo> users_;
};
