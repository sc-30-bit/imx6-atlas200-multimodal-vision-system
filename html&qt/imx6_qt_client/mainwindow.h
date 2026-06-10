#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QTabWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void onTestServiceClicked();
    void onSnapshotTimer();
    void onToggleSnapshotClicked();
    void onNetworkReplyFinished(QNetworkReply *reply);

private:
    void initUi();
    void setAnalyzingState(bool analyzing);
    void setPresetPrompt(const QString &prompt);

    void showReturnedImage(const QString &base64Image);
    void appendHistory(const QString &prompt, const QString &result, double costTime);

    QString getPcIp() const;
    QString getAnalyzeUrl() const;
    QString getSnapshotUrl() const;
    QString getStatusUrl() const;

private:
    QTabWidget *tabWidget;

    QLabel *titleLabel;
    QLabel *infoLabel;
    QLabel *imageLabel;
    QLabel *statusLabel;

    QLineEdit *ipEdit;
    QTextEdit *promptEdit;
    QTextEdit *resultEdit;
    QTextEdit *historyEdit;

    QPushButton *testButton;
    QPushButton *snapshotButton;
    QPushButton *analyzeButton;
    QPushButton *exitButton;

    QPushButton *presetDescButton;
    QPushButton *presetTrafficButton;
    QPushButton *presetRiskButton;
    QPushButton *presetCountButton;

    QTimer *snapshotTimer;
    QNetworkAccessManager *networkManager;

    QString lastPrompt;
    bool snapshotBusy;
};

#endif // MAINWINDOW_H
