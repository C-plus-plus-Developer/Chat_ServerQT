#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QThread>
#include <QTimer>
#include <QListWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QInputDialog>

class ServerController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void toggleServer();
    void updateStatistics();
    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString& error);
    void onLogMessage(const QString& message);

    // методы для управления админами
    void addAdmin();
    void removeAdmin();
    void changeAdminPassword();
    void refreshAdminList();

private:
    void onAdminSelectionChanged();
    void setupUI();
    void loadTestData();
    void updateRealStatistics();

    // UI элементы
    QPushButton *serverButton;
    QLabel *statusLabel;
    QTextEdit *logTextEdit;

    // Статистика
    QListWidget *adminsList;
    QListWidget *usersList;
    QLabel *adminsCountLabel;
    QLabel *usersCountLabel;

    // Кнопки управления админами
    QPushButton *addAdminButton;
    QPushButton *removeAdminButton;
    QPushButton *changePasswordButton;
    QPushButton *refreshAdminButton;

    // Таймер для обновления статистики
    QTimer *statsTimer;

    // Сервер
    ServerController *serverController;
    bool serverRunning;

    // Тестовые данные
    int adminCount;
    int userCount;
};

#endif
