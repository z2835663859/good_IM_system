#include "main_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QDateTime>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      current_user_id_(0),
      current_group_id_(0),
      is_group_chat_(false) {
    
    // Initialize models
    current_chat_model_ = std::make_unique<ChatModel>();
    group_model_ = std::make_unique<GroupModel>();
    user_model_ = std::make_unique<UserModel>();
    
    // Initialize network
    network_ = std::make_unique<IMClientNetwork>();
    
    // Setup UI
    setWindowTitle("IM Client - Modern GUI");
    setWindowIcon(QIcon());
    setGeometry(100, 100, 1200, 700);
    
    setup_ui();
    setup_styles();
    connect_signals();
    
    // Setup heartbeat timer
    heartbeat_timer_ = new QTimer(this);
    connect(heartbeat_timer_, &QTimer::timeout, this, &MainWindow::on_heartbeat_timeout);
    
    show_login_page();
}

MainWindow::~MainWindow() {
    network_->disconnect_from_server();
}

void MainWindow::setup_ui() {
    QStackedWidget* stacked = new QStackedWidget(this);
    setCentralWidget(stacked);
    
    // ============= Login Page =============
    login_widget_ = new QWidget();
    QVBoxLayout* login_layout = new QVBoxLayout(login_widget_);
    login_layout->setSpacing(20);
    login_layout->addStretch();
    
    // Title
    QLabel* title = new QLabel("IM 客户端");
    title->setStyleSheet("font-size: 32px; font-weight: bold; color: #0078d4;");
    title->setAlignment(Qt::AlignCenter);
    login_layout->addWidget(title);
    
    // Form
    QGroupBox* form_group = new QGroupBox("登录");
    QFormLayout* form_layout = new QFormLayout(form_group);
    
    server_host_input_ = new QLineEdit("127.0.0.1");
    server_port_input_ = new QSpinBox();
    server_port_input_->setMinimum(1);
    server_port_input_->setMaximum(65535);
    server_port_input_->setValue(9999);
    
    username_input_ = new QLineEdit();
    username_input_->setPlaceholderText("用户名");
    username_input_->setText("");  // 不预填，让用户手动选择
    
    password_input_ = new QLineEdit();
    password_input_->setPlaceholderText("密码");
    password_input_->setEchoMode(QLineEdit::Password);
    password_input_->setText("");  // 不预填密码
    
    form_layout->addRow("服务器地址:", server_host_input_);
    form_layout->addRow("服务器端口:", server_port_input_);
    form_layout->addRow("用户名:", username_input_);
    form_layout->addRow("密码:", password_input_);
    
    login_layout->addWidget(form_group);
    
    login_button_ = new QPushButton("登录");
    login_button_->setMinimumHeight(40);
    login_button_->setStyleSheet("background-color: #0078d4; color: white; font-weight: bold;");
    login_layout->addWidget(login_button_);
    
    login_status_label_ = new QLabel();
    login_status_label_->setAlignment(Qt::AlignCenter);
    login_status_label_->setStyleSheet("color: #d32f2f;");
    login_layout->addWidget(login_status_label_);
    
    login_layout->addStretch();
    
    // ============= Main Page =============
    main_widget_ = new QWidget();
    QVBoxLayout* main_layout = new QVBoxLayout(main_widget_);
    main_layout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar
    QHBoxLayout* toolbar = new QHBoxLayout();
    user_info_label_ = new QLabel("用户: 未登录");
    status_label_ = new QLabel("❌ 已断开连接");
    status_label_->setStyleSheet("color: #d32f2f;");
    toolbar->addWidget(user_info_label_);
    toolbar->addStretch();
    toolbar->addWidget(status_label_);
    main_layout->addLayout(toolbar);
    
    // Main content
    QHBoxLayout* content_layout = new QHBoxLayout();
    
    // Left sidebar (users & groups)
    QVBoxLayout* sidebar_layout = new QVBoxLayout();
    
    QLabel* users_label = new QLabel("📋 联系人");
    users_label->setStyleSheet("font-weight: bold; font-size: 12px; color: #999;");
    sidebar_layout->addWidget(users_label);
    
    user_list_view_ = new QListView();
    user_list_view_->setModel(user_model_.get());
    user_list_view_->setMaximumWidth(200);
    sidebar_layout->addWidget(user_list_view_);
    
    QLabel* groups_label = new QLabel("👥 群组");
    groups_label->setStyleSheet("font-weight: bold; font-size: 12px; color: #999;");
    sidebar_layout->addWidget(groups_label);
    
    group_list_view_ = new QListView();
    group_list_view_->setModel(group_model_.get());
    group_list_view_->setMaximumWidth(200);
    sidebar_layout->addWidget(group_list_view_);
    
    create_group_button_ = new QPushButton("创建群组");
    join_group_button_ = new QPushButton("加入群组");
    sidebar_layout->addWidget(create_group_button_);
    sidebar_layout->addWidget(join_group_button_);
    
    QWidget* sidebar_widget = new QWidget();
    sidebar_widget->setLayout(sidebar_layout);
    sidebar_widget->setMaximumWidth(220);
    sidebar_widget->setStyleSheet("background-color: #2d2d2d; border-right: 1px solid #444;");
    
    // Center area (chat)
    QVBoxLayout* chat_layout = new QVBoxLayout();
    
    current_chat_label_ = new QLabel("选择联系人或群组");
    current_chat_label_->setStyleSheet("font-weight: bold; font-size: 16px; padding: 10px;");
    chat_layout->addWidget(current_chat_label_);
    
    chat_history_view_ = new QListView();
    chat_history_view_->setModel(current_chat_model_.get());
    chat_history_view_->setStyleSheet(
        "QListView { background-color: #1e1e1e; border: none; }"
        "QListView::item { padding: 5px; border: none; }"
    );
    chat_layout->addWidget(chat_history_view_);
    
    // Message input area
    QHBoxLayout* input_layout = new QHBoxLayout();
    message_input_ = new QTextEdit();
    message_input_->setMaximumHeight(60);
    message_input_->setPlaceholderText("输入消息...");
    input_layout->addWidget(message_input_);
    
    send_button_ = new QPushButton("发送");
    send_button_->setMaximumWidth(80);
    input_layout->addWidget(send_button_);
    
    chat_layout->addLayout(input_layout);
    
    QWidget* chat_widget = new QWidget();
    chat_widget->setLayout(chat_layout);
    
    content_layout->addWidget(sidebar_widget);
    content_layout->addWidget(chat_widget, 1);
    
    main_layout->addLayout(content_layout, 1);
    
    // Add to stacked widget
    stacked->addWidget(login_widget_);
    stacked->addWidget(main_widget_);
}

void MainWindow::setup_styles() {
    QString dark_style = R"(
        QMainWindow {
            background-color: #1e1e1e;
            color: #ffffff;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #ffffff;
        }
        QLineEdit, QTextEdit {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 5px;
        }
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1084d8;
        }
        QPushButton:pressed {
            background-color: #006bb3;
        }
        QListView {
            background-color: #2d2d2d;
            border: 1px solid #444;
            border-radius: 4px;
        }
        QListView::item {
            padding: 8px;
            border-bottom: 1px solid #3d3d3d;
        }
        QListView::item:hover {
            background-color: #3d3d3d;
        }
        QListView::item:selected {
            background-color: #0078d4;
        }
        QLabel {
            color: #ffffff;
        }
        QGroupBox {
            color: #ffffff;
            border: 1px solid #444;
            border-radius: 4px;
            margin-top: 6px;
            padding-top: 6px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 3px 0 3px;
        }
        QSpinBox {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 5px;
        }
    )";
    
    qApp->setStyle("Fusion");
    qApp->setStyleSheet(dark_style);
}

void MainWindow::connect_signals() {
    // Network signals
    connect(network_.get(), &IMClientNetwork::connected, this, &MainWindow::on_connected);
    connect(network_.get(), &IMClientNetwork::disconnected, this, &MainWindow::on_disconnected);
    connect(network_.get(), &IMClientNetwork::connection_error, this, &MainWindow::on_connection_error);
    connect(network_.get(), &IMClientNetwork::login_success, this, &MainWindow::on_login_success);
    connect(network_.get(), &IMClientNetwork::login_failed, this, &MainWindow::on_login_failed);
    connect(network_.get(), &IMClientNetwork::message_received, this, &MainWindow::on_message_received);
    connect(network_.get(), &IMClientNetwork::group_message_received, this, &MainWindow::on_group_message_received);
    connect(network_.get(), &IMClientNetwork::group_created, this, &MainWindow::on_group_created);
    connect(network_.get(), &IMClientNetwork::group_joined, this, &MainWindow::on_group_joined);
    connect(network_.get(), &IMClientNetwork::groups_list_received, this, &MainWindow::on_groups_list_received);
    connect(network_.get(), &IMClientNetwork::notify, this, &MainWindow::on_notify);
    
    // UI signals
    connect(login_button_, &QPushButton::clicked, this, &MainWindow::on_login_clicked);
    connect(send_button_, &QPushButton::clicked, this, &MainWindow::on_send_clicked);
    connect(create_group_button_, &QPushButton::clicked, this, &MainWindow::on_create_group_clicked);
    connect(join_group_button_, &QPushButton::clicked, this, &MainWindow::on_join_group_clicked);
    connect(user_list_view_, &QListView::clicked, this, &MainWindow::on_user_selected);
    connect(group_list_view_, &QListView::clicked, this, &MainWindow::on_group_selected);
    connect(message_input_, &QTextEdit::textChanged, this, &MainWindow::on_message_text_changed);
}

void MainWindow::show_login_page() {
    ((QStackedWidget*)centralWidget())->setCurrentWidget(login_widget_);
}

void MainWindow::show_main_page() {
    ((QStackedWidget*)centralWidget())->setCurrentWidget(main_widget_);
}

void MainWindow::on_login_clicked() {
    QString host = server_host_input_->text();
    int port = server_port_input_->value();
    QString username = username_input_->text();
    QString password = password_input_->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        login_status_label_->setText("❌ Please enter username and password");
        return;
    }
    
    if (host.isEmpty()) {
        host = "localhost";
        server_host_input_->setText(host);
    }
    
    login_status_label_->setText("Connecting to " + host + ":" + QString::number(port) + "...");
    
    // Save login credentials for when connection succeeds
    pending_login_username_ = username;
    pending_login_password_ = password;
    
    network_->connect_to_server(host, port);
}

void MainWindow::on_connected() {
    login_status_label_->setText("已连接，正在登录...");
    
    // Send login immediately after connection
    if (!pending_login_username_.isEmpty()) {
        network_->send_login(pending_login_username_, pending_login_password_);
        pending_login_username_.clear();
        pending_login_password_.clear();
    }
}

void MainWindow::on_disconnected() {
    status_label_->setText("❌ 已断开连接");
    status_label_->setStyleSheet("color: #d32f2f;");
    heartbeat_timer_->stop();
    show_login_page();
}

void MainWindow::on_connection_error(const QString& error) {
    login_status_label_->setText("❌ Connection error: " + error);
}

void MainWindow::on_login_success(uint32_t user_id, const QString& username) {
    user_info_label_->setText(QString("用户: %1").arg(username));
    status_label_->setText("✅ 已连接");
    status_label_->setStyleSheet("color: #4caf50;");
    
    show_main_page();
    
    // Add default test contacts (bob and charlie)
    if (username != "bob") {
        UserInfo user;
        user.user_id = 2;
        user.username = "bob";
        user_model_->add_user(user);
    }
    if (username != "charlie") {
        UserInfo user;
        user.user_id = 3;
        user.username = "charlie";
        user_model_->add_user(user);
    }
    if (username != "alice") {
        UserInfo user;
        user.user_id = 1;
        user.username = "alice";
        user_model_->add_user(user);
    }
    
    // Start heartbeat
    heartbeat_timer_->start(30000);  // 30 seconds
    
    // Load groups
    refresh_groups();
}

void MainWindow::on_login_failed(const QString& error) {
    login_status_label_->setText("❌ 登录失败: " + error);
}

void MainWindow::on_send_clicked() {
    QString text = message_input_->toPlainText().trimmed();
    if (text.isEmpty())
        return;
    
    if (is_group_chat_ && current_group_id_ > 0) {
        network_->send_chat_group(current_group_id_, text);
    } else if (!is_group_chat_ && current_user_id_ > 0) {
        network_->send_chat_to(current_user_id_, text);
    } else {
        return;  // No valid recipient selected
    }
    
    // Add to chat history
    ChatMessage msg;
    msg.from_user_id = network_->get_user_id();
    msg.from_username = network_->get_username();
    msg.text = text;
    msg.timestamp = QDateTime::currentDateTime();
    msg.is_sender = true;
    msg.group_id = is_group_chat_ ? current_group_id_ : 0;
    
    // Add directly to the current model that is displayed
    current_chat_model_->add_message(msg);
    
    // Also save to backup for later access
    if (is_group_chat_) {
        uint32_t key = current_group_id_ + 1000000;
        if (chat_histories_.find(key) == chat_histories_.end()) {
            chat_histories_[key] = std::make_unique<ChatModel>();
        }
        if (chat_histories_[key]) {
            chat_histories_[key]->add_message(msg);
        }
    } else {
        if (chat_histories_.find(current_user_id_) == chat_histories_.end()) {
            chat_histories_[current_user_id_] = std::make_unique<ChatModel>();
        }
        if (chat_histories_[current_user_id_]) {
            chat_histories_[current_user_id_]->add_message(msg);
        }
    }
    
    message_input_->clear();
    chat_history_view_->scrollToBottom();
}

void MainWindow::on_message_received(uint32_t from_user_id, const QString& from_username, const QString& text) {
    ChatMessage msg;
    msg.from_user_id = from_user_id;
    msg.from_username = from_username;
    msg.text = text;
    msg.timestamp = QDateTime::currentDateTime();
    msg.is_sender = false;
    msg.group_id = 0;
    
    // Store in history
    if (chat_histories_.find(from_user_id) == chat_histories_.end()) {
        chat_histories_[from_user_id] = std::make_unique<ChatModel>();
    }
    chat_histories_[from_user_id]->add_message(msg);
    
    // If this is current chat, show immediately
    if (!is_group_chat_ && current_user_id_ == from_user_id) {
        current_chat_model_->add_message(msg);
        chat_history_view_->scrollToBottom();
    }
}

void MainWindow::on_group_message_received(uint32_t group_id, uint32_t from_user_id,
                                           const QString& from_username, const QString& text) {
    ChatMessage msg;
    msg.from_user_id = from_user_id;
    msg.from_username = from_username;
    msg.text = text;
    msg.timestamp = QDateTime::currentDateTime();
    msg.is_sender = false;
    msg.group_id = group_id;
    
    uint32_t key = group_id + 1000000;
    if (chat_histories_.find(key) == chat_histories_.end()) {
        chat_histories_[key] = std::make_unique<ChatModel>();
    }
    chat_histories_[key]->add_message(msg);
    
    if (is_group_chat_ && current_group_id_ == group_id) {
        current_chat_model_->add_message(msg);
        chat_history_view_->scrollToBottom();
    }
}

void MainWindow::on_group_created(uint32_t group_id, const QString& group_name) {
    GroupInfo group;
    group.group_id = group_id;
    group.group_name = group_name;
    group.member_count = 1;
    
    group_model_->add_group(group);
    chat_histories_[group_id + 1000000] = std::make_unique<ChatModel>();
    
    // 只更新创建者本人的群组列表，不广播给所有人
    // 其他用户需要主动加入群组
    
    // 显示群组ID给用户
    QMessageBox::information(this, "群组创建成功", 
        QString("群组 '%1' 创建成功！\n\n群组ID: %2\n\n请将此ID告诉其他用户来加入群组")
        .arg(group_name)
        .arg(group_id));
}

void MainWindow::on_group_joined(uint32_t group_id) {
    // 加入群组成功后，刷新群组列表
    refresh_groups();
}

void MainWindow::on_create_group_clicked() {
    
    // Create a simple custom dialog without any macOS native components
    QDialog dialog(this);
    dialog.setWindowTitle("创建群组");
    dialog.setMinimumWidth(300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // Label
    QLabel* label = new QLabel("群组名称:");
    label->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(label);
    
    // Input field
    QLineEdit* input_field = new QLineEdit();
    input_field->setStyleSheet(
        "QLineEdit {"
        "    background-color: #3d3d3d;"
        "    color: #ffffff;"
        "    border: 1px solid #555;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    font-size: 13px;"
        "}"
    );
    input_field->setFocus();
    layout->addWidget(input_field);
    
    // Buttons
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    QPushButton* create_btn = new QPushButton("创建");
    create_btn->setStyleSheet(
        "QPushButton {"
        "    background-color: #0078d4;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 20px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1084d7; }"
    );
    create_btn->setMinimumWidth(80);
    button_layout->addWidget(create_btn);
    
    QPushButton* cancel_btn = new QPushButton("取消");
    cancel_btn->setStyleSheet(
        "QPushButton {"
        "    background-color: #555555;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 20px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #666666; }"
    );
    cancel_btn->setMinimumWidth(80);
    button_layout->addWidget(cancel_btn);
    
    layout->addLayout(button_layout);
    
    // Dialog styling
    dialog.setStyleSheet(
        "QDialog {"
        "    background-color: #2d2d2d;"
        "}"
    );
    
    // Connect buttons
    connect(create_btn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancel_btn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Run dialog
    int result = dialog.exec();
    
    if (result == QDialog::Accepted) {
        QString group_name = input_field->text().trimmed();
        if (!group_name.isEmpty()) {
            network_->send_create_group(group_name, "");
        }
    }
}

void MainWindow::on_join_group_clicked() {
    // Create a simple custom dialog without any macOS native components
    QDialog dialog(this);
    dialog.setWindowTitle("加入群组");
    dialog.setMinimumWidth(300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // Label
    QLabel* label = new QLabel("群组ID:");
    label->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(label);
    
    // Input field
    QLineEdit* input_field = new QLineEdit();
    input_field->setStyleSheet(
        "QLineEdit {"
        "    background-color: #3d3d3d;"
        "    color: #ffffff;"
        "    border: 1px solid #555;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    font-size: 13px;"
        "}"
    );
    input_field->setFocus();
    layout->addWidget(input_field);
    
    // Buttons
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    QPushButton* join_btn = new QPushButton("加入");
    join_btn->setStyleSheet(
        "QPushButton {"
        "    background-color: #0078d4;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 20px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1084d7; }"
    );
    join_btn->setMinimumWidth(80);
    button_layout->addWidget(join_btn);
    
    QPushButton* cancel_btn = new QPushButton("Cancel");
    cancel_btn->setStyleSheet(
        "QPushButton {"
        "    background-color: #555555;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 20px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #666666; }"
    );
    cancel_btn->setMinimumWidth(80);
    button_layout->addWidget(cancel_btn);
    
    layout->addLayout(button_layout);
    
    // Dialog styling
    dialog.setStyleSheet(
        "QDialog {"
        "    background-color: #2d2d2d;"
        "}"
    );
    
    // Connect buttons
    connect(join_btn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancel_btn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Run dialog
    if (dialog.exec() == QDialog::Accepted) {
        QString group_id_str = input_field->text().trimmed();
        if (!group_id_str.isEmpty()) {
            uint32_t group_id = group_id_str.toUInt();
            network_->send_join_group(group_id);
        }
    }
}

void MainWindow::on_groups_list_received(const QString& json_list) {
    group_model_->clear_groups();  // 清除旧的群组列表
    
    QJsonDocument doc = QJsonDocument::fromJson(json_list.toUtf8());
    QJsonObject obj = doc.object();
    QJsonArray groups = obj["groups"].toArray();
    
    for (const auto& g : groups) {
        QJsonObject group_obj = g.toObject();
        GroupInfo group;
        group.group_id = group_obj["group_id"].toInt();
        group.group_name = group_obj["group_name"].toString();
        group.member_count = group_obj["member_count"].toInt();
        
        group_model_->add_group(group);
    }
}

void MainWindow::on_user_selected(const QModelIndex& index) {
    if (const UserInfo* user = user_model_->get_user(index.row())) {
        switch_to_user(user->user_id, user->username);
    }
}

void MainWindow::on_group_selected(const QModelIndex& index) {
    if (const GroupInfo* group = group_model_->get_group(index.row())) {
        switch_to_group(group->group_id, group->group_name);
    }
}

void MainWindow::on_message_text_changed() {
    // Auto-resize text edit
    send_button_->setEnabled(!message_input_->toPlainText().trimmed().isEmpty());
}

void MainWindow::on_notify(const QString& message) {
    QMessageBox::information(this, "通知", message);
}

void MainWindow::on_heartbeat_timeout() {
    network_->send_heartbeat();
}

void MainWindow::switch_to_user(uint32_t user_id, const QString& username) {
    is_group_chat_ = false;
    current_user_id_ = user_id;
    current_group_id_ = 0;
    current_chat_name_ = username;
    
    current_chat_label_->setText(QString("💬 与 %1 聊天").arg(username));
    
    // Load or create chat history
    if (chat_histories_.find(user_id) == chat_histories_.end()) {
        chat_histories_[user_id] = std::make_unique<ChatModel>();
    }
    
    current_chat_model_ = std::make_unique<ChatModel>();
    for (const auto& msg : chat_histories_[user_id]->messages()) {
        current_chat_model_->add_message(msg);
    }
    chat_history_view_->setModel(current_chat_model_.get());
}

void MainWindow::switch_to_group(uint32_t group_id, const QString& group_name) {
    is_group_chat_ = true;
    current_group_id_ = group_id;
    current_user_id_ = 0;
    current_chat_name_ = group_name;
    
    current_chat_label_->setText(QString("👥 群组: %1").arg(group_name));
    
    uint32_t key = group_id + 1000000;
    if (chat_histories_.find(key) == chat_histories_.end()) {
        chat_histories_[key] = std::make_unique<ChatModel>();
    }
    
    current_chat_model_ = std::make_unique<ChatModel>();
    for (const auto& msg : chat_histories_[key]->messages()) {
        current_chat_model_->add_message(msg);
    }
    chat_history_view_->setModel(current_chat_model_.get());
}

void MainWindow::refresh_groups() {
    network_->send_list_groups();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    network_->send_logout();
    QMainWindow::closeEvent(event);
}
