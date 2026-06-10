#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QUrl>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QPixmap>
#include <QDateTime>
#include <QByteArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      snapshotBusy(false)
{
    networkManager = new QNetworkAccessManager(this);

    connect(networkManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(onNetworkReplyFinished(QNetworkReply*)));

    snapshotTimer = new QTimer(this);
    connect(snapshotTimer, SIGNAL(timeout()),
            this, SLOT(onSnapshotTimer()));

    initUi();

    // LCD 屏上别刷太快，1s 一帧比较稳
    snapshotTimer->start(1000);
}

MainWindow::~MainWindow()
{
    if (snapshotTimer->isActive()) {
        snapshotTimer->stop();
    }
}

void MainWindow::initUi()
{
    QWidget *centralWidget = new QWidget(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    titleLabel = new QLabel("嵌入式多模态视觉分析系统", this);

    QFont titleFont;
    titleFont.setPointSize(13);
    titleFont.setBold(true);

    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFixedHeight(24);

    infoLabel = new QLabel("匿名成员 A    匿名成员 B", this);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setFixedHeight(20);
    infoLabel->setStyleSheet("font-size: 12px; color: #333333;");

    tabWidget = new QTabWidget(this);

    // =========================================================
    // Tab 1: 主分析页面
    // =========================================================
    QWidget *analyzePage = new QWidget(this);
    QHBoxLayout *rootLayout = new QHBoxLayout(analyzePage);
    rootLayout->setContentsMargins(2, 2, 2, 2);
    rootLayout->setSpacing(6);

    // 左侧：当前帧 + 状态
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(4);

    imageLabel = new QLabel(this);
    imageLabel->setMinimumSize(380, 285);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setText("正在从 PC 获取当前画面...");
    imageLabel->setStyleSheet(
        "background-color: black;"
        "color: white;"
        "border: 1px solid gray;"
    );

    statusLabel = new QLabel("状态：等待操作", this);
    statusLabel->setFixedHeight(22);
    statusLabel->setStyleSheet("color: #0066cc; font-size: 12px;");

    leftLayout->addWidget(imageLabel, 1);
    leftLayout->addWidget(statusLabel);

    // 右侧：紧凑控制区
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(4);

    QHBoxLayout *ipLayout = new QHBoxLayout();
    QLabel *ipLabel = new QLabel("PC:", this);
    ipLabel->setFixedWidth(26);

    ipEdit = new QLineEdit(this);
    ipEdit->setText("192.168.137.101");
    ipEdit->setMinimumHeight(26);

    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(ipEdit);

    QHBoxLayout *topButtonLayout = new QHBoxLayout();

    testButton = new QPushButton("测试", this);
    snapshotButton = new QPushButton("暂停刷新", this);
    exitButton = new QPushButton("退出", this);

    testButton->setMinimumHeight(28);
    snapshotButton->setMinimumHeight(28);
    exitButton->setMinimumHeight(28);

    topButtonLayout->addWidget(testButton);
    topButtonLayout->addWidget(snapshotButton);
    topButtonLayout->addWidget(exitButton);

    QLabel *promptLabel = new QLabel("提示词：", this);
    promptLabel->setFixedHeight(18);

    promptEdit = new QTextEdit(this);
    promptEdit->setMinimumHeight(55);
    promptEdit->setMaximumHeight(70);
    promptEdit->setText("请判断当前画面中是否有车辆或行人，只输出一句话。");
    promptEdit->setPlaceholderText("请输入分析提示词");

    QGridLayout *presetLayout = new QGridLayout();
    presetLayout->setSpacing(3);

    presetDescButton = new QPushButton("描述", this);
    presetTrafficButton = new QPushButton("交通", this);
    presetRiskButton = new QPushButton("风险", this);
    presetCountButton = new QPushButton("计数", this);

    presetDescButton->setMinimumHeight(27);
    presetTrafficButton->setMinimumHeight(27);
    presetRiskButton->setMinimumHeight(27);
    presetCountButton->setMinimumHeight(27);

    presetLayout->addWidget(presetDescButton, 0, 0);
    presetLayout->addWidget(presetTrafficButton, 0, 1);
    presetLayout->addWidget(presetRiskButton, 1, 0);
    presetLayout->addWidget(presetCountButton, 1, 1);

    analyzeButton = new QPushButton("分析当前画面", this);
    analyzeButton->setMinimumHeight(36);
    analyzeButton->setStyleSheet("font-weight: bold;");

    QLabel *resultLabel = new QLabel("本次结果：", this);
    resultLabel->setFixedHeight(18);

    resultEdit = new QTextEdit(this);
    resultEdit->setReadOnly(true);
    resultEdit->setMinimumHeight(85);
    resultEdit->setPlaceholderText("MiniCPM-V 结果");

    rightLayout->addLayout(ipLayout);
    rightLayout->addLayout(topButtonLayout);
    rightLayout->addWidget(promptLabel);
    rightLayout->addWidget(promptEdit);
    rightLayout->addLayout(presetLayout);
    rightLayout->addWidget(analyzeButton);
    rightLayout->addWidget(resultLabel);
    rightLayout->addWidget(resultEdit, 1);

    rootLayout->addLayout(leftLayout, 5);
    rootLayout->addLayout(rightLayout, 3);

    // =========================================================
    // Tab 2: 历史记录页面
    // =========================================================
    QWidget *historyPage = new QWidget(this);
    QVBoxLayout *historyLayout = new QVBoxLayout(historyPage);
    historyLayout->setContentsMargins(4, 4, 4, 4);
    historyLayout->setSpacing(4);

    QLabel *historyLabel = new QLabel("分析历史记录", this);
    historyLabel->setAlignment(Qt::AlignCenter);
    historyLabel->setFixedHeight(24);

    historyEdit = new QTextEdit(this);
    historyEdit->setReadOnly(true);
    historyEdit->setPlaceholderText("最近几次分析记录将在这里显示");

    historyLayout->addWidget(historyLabel);
    historyLayout->addWidget(historyEdit, 1);

    tabWidget->addTab(analyzePage, "分析");
    tabWidget->addTab(historyPage, "历史");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(tabWidget, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("IMX6 多模态视觉分析客户端");

    connect(testButton, SIGNAL(clicked()),
            this, SLOT(onTestServiceClicked()));

    connect(snapshotButton, SIGNAL(clicked()),
            this, SLOT(onToggleSnapshotClicked()));

    connect(analyzeButton, SIGNAL(clicked()),
            this, SLOT(onAnalyzeClicked()));

    connect(exitButton, SIGNAL(clicked()),
            this, SLOT(close()));

    connect(presetDescButton, &QPushButton::clicked, this, [this]() {
        setPresetPrompt("请用中文简要描述当前画面的主要内容，控制在80字以内。");
    });

    connect(presetTrafficButton, &QPushButton::clicked, this, [this]() {
        setPresetPrompt("请分析当前画面中的交通参与者，例如行人、车辆、道路环境，并给出一句简洁判断。");
    });

    connect(presetRiskButton, &QPushButton::clicked, this, [this]() {
        setPresetPrompt("请判断当前画面中是否存在火灾、交通事故、人摔倒或其他安全风险，如果有请简要说明。");
    });

    connect(presetCountButton, &QPushButton::clicked, this, [this]() {
        setPresetPrompt("请统计当前画面中可见的主要目标类别和数量，例如行人、车辆等。");
    });
}

QString MainWindow::getPcIp() const
{
    return ipEdit->text().trimmed();
}

QString MainWindow::getAnalyzeUrl() const
{
    return QString("http://%1:8000/analyze").arg(getPcIp());
}

QString MainWindow::getSnapshotUrl() const
{
    return QString("http://%1:8000/snapshot").arg(getPcIp());
}

QString MainWindow::getStatusUrl() const
{
    return QString("http://%1:8000/").arg(getPcIp());
}

void MainWindow::setPresetPrompt(const QString &prompt)
{
    promptEdit->setText(prompt);
    statusLabel->setText("状态：已填入预设提示词");
}

void MainWindow::setAnalyzingState(bool analyzing)
{
    analyzeButton->setEnabled(!analyzing);
    testButton->setEnabled(!analyzing);

    if (analyzing) {
        analyzeButton->setText("分析中...");
        statusLabel->setText("状态：正在请求 PC 端分析");
    } else {
        analyzeButton->setText("分析当前画面");
    }
}

void MainWindow::onToggleSnapshotClicked()
{
    if (snapshotTimer->isActive()) {
        snapshotTimer->stop();
        snapshotButton->setText("继续刷新");
        statusLabel->setText("状态：画面刷新已暂停");
    } else {
        snapshotTimer->start(1000);
        snapshotButton->setText("暂停刷新");
        statusLabel->setText("状态：画面刷新已开启");
    }
}

void MainWindow::onSnapshotTimer()
{
    if (snapshotBusy) {
        return;
    }

    QString pcIp = getPcIp();

    if (pcIp.isEmpty()) {
        return;
    }

    snapshotBusy = true;

    QUrl url(getSnapshotUrl());
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("requestType", "snapshot");
}

void MainWindow::onTestServiceClicked()
{
    QString pcIp = getPcIp();

    if (pcIp.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入 PC IP 地址");
        return;
    }

    QUrl url(getStatusUrl());
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("requestType", "status");

    statusLabel->setText("状态：正在测试 PC 服务");
}

void MainWindow::onAnalyzeClicked()
{
    QString pcIp = getPcIp();
    QString prompt = promptEdit->toPlainText().trimmed();

    if (pcIp.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入 PC IP 地址");
        return;
    }

    if (prompt.isEmpty()) {
        prompt = "请简要描述当前画面。";
    }

    lastPrompt = prompt;

    QUrl url(getAnalyzeUrl());
    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json; charset=utf-8"
    );

    QJsonObject json;
    json.insert("prompt", prompt);
    json.insert("return_image", true);

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    resultEdit->clear();
    resultEdit->setText("正在抓取当前帧并分析...");

    setAnalyzingState(true);

    QNetworkReply *reply = networkManager->post(request, postData);
    reply->setProperty("requestType", "analyze");
}

void MainWindow::showReturnedImage(const QString &base64Image)
{
    if (base64Image.isEmpty()) {
        imageLabel->setText("后端未返回 image_base64");
        return;
    }

    QByteArray imgBytes = QByteArray::fromBase64(base64Image.toUtf8());

    QPixmap pixmap;
    bool ok = pixmap.loadFromData(imgBytes);

    if (!ok || pixmap.isNull()) {
        imageLabel->setText("当前帧图片解析失败");
        return;
    }

    imageLabel->setPixmap(
        pixmap.scaled(
            imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
        )
    );
}

void MainWindow::appendHistory(const QString &prompt, const QString &result, double costTime)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");

    QString item;
    item += "[" + timeStr + "] ";
    item += "耗时 " + QString::number(costTime, 'f', 2) + "s\n";
    item += "Q: " + prompt + "\n";
    item += "A: " + result + "\n";
    item += "------------------------------\n";

    historyEdit->append(item);
}

void MainWindow::onNetworkReplyFinished(QNetworkReply *reply)
{
    QString requestType = reply->property("requestType").toString();

    if (requestType == "snapshot") {
        snapshotBusy = false;
    }

    if (requestType == "analyze") {
        setAnalyzingState(false);
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();

        if (requestType == "snapshot") {
            statusLabel->setText("状态：画面刷新失败");
        } else if (requestType == "status") {
            statusLabel->setText("状态：PC 服务连接失败");
            resultEdit->setText("PC 服务连接失败：\n" + err);
        } else {
            resultEdit->setText("请求失败：\n" + err);
            statusLabel->setText("状态：请求失败");
        }

        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (requestType == "status") {
            statusLabel->setText("状态：PC 服务在线");
            resultEdit->setText("PC 服务响应：\n" + QString::fromUtf8(responseData));
        } else if (requestType == "snapshot") {
            statusLabel->setText("状态：画面 JSON 解析失败");
        } else {
            resultEdit->setText("JSON 解析失败：\n" + QString::fromUtf8(responseData));
            statusLabel->setText("状态：JSON 解析失败");
        }

        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();

    if (requestType == "status") {
        statusLabel->setText("状态：PC 服务在线");
        resultEdit->setText("PC 服务在线");
        reply->deleteLater();
        return;
    }

    QString status = obj.value("status").toString();

    if (requestType == "snapshot") {
        if (status == "ok") {
            QString imageBase64 = obj.value("image_base64").toString();
            showReturnedImage(imageBase64);
            statusLabel->setText("状态：画面已刷新");
        } else {
            QString message = obj.value("message").toString();
            statusLabel->setText("状态：画面刷新失败 " + message);
        }

        reply->deleteLater();
        return;
    }

    if (status == "ok") {
        QString result = obj.value("result").toString();
        double costTime = obj.value("cost_time").toDouble();
        QString imageBase64 = obj.value("image_base64").toString();

        showReturnedImage(imageBase64);

        QString displayText;
        displayText += result;

        if (costTime > 0.0) {
            displayText += QString("\n\n耗时：%1 秒").arg(costTime, 0, 'f', 3);
        }

        resultEdit->setText(displayText);
        appendHistory(lastPrompt, result, costTime);

        statusLabel->setText("状态：分析完成");
    } else {
        QString message = obj.value("message").toString();

        if (message.isEmpty()) {
            message = obj.value("detail").toString();
        }

        if (message.isEmpty()) {
            message = "未知错误";
        }

        resultEdit->setText("分析失败：\n" + message);
        statusLabel->setText("状态：分析失败");
    }

    reply->deleteLater();
}
