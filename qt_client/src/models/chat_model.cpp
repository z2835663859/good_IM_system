#include "chat_model.hpp"
#include <QRandomGenerator>

// ============================================
// ChatModel
// ============================================

ChatModel::ChatModel(QObject* parent)
    : QAbstractListModel(parent) {
    // Initialize avatar colors
    avatar_colors_ << QColor("#FF6B6B") << QColor("#4ECDC4") << QColor("#45B7D1")
                   << QColor("#FFA07A") << QColor("#98D8C8") << QColor("#F7DC6F");
}

int ChatModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return messages_.size();
}

QVariant ChatModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= messages_.size())
        return QVariant();

    const ChatMessage& msg = messages_[index.row()];

    switch (role) {
        case Qt::DisplayRole: {
            // Format: "[User] message"
            return QString("[%1] %2").arg(msg.from_username, msg.text);
        }
        case UsernameRole:
            return msg.from_username;
        case TextRole:
            return msg.text;
        case TimestampRole:
            return msg.timestamp.toString("hh:mm");
        case IsSenderRole:
            return msg.is_sender;
        case AvatarColorRole:
            return get_avatar_color(msg.from_user_id).name();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ChatModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[UsernameRole] = "username";
    roles[TextRole] = "text";
    roles[TimestampRole] = "timestamp";
    roles[IsSenderRole] = "isSender";
    roles[AvatarColorRole] = "avatarColor";
    return roles;
}

void ChatModel::add_message(const ChatMessage& msg) {
    beginInsertRows(QModelIndex(), messages_.size(), messages_.size());
    messages_.append(msg);
    endInsertRows();
}

void ChatModel::clear_messages() {
    beginResetModel();
    messages_.clear();
    endResetModel();
}

const ChatMessage* ChatModel::get_message(int index) const {
    if (index < 0 || index >= messages_.size())
        return nullptr;
    return &messages_[index];
}

QColor ChatModel::get_avatar_color(uint32_t user_id) const {
    return avatar_colors_[user_id % avatar_colors_.size()];
}

// ============================================
// GroupModel
// ============================================

GroupModel::GroupModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int GroupModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return groups_.size();
}

QVariant GroupModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= groups_.size())
        return QVariant();

    const GroupInfo& group = groups_[index.row()];

    switch (role) {
        case Qt::DisplayRole:
            return group.group_name;
        case GroupIdRole:
            return (int)group.group_id;
        case GroupNameRole:
            return group.group_name;
        case MemberCountRole:
            return group.member_count;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> GroupModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[GroupIdRole] = "groupId";
    roles[GroupNameRole] = "groupName";
    roles[MemberCountRole] = "memberCount";
    return roles;
}

void GroupModel::add_group(const GroupInfo& group) {
    beginInsertRows(QModelIndex(), groups_.size(), groups_.size());
    groups_.append(group);
    endInsertRows();
}

void GroupModel::clear_groups() {
    beginResetModel();
    groups_.clear();
    endResetModel();
}

void GroupModel::remove_group(uint32_t group_id) {
    for (int i = 0; i < groups_.size(); ++i) {
        if (groups_[i].group_id == group_id) {
            beginRemoveRows(QModelIndex(), i, i);
            groups_.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}

const GroupInfo* GroupModel::get_group(int index) const {
    if (index < 0 || index >= groups_.size())
        return nullptr;
    return &groups_[index];
}

const GroupInfo* GroupModel::find_group(uint32_t group_id) const {
    for (const auto& g : groups_) {
        if (g.group_id == group_id)
            return &g;
    }
    return nullptr;
}

// ============================================
// UserModel
// ============================================

UserModel::UserModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int UserModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return users_.size();
}

QVariant UserModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= users_.size())
        return QVariant();

    const UserInfo& user = users_[index.row()];

    switch (role) {
        case Qt::DisplayRole:
            return user.username;
        case UserIdRole:
            return (int)user.user_id;
        case UsernameRole:
            return user.username;
        case IsOnlineRole:
            return user.is_online;
        case AvatarRole:
            return user.username.at(0).toLower();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> UserModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[UserIdRole] = "userId";
    roles[UsernameRole] = "username";
    roles[IsOnlineRole] = "isOnline";
    roles[AvatarRole] = "avatar";
    return roles;
}

void UserModel::add_user(const UserInfo& user) {
    beginInsertRows(QModelIndex(), users_.size(), users_.size());
    users_.append(user);
    endInsertRows();
}

void UserModel::clear_users() {
    beginResetModel();
    users_.clear();
    endResetModel();
}

void UserModel::update_user_status(uint32_t user_id, bool is_online) {
    for (int i = 0; i < users_.size(); ++i) {
        if (users_[i].user_id == user_id) {
            users_[i].is_online = is_online;
            emit dataChanged(index(i), index(i), {IsOnlineRole});
            return;
        }
    }
}

const UserInfo* UserModel::get_user(int index) const {
    if (index < 0 || index >= users_.size())
        return nullptr;
    return &users_[index];
}

const UserInfo* UserModel::find_user(uint32_t user_id) const {
    for (const auto& u : users_) {
        if (u.user_id == user_id)
            return &u;
    }
    return nullptr;
}
