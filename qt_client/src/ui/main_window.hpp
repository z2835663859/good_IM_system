#pragma once

#include <QMainWindow>
#include <QListView>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QTimer>
#include <QFormLayout>
#include <QGroupBox>
#include <memory>
#include <map>

#include "network/im_client_network.hpp"
#include "models/chat_model.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Network slots
    void on_connected();
    void on_disconnected();
    void on_connection_error(const QString& error);
    void on_login_success(uint32_t user_id, const QString& username);
    void on_login_failed(const QString& error);
    void on_message_received(uint32_t from_user_id, const QString& from_username, const QString& text);
    void on_group_message_received(uint32_t group_id, uint32_t from_user_id,
                                   const QString& from_username, const QString& text);
    void on_group_created(uint32_t group_id, const QString& group_name);
    void on_group_joined(uint32_t group_id);
    void on_groups_list_received(const QString& json_list);
    void on_notify(const QString& message);

    // UI slots
    void on_login_clicked();
    void on_send_clicked();
    void on_create_group_clicked();
    void on_join_group_clicked();
    void on_user_selected(const QModelIndex& index);
    void on_group_selected(const QModelIndex& index);
    void on_message_text_changed();

    // Timer
    void on_heartbeat_timeout();

private:
    void setup_ui();
    void setup_styles();
    void connect_signals();

    void show_login_page();
    void show_main_page();
    void switch_to_user(uint32_t user_id, const QString& username);
    void switch_to_group(uint32_t group_id, const QString& group_name);
    void refresh_groups();

    // UI components - Login page
    QWidget* login_widget_;
    QLineEdit* username_input_;
    QLineEdit* password_input_;
    QLineEdit* server_host_input_;
    QSpinBox* server_port_input_;
    QPushButton* login_button_;
    QLabel* login_status_label_;

    // UI components - Main page
    QWidget* main_widget_;
    QListView* chat_history_view_;
    QTextEdit* message_input_;
    QPushButton* send_button_;
    QPushButton* create_group_button_;
    QPushButton* join_group_button_;
    QLabel* current_chat_label_;
    QLabel* status_label_;
    QLabel* user_info_label_;

    // Sidebar
    QListView* user_list_view_;
    QListView* group_list_view_;

    // Models
    std::unique_ptr<ChatModel> current_chat_model_;
    std::unique_ptr<GroupModel> group_model_;
    std::unique_ptr<UserModel> user_model_;
    std::map<uint32_t, std::unique_ptr<ChatModel>> chat_histories_;

    // Network
    std::unique_ptr<IMClientNetwork> network_;

    // State
    uint32_t current_user_id_;
    uint32_t current_group_id_;
    QString current_chat_name_;
    bool is_group_chat_;
    QTimer* heartbeat_timer_;
    QString pending_login_username_;
    QString pending_login_password_;
};
