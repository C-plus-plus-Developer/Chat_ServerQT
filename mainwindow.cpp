#include "mainwindow.h"
#include "ServerController.h"
#include <QDateTime>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , serverRunning(false)
    , adminCount(0)
    , userCount(0)
    , serverController(nullptr)
{
    setupUI();
    loadTestData();

    statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(updateStatistics()));
    statsTimer->start(2000);

    // подключения для списка админов
    connect(adminsList, SIGNAL(itemSelectionChanged()), this, SLOT(onAdminSelectionChanged()));
    connect(adminsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(changeAdminPassword()));
}

MainWindow::~MainWindow()
{
    if (serverController) {
        serverController->stopServer();
        serverController->wait(3000);
        serverController->deleteLater();
    }
}

void MainWindow::onAdminSelectionChanged() {
    qDebug() << "Admin selection changed";

    QList<QListWidgetItem*> selected = adminsList->selectedItems();
    qDebug() << "Selected items count:" << selected.count();

    if (!selected.isEmpty()) {
        QString selectedText = selected.first()->text();
        qDebug() << "Selected admin:" << selectedText;
    }

    // Обновляем кнопки
    refreshAdminList();
}

void MainWindow::setupUI()
{
    setWindowTitle("Chat Server Control Panel");
    setMinimumSize(1200, 800);

    setStyleSheet(
        "QMainWindow { background-color: #2b2b2b; color: #ffffff; }"
        "QGroupBox { "
        "    color: #ffffff; "
        "    font-weight: bold; "
        "    border: 2px solid #555; "
        "    border-radius: 8px; "
        "    margin-top: 1ex; "
        "    padding-top: 10px; "
        "    background-color: #3c3c3c;"
        "}"
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    left: 10px; "
        "    padding: 0 5px 0 5px; "
        "    color: #ffffff; "
        "    background-color: #2b2b2b;"
        "}"
        "QPushButton { "
        "    background-color: #555; "
        "    color: white; "
        "    border: 1px solid #777; "
        "    padding: 8px; "
        "    border-radius: 4px; "
        "    font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #666; }"
        "QPushButton:pressed { background-color: #444; }"
        "QPushButton:disabled { background-color: #333; color: #888; }"
        "QLabel { color: #ffffff; }"
        "QListWidget { "
        "    background-color: #2b2b2b; "
        "    color: #ffffff; "
        "    border: 2px solid #555; "
        "    border-radius: 4px; "
        "    outline: none;"
        "}"
        "QListWidget::item { "
        "    background-color: #3c3c3c; "
        "    color: #ffffff; "
        "    border: 1px solid #555; "
        "    border-radius: 3px; "
        "    padding: 8px; "
        "    margin: 2px; "
        "}"
        "QListWidget::item:hover { "
        "    background-color: #4a4a4a; "
        "    border: 1px solid #666; "
        "}"
        "QListWidget::item:selected { "
        "    background-color: #0078d7; "
        "    color: white; "
        "    border: 1px solid #1e90ff; "
        "    font-weight: bold; "
        "}"
        "QListWidget::item:selected:active { "
        "    background-color: #106ebe; "
        "}"
        "QTextEdit { "
        "    background-color: #1e1e1e; "
        "    color: #00ff00; "
        "    border: 1px solid #555; "
        "    border-radius: 4px; "
        "    padding: 8px; "
        "    font-family: 'Courier New', monospace; "
        "    font-size: 12px; "
        "}"
        );

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #2b2b2b;");
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    //Управление сервером
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(5, 5, 5, 5);

    QGroupBox *serverGroup = new QGroupBox("Управление сервером");
    QVBoxLayout *serverLayout = new QVBoxLayout(serverGroup);
    serverLayout->setSpacing(10);

    serverButton = new QPushButton("Запустить сервер");
    serverButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #ff4444;"
        "    color: white;"
        "    border: none;"
        "    padding: 15px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ff6666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #cc0000;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #666;"
        "    color: #888;"
        "}"
        );
    serverButton->setMinimumHeight(60);
    connect(serverButton, SIGNAL(clicked()), this, SLOT(toggleServer()));

    statusLabel = new QLabel("Статус: Сервер остановлен");
    statusLabel->setStyleSheet("font-size: 14px; color: #ff6b6b; padding: 10px; font-weight: bold;");
    statusLabel->setAlignment(Qt::AlignCenter);

    serverLayout->addWidget(serverButton);
    serverLayout->addWidget(statusLabel);

    // Группа логов
    QGroupBox *logGroup = new QGroupBox("Лог сервера");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    logTextEdit = new QTextEdit;
    logTextEdit->setReadOnly(true);
    logTextEdit->setMaximumHeight(300);
    logTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #00ff00;"
        "    border: 1px solid #555;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-family: 'Courier New', monospace;"
        "    font-size: 12px;"
        "    selection-background-color: #0078d7;"
        "}"
        );

    logLayout->addWidget(logTextEdit);

    leftLayout->addWidget(serverGroup);
    leftLayout->addWidget(logGroup);

    //Статистика
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(5, 5, 5, 5);

    // Группа админов
    QGroupBox *adminsGroup = new QGroupBox("Управление администраторами");
    QVBoxLayout *adminsLayout = new QVBoxLayout(adminsGroup);
    adminsLayout->setSpacing(8);

    adminsCountLabel = new QLabel("Всего: 0");
    adminsCountLabel->setStyleSheet("font-weight: bold; color: #ff6b6b; font-size: 14px; padding: 5px;");
    adminsCountLabel->setAlignment(Qt::AlignCenter);

    // Кнопки управления админами
    QHBoxLayout *adminButtonsLayout = new QHBoxLayout();
    adminButtonsLayout->setSpacing(5);

    addAdminButton = new QPushButton("Добавить");
    addAdminButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 12px;"
        "    font-size: 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #66BB6A;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #388E3C;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #666;"
        "    color: #888;"
        "}"
        );
    addAdminButton->setMinimumHeight(30);
    connect(addAdminButton, SIGNAL(clicked()), this, SLOT(addAdmin()));

    removeAdminButton = new QPushButton("Удалить");
    removeAdminButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 12px;"
        "    font-size: 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ef5350;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c62828;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #666;"
        "    color: #888;"
        "}"
        );
    removeAdminButton->setMinimumHeight(30);
    removeAdminButton->setEnabled(false);
    connect(removeAdminButton, SIGNAL(clicked()), this, SLOT(removeAdmin()));

    changePasswordButton = new QPushButton("Сменить пароль");
    changePasswordButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #FF9800;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 12px;"
        "    font-size: 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #FFB74D;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #EF6C00;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #666;"
        "    color: #888;"
        "}"
        );
    changePasswordButton->setMinimumHeight(30);
    changePasswordButton->setEnabled(false);
    connect(changePasswordButton, SIGNAL(clicked()), this, SLOT(changeAdminPassword()));

    refreshAdminButton = new QPushButton("Обновить");
    refreshAdminButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 12px;"
        "    font-size: 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #42A5F5;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
        );
    refreshAdminButton->setMinimumHeight(30);
    connect(refreshAdminButton, SIGNAL(clicked()), this, SLOT(refreshAdminList()));

    adminButtonsLayout->addWidget(addAdminButton);
    adminButtonsLayout->addWidget(removeAdminButton);
    adminButtonsLayout->addWidget(changePasswordButton);
    adminButtonsLayout->addWidget(refreshAdminButton);

    // Список админов
    adminsList = new QListWidget;
    adminsList->setSelectionMode(QAbstractItemView::SingleSelection);
    adminsList->setSelectionBehavior(QAbstractItemView::SelectRows);
    adminsList->setFocusPolicy(Qt::StrongFocus);
    adminsList->setStyleSheet(
        "QListWidget {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 2px solid #555;"
        "    border-radius: 6px;"
        "    padding: 2px;"
        "    font-size: 12px;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    background-color: #3c3c3c;"
        "    color: #ffffff;"
        "    border: 1px solid #555;"
        "    border-radius: 3px;"
        "    padding: 8px;"
        "    margin: 2px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #4a4a4a;"
        "    border: 1px solid #666;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #0078d7;"
        "    color: white;"
        "    border: 1px solid #1e90ff;"
        "    font-weight: bold;"
        "}"
        "QListWidget::item:selected:active {"
        "    background-color: #106ebe;"
        "}"
        "QListWidget::item:focus {"
        "    outline: none;"
        "}"
        );

    adminsLayout->addWidget(adminsCountLabel);
    adminsLayout->addLayout(adminButtonsLayout);
    adminsLayout->addWidget(adminsList);

    // Группа пользователей
    QGroupBox *usersGroup = new QGroupBox("Пользователи чата");
    QVBoxLayout *usersLayout = new QVBoxLayout(usersGroup);

    usersCountLabel = new QLabel("Онлайн: 0 / Всего: 0");
    usersCountLabel->setStyleSheet("font-weight: bold; color: #4fc3f7; font-size: 14px; padding: 5px;");
    usersCountLabel->setAlignment(Qt::AlignCenter);

    usersList = new QListWidget;
    usersList->setSelectionMode(QAbstractItemView::NoSelection);
    usersList->setStyleSheet(
        "QListWidget {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 2px solid #555;"
        "    border-radius: 6px;"
        "    padding: 2px;"
        "    font-size: 12px;"
        "}"
        "QListWidget::item {"
        "    background-color: #3c3c3c;"
        "    color: #ffffff;"
        "    border: 1px solid #555;"
        "    border-radius: 3px;"
        "    padding: 8px;"
        "    margin: 2px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #4a4a4a;"
        "}"
        );

    usersLayout->addWidget(usersCountLabel);
    usersLayout->addWidget(usersList);

    rightLayout->addWidget(adminsGroup);
    rightLayout->addWidget(usersGroup);

    //Размещение панелей
    mainLayout->addWidget(leftPanel, 2);
    mainLayout->addWidget(rightPanel, 1);

    // Начальное сообщение
    logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") + " Готов к запуску...");
}


void MainWindow::loadTestData()
{
    adminsList->clear();
    usersList->clear();

    adminsCountLabel->setText("Всего: 0");
    usersCountLabel->setText("Онлайн: 0 / Всего: 0");
}

void MainWindow::toggleServer()
{
    if (!serverRunning) {
        // Запуск сервера
        serverController = new ServerController(this);
        connect(serverController, SIGNAL(serverStarted()), this, SLOT(onServerStarted()));
        connect(serverController, SIGNAL(serverStopped()), this, SLOT(onServerStopped()));
        connect(serverController, SIGNAL(serverError(QString)), this, SLOT(onServerError(QString)));
        connect(serverController, SIGNAL(logMessage(QString)), this, SLOT(onLogMessage(QString)));

        serverController->start();
    } else {
        if (serverController) {
            serverController->stopServer();
        }
    }
}


void MainWindow::onServerStarted()
{
    serverRunning = true;

    serverButton->setText("Остановить сервер");
    serverButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 15px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #66BB6A;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #388E3C;"
        "}"
        );

    statusLabel->setText("Статус: Сервер запущен на порту 7777");
    statusLabel->setStyleSheet("font-size: 14px; color: #4CAF50; font-weight: bold; padding: 10px;");

    // Обновление списка админов при запуске сервера
    refreshAdminList();
}

void MainWindow::onServerStopped()
{
    serverRunning = false;

    serverButton->setText("Запустить сервер");
    serverButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #ff4444;"
        "    color: white;"
        "    border: none;"
        "    padding: 15px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ff6666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #cc0000;"
        "}"
        );

    statusLabel->setText("Статус: Сервер остановлен");
    statusLabel->setStyleSheet("font-size: 14px; color: #ff6b6b; padding: 10px;");

    if (serverController) {
        serverController->wait();
        serverController->deleteLater();
        serverController = nullptr;
    }

    usersList->clear();
    usersCountLabel->setText("Онлайн: 0 / Всего: 0");

    // Обновление списка админов при остановке
    refreshAdminList();
}

void MainWindow::onServerError(const QString& error)
{
    logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") + " ❌ Ошибка: " + error);
    onServerStopped();
}

void MainWindow::onLogMessage(const QString& message)
{
    logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") + " " + message);
}


void MainWindow::refreshAdminList() {
    if (!serverController || !serverRunning) {
        adminsList->clear();
        adminsCountLabel->setText("Всего: 0");

        // Отключаем кнопки управления
        addAdminButton->setEnabled(false);
        removeAdminButton->setEnabled(false);
        changePasswordButton->setEnabled(false);
        refreshAdminButton->setEnabled(false);
        return;
    }

    QStringList admins = serverController->getAdminList();

    // Сохраняем текущее выделение
    QList<QListWidgetItem*> currentSelection = adminsList->selectedItems();
    QString selectedLogin;
    if (!currentSelection.isEmpty()) {
        selectedLogin = currentSelection.first()->text();
    }

    // Обновляем список
    adminsList->clear();

    for (const QString& login : admins) {
        if (!login.isEmpty()) {
            adminsList->addItem(login);
        }
    }

    adminsCountLabel->setText(QString("Всего: %1").arg(adminsList->count()));

    // Восстанавливаем выделение
    if (!selectedLogin.isEmpty()) {
        for (int i = 0; i < adminsList->count(); ++i) {
            QListWidgetItem* item = adminsList->item(i);
            if (item->text() == selectedLogin) {
                item->setSelected(true);
                adminsList->setCurrentItem(item);
                break;
            }
        }
    }

    // Обновляем состояние кнопок
    bool hasSelection = !adminsList->selectedItems().isEmpty();
    bool canRemove = adminsList->count() > 1 && hasSelection;

    removeAdminButton->setEnabled(canRemove);
    changePasswordButton->setEnabled(hasSelection);
    addAdminButton->setEnabled(true);
    refreshAdminButton->setEnabled(true);

    // Обновляем подсказки
    if (adminsList->count() <= 1) {
        removeAdminButton->setToolTip("Нельзя удалить последнего администратора");
        removeAdminButton->setEnabled(false);
    } else if (!hasSelection) {
        removeAdminButton->setToolTip("Выберите администратора для удаления");
        removeAdminButton->setEnabled(false);
    } else {
        removeAdminButton->setToolTip("Удалить выбранного администратора");
        removeAdminButton->setEnabled(true);
    }
}

void MainWindow::updateStatistics()
{
    if (serverRunning && serverController) {
        updateRealStatistics();
    }
}

void MainWindow::updateRealStatistics()
{
    if (serverController && serverController->isServerRunning()) {
        // Обновляем статистику из сервера
        int onlineCount = serverController->getOnlineUsersCount();
        int totalCount = serverController->getTotalUsersCount();
        QStringList onlineUsers = serverController->getOnlineUsersList();

        // Обновляем пользователей
        usersCountLabel->setText(QString("Онлайн: %1 / Всего: %2").arg(onlineCount).arg(totalCount));

        // Обновляем список
        usersList->clear();
        usersList->addItems(onlineUsers);

        // каждые две секунды админы обновляются(возможно зря)
        static int refreshCounter = 0;
        refreshCounter++;
        if (refreshCounter >= 1) {
            refreshAdminList();
            refreshCounter = 0;
        }

        // Добавление лога при подключении новых пользователей
        static int lastOnlineCount = 0;
        if (onlineCount > lastOnlineCount) {
            logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                                QString("Новый пользователь онлайн. Всего онлайн: %1").arg(onlineCount));
        } else if (onlineCount < lastOnlineCount) {
            logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                                QString("Пользователь отключился. Онлайн: %1").arg(onlineCount));
        }
        lastOnlineCount = onlineCount;

        if (logTextEdit->document()->lineCount() > 50) {
            QTextCursor cursor(logTextEdit->document());
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 1);
            cursor.removeSelectedText();
        }
    }
}

void MainWindow::addAdmin() {
    if (!serverController || !serverRunning) {
        QMessageBox::warning(this, "Ошибка", "Сервер не запущен!");
        return;
    }

    bool ok;
    QString login = QInputDialog::getText(this, "Добавить администратора",
                                          "Логин:", QLineEdit::Normal, "", &ok);
    if (!ok || login.isEmpty()) return;

    QString password = QInputDialog::getText(this, "Добавить администратора",
                                             "Пароль:", QLineEdit::Normal, "", &ok);
    if (!ok || password.isEmpty()) return;

    if (serverController->addAdmin(login, password)) {
        logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                            "Добавлен новый администратор: " + login);
        refreshAdminList();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить администратора!");
    }
}

void MainWindow::removeAdmin() {
    if (!serverController || !serverRunning) {
        QMessageBox::warning(this, "Ошибка", "Сервер не запущен!");
        return;
    }

    QList<QListWidgetItem*> selected = adminsList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Информация", "Выберите администратора для удаления!");
        return;
    }

    QString login = selected.first()->text().trimmed(); // Теперь это просто логин

    if (login.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось извлечь логин администратора!");
        return;
    }

    // Проверяем существование админа
    QStringList currentAdmins = serverController->getAdminList();
    bool adminExists = currentAdmins.contains(login);

    if (!adminExists) {
        QMessageBox::warning(this, "Ошибка",
                             QString("Администратор '%1' не найден!\n\n"
                                     "Доступные администраторы:\n%2")
                                 .arg(login)
                                 .arg(currentAdmins.join("\n")));
        return;
    }

    // Просим подтвердить удаление
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Подтверждение удаления",
                                                              QString("Вы уверены, что хотите удалить администратора?\n\n"
                                                                      "Логин: %1\n\n"
                                                                      "После удаления этот администратор не сможет заходить в админ-панель.")
                                                                  .arg(login),
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                            " Попытка удаления администратора: " + login);

        bool success = serverController->removeAdmin(login);

        if (success) {
            logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                                " Удален администратор: " + login);
            QMessageBox::information(this, "Успех",
                                     QString("Администратор '%1' успешно удален!").arg(login));

            // После удаления обновляем админов
            refreshAdminList();
        } else {
            logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                                " Ошибка удаления администратора: " + login);
            QMessageBox::warning(this, "Ошибка",
                                 QString("Не удалось удалить администратора '%1'!\n\n"
                                         "Возможные причины:\n"
                                         "• Это последний администратор\n"
                                         "• Ошибка на сервере\n"
                                         "• Проблема с файлом admins.txt")
                                     .arg(login));
        }
    }
}

void MainWindow::changeAdminPassword() {
    if (!serverController || !serverRunning) {
        QMessageBox::warning(this, "Ошибка", "Сервер не запущен!");
        return;
    }

    QList<QListWidgetItem*> selected = adminsList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Информация", "Выберите администратора для смены пароля!");
        return;
    }

    QString adminInfo = selected.first()->text();
    QString login = adminInfo.split(" ").first();

    if (login.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось извлечь логин администратора!");
        return;
    }

    bool ok;
    QString newPassword = QInputDialog::getText(this, "Сменить пароль администратора",
                                                "Новый пароль для " + login + ":",
                                                QLineEdit::Normal, "", &ok);
    if (!ok || newPassword.isEmpty()) return;

    QString confirmPassword = QInputDialog::getText(this, "Подтверждение пароля",
                                                    "Подтвердите новый пароль:",
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || confirmPassword.isEmpty()) return;

    if (newPassword != confirmPassword) {
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают!");
        return;
    }

    if (serverController->changeAdminPassword(login, newPassword)) {
        logTextEdit->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") +
                            "Пароль администратора " + login + " изменен");
        QMessageBox::information(this, "Успех", "Пароль администратора изменен!");
        refreshAdminList();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось изменить пароль администратора!");
    }
}
