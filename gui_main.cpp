#include <windows.h>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTextBrowser>

#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QFont>
#include <QColor>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QLibrary>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTextBlock>


#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <chrono>

using namespace std;

typedef int (*Func_Init)();
typedef int (*Func_Load)(const char*);
typedef int (*Func_Count)();
typedef int (*Func_List)(char*, int);
typedef int (*Func_Freq)(int, char*, int);
typedef int (*Func_Global)(char*, int);
typedef int (*Func_Del)(int);
typedef int (*Func_DelAll)();
typedef int (*Func_Search)(const char*, int, char*, int);
typedef void (*Func_Release)();

static QLibrary* g_lib = nullptr;
static Func_Init InitEngine = nullptr;
static Func_Load LoadDocuments = nullptr;
static Func_Count GetDocCount = nullptr;
static Func_List GetFileList = nullptr;
static Func_Freq GetFileWordFreq = nullptr;
static Func_Global GetGlobalWordFreq = nullptr;
static Func_Del DeleteDoc = nullptr;
static Func_DelAll DeleteAllDocs = nullptr;
static Func_Search Search = nullptr;
static Func_Release ReleaseEngine = nullptr;

bool loadDLL() {
    QStringList paths;
    paths << "SearchBridge64.dll" << "./SearchBridge64.dll"
          << QCoreApplication::applicationDirPath() + "/SearchBridge64.dll"
          << QCoreApplication::applicationDirPath() + "/../SearchBridge64.dll"
          << QCoreApplication::applicationDirPath() + "/release/SearchBridge64.dll";
    for (const QString& p : paths) {
        QLibrary* lib = new QLibrary(p);
        if (lib->load()) {
            InitEngine = (Func_Init)lib->resolve("InitEngine");
            LoadDocuments = (Func_Load)lib->resolve("LoadDocuments");
            GetDocCount = (Func_Count)lib->resolve("GetDocCount");
            GetFileList = (Func_List)lib->resolve("GetFileList");
            GetFileWordFreq = (Func_Freq)lib->resolve("GetFileWordFreq");
            GetGlobalWordFreq = (Func_Global)lib->resolve("GetGlobalWordFreq");
            DeleteDoc = (Func_Del)lib->resolve("DeleteDoc");
            DeleteAllDocs = (Func_DelAll)lib->resolve("DeleteAllDocs");
            Search = (Func_Search)lib->resolve("Search");
            ReleaseEngine = (Func_Release)lib->resolve("ReleaseEngine");
            if (InitEngine && LoadDocuments && GetDocCount && Search && ReleaseEngine) {
                g_lib = lib; return true;
            }
            delete lib;
        } else { delete lib; }
    }
    return false;
}

// JSON 解析
vector<map<QString,QString>> parseJson(const QString& json) {
    vector<map<QString,QString>> result;
    int pos = 0;
    while ((pos = json.indexOf('{', pos)) != -1) {
        int end = json.indexOf('}', pos);
        if (end == -1) break;
        QString item = json.mid(pos+1, end-pos-1);
        map<QString,QString> kv;
        for (const QString& pair : item.split(',')) {
            int colon = pair.indexOf(':');
            if (colon == -1) continue;
            kv[pair.mid(0,colon).trimmed().remove('"')] = pair.mid(colon+1).trimmed().remove('"');
        }
        result.push_back(kv);
        pos = end+1;
    }
    return result;
}

// 工作线程（仅用于加载文件夹，因为涉及文件IO可能较慢）
class LoadWorker : public QThread {
    Q_OBJECT
public:
    LoadWorker(const string& p) : path(p) {}
signals:
    void done(int count);
protected:
    void run() override {
        int n = LoadDocuments ? LoadDocuments(path.c_str()) : -1;
        emit done(n);
    }
private:
    string path;
};

class SearchWorker : public QThread {
    Q_OBJECT
public:
    SearchWorker(const string& q, int k) : query(q), topK(k) {}
signals:
    void done(const QString& json);
protected:
    void run() override {
        char buf[1024*1024];
        if (Search) Search(query.c_str(), topK, buf, sizeof(buf));
        emit done(QString::fromUtf8(buf));
    }
private:
    string query;
    int topK;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
    fprintf(stderr, "A\n"); fflush(stderr);
    initUI();
    fprintf(stderr, "B\n"); fflush(stderr);
    if (InitEngine) InitEngine();
    fprintf(stderr, "C\n"); fflush(stderr);
    refreshAll();
    fprintf(stderr, "D\n"); fflush(stderr);
}

    ~MainWindow() { if (ReleaseEngine) ReleaseEngine(); }

private slots:
    void onLoadFolder() {
        QString dir = QFileDialog::getExistingDirectory(this, "选择文档文件夹");
        if (dir.isEmpty()) return;
        setButtonsEnabled(false);
        progressBar->setVisible(true); progressBar->setMaximum(0);
        statusLabel->setText("加载中...");
        LoadWorker* w = new LoadWorker(dir.toStdString());
        connect(w, &LoadWorker::done, this, [this](int n) {
            refreshAll();
            statusLabel->setText(QString("加载完成！%1 个文档").arg(n));
            setButtonsEnabled(true); progressBar->setVisible(false);
        });
        connect(w, &QThread::finished, w, &QObject::deleteLater);
        w->start();
    }

    void onDeleteFile() {
        auto items = fileList->selectedItems();
        if (items.isEmpty()) { QMessageBox::information(this, "提示", "请先选择文件"); return; }
        int docId = items[0]->data(Qt::UserRole).toInt();
        if (docId < 0) return;
        if (QMessageBox::question(this, "确认", "删除此文件？") != QMessageBox::Yes) return;
        if (DeleteDoc) DeleteDoc(docId);
        refreshAll();
        statusLabel->setText("已删除");
    }

    void onDeleteAll() {
        if (GetDocCount && GetDocCount() == 0) return;
        if (QMessageBox::question(this, "确认", "清空所有文件？") != QMessageBox::Yes) return;
        if (DeleteAllDocs) DeleteAllDocs();
        refreshAll();
        statusLabel->setText("已清空");
    }

    void onSearch() {
        QString q = searchInput->text().trimmed();
        if (q.isEmpty()) { QMessageBox::information(this, "提示", "请输入查询词"); return; }
        if (!GetDocCount || GetDocCount() == 0) { QMessageBox::information(this, "提示", "请先加载文档"); return; }
        setButtonsEnabled(false);
        statusLabel->setText("搜索中...");
        SearchWorker* w = new SearchWorker(q.toStdString(), topKSpin->value());
        connect(w, &SearchWorker::done, this, [this](const QString& json) {
            showSearchResults(json);
            setButtonsEnabled(true);
            statusLabel->setText("搜索完成");
        });
        connect(w, &QThread::finished, w, &QObject::deleteLater);
        w->start();
    }

    void onBenchmark() {
        if (!GetDocCount || GetDocCount() == 0) {
            QMessageBox::information(this, "提示", "请先加载文档");
            return;
        }
        QString fp = QFileDialog::getOpenFileName(this, "选择关键词文件", "", "文本文件 (*.txt);;所有文件 (*)");
        if (fp.isEmpty()) return;
        QFile f(fp);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "错误", "无法打开"); return;
        }
        QStringList queries;
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString l = in.readLine().trimmed();
            if (!l.isEmpty()) queries << l;
        }
        f.close();
        if (queries.isEmpty()) { QMessageBox::information(this, "提示", "文件为空"); return; }
        if (QMessageBox::question(this, "确认", QString("读取 %1 个关键词\n\n是否开始？").arg(queries.size())) != QMessageBox::Yes) return;

        // 同步执行 Benchmark，使用 processEvents 保持界面响应
        setButtonsEnabled(false);
        progressBar->setVisible(true);
        progressBar->setMaximum(queries.size());
        progressBar->setValue(0);
        statusLabel->setText("Benchmark 测试中...");

        char buf[1024*1024];
        QVector<double> times;
        QVector<int> counts;
        QStringList validQs;

        for (int i = 0; i < queries.size(); i++) {
            QString q = queries[i].trimmed();
            if (q.isEmpty()) continue;

            auto start = chrono::high_resolution_clock::now();
            int n = Search ? Search(q.toUtf8().constData(), topKSpin->value(), buf, sizeof(buf)) : 0;
            auto end = chrono::high_resolution_clock::now();
            double sec = chrono::duration<double>(end - start).count();

            times.push_back(sec);
            counts.push_back(max(0, n));
            validQs.push_back(q);

            progressBar->setValue(i + 1);
            QApplication::processEvents();
        }

        if (times.isEmpty()) {
            resultText->setPlainText("无有效数据");
        } else {
            // 生成报告
            QVector<int> indices;
            for (int i = 0; i < times.size(); i++) indices.push_back(i);
            sort(indices.begin(), indices.end(), [&](int a, int b) {
                if (counts[a] != counts[b]) return counts[a] > counts[b];
                return validQs[a] < validQs[b];
            });

            double totalTime = 0;
            for (double t : times) totalTime += t;
            double avgTime = totalTime / times.size();

            int fi = 0, si = 0;
            for (int i = 0; i < times.size(); i++) {
                if (times[i] < times[fi]) fi = i;
                if (times[i] > times[si]) si = i;
            }

            QString r;
            r += QString(70,'=')+"\n              BENCHMARK \u6027\u80fd\u6d4b\u8bd5\u62a5\u544a\n"+QString(70,'=')+"\n";
            r += QString("  \u6d4b\u8bd5\u65f6\u95f4: %1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            r += QString("  \u6587\u6863\u603b\u6570: %1\n").arg(GetDocCount ? GetDocCount() : 0);
            r += QString("  \u6d4b\u8bd5\u6b21\u6570: %1\n").arg(times.size());
            r += QString("  \u603b\u8fd0\u884c\u65f6\u957f: %1 \u79d2\n").arg(totalTime,0,'f',4);
            r += QString("  \u5355\u6b21\u5e73\u5747\u8017\u65f6: %1 \u79d2\n").arg(avgTime,0,'f',6);
            r += QString("  \u6700\u5feb\u68c0\u7d22: %1 \u79d2 (%2)\n").arg(times[fi],0,'f',6).arg(validQs[fi]);
            r += QString("  \u6700\u6162\u68c0\u7d22: %1 \u79d2 (%2)\n").arg(times[si],0,'f',6).arg(validQs[si]);
            r += QString(70,'-')+"\n  \u6392\u540d  \u5173\u952e\u8bcd              \u7ed3\u679c\u6570  \u8017\u65f6(\u79d2)\n"+QString(70,'-')+"\n";
            for (int rk = 0; rk < indices.size(); rk++) {
                int i = indices[rk];
                r += QString("  %1  %2  %3  %4\n").arg(rk+1,4).arg(validQs[i],18).arg(counts[i],8).arg(times[i],12,'f',6);
            }
            r += QString(70,'=')+"\n";
            resultText->setPlainText(r);
        }
        statusLabel->setText("Benchmark \u6d4b\u8bd5\u5b8c\u6210\uff01");
        setButtonsEnabled(true);
        progressBar->setVisible(false);
    }

    void onFileClicked(QListWidgetItem* item) {
        int docId = item->data(Qt::UserRole).toInt();
        if (docId < 0) return;
        if (!GetFileWordFreq) return;
        char buf[1024*1024];
        int n = GetFileWordFreq(docId, buf, sizeof(buf));
        if (n <= 0) return;
        freqText->clear();
        QTextCursor cur = freqText->textCursor();
        QTextCharFormat f;
        f.setFontPointSize(10); f.setFontWeight(QFont::Bold); f.setForeground(QColor("#000"));
        cur.insertText(QString("\u6587\u4ef6\u8bcd\u9891\uff08%1 \u4e2a\u552f\u4e00\u8bcd\uff09\n\n").arg(n), f);
        f.setForeground(QColor("#003366")); f.setFontPointSize(9);
        cur.insertText("  \u6392\u540d  \u8bcd                \u8bcd\u9891\n  "+QString(30,QChar(0x2500))+"\n", f);
        auto items = parseJson(QString::fromUtf8(buf));
        for (auto& it : items) {
            int rank = it["rank"].toInt();
            f.setForeground(rank <= 5 ? QColor("#C00") : QColor("#333"));
            f.setFontWeight(rank <= 5 ? QFont::Bold : QFont::Normal); f.setFontPointSize(9);
            cur.insertText(QString("  %1  %2  %3\n").arg(rank,5).arg(it["word"],16).arg(it["freq"].toInt(),8), f);
        }
        freqInfo->setText("文件词频统计");
        freqStat->setText(QString("%1 个唯一词").arg(n));
    }

    void onFileDoubleClicked(QListWidgetItem* item) {
        QString fp = item->data(Qt::UserRole+1).toString();
        if (!fp.isEmpty() && QFile::exists(fp)) QDesktopServices::openUrl(QUrl::fromLocalFile(fp));
    }

private:
    void initUI() {
        setWindowTitle("搜索引擎 - DLL版");
        resize(1200,800);
        QWidget* c = new QWidget(this); setCentralWidget(c);
        QVBoxLayout* ml = new QVBoxLayout(c); ml->setContentsMargins(8,8,8,8);
        QSplitter* sp = new QSplitter(Qt::Horizontal); ml->addWidget(sp);
        // 左
        QWidget* lw = new QWidget(); QVBoxLayout* ll = new QVBoxLayout(lw); ll->setContentsMargins(0,0,5,0);
        QGroupBox* fg = new QGroupBox("\u8bcd\u9891\u7edf\u8ba1"); QVBoxLayout* fl = new QVBoxLayout(fg);
        freqInfo = new QLabel("点击文件查看词频"); freqInfo->setStyleSheet("color:#888;"); fl->addWidget(freqInfo);
        btnBackGlobal = new QPushButton("返回全局词频"); btnBackGlobal->setFixedHeight(22); fl->addWidget(btnBackGlobal);

        freqText = new QTextEdit(); freqText->setReadOnly(true); freqText->setFont(QFont("Consolas",9)); freqText->setMinimumWidth(300); fl->addWidget(freqText);
        freqStat = new QLabel(""); freqStat->setStyleSheet("color:#888;"); fl->addWidget(freqStat);
        ll->addWidget(fg); lw->setMinimumWidth(300); sp->addWidget(lw);
        // 中
        QWidget* mw = new QWidget(); QVBoxLayout* ml2 = new QVBoxLayout(mw); ml2->setContentsMargins(3,0,3,0);
        QGroupBox* dg = new QGroupBox("\u6587\u6863\u5217\u8868"); QVBoxLayout* dgl = new QVBoxLayout(dg);
        QHBoxLayout* bl = new QHBoxLayout();
        btnFolder = new QPushButton("加载文件夹"); btnDelete = new QPushButton("删除选中");
        btnDeleteAll = new QPushButton("清空所有"); btnBenchmark = new QPushButton("Benchmark");
        bl->addWidget(btnFolder); bl->addWidget(btnDelete); bl->addWidget(btnDeleteAll); bl->addWidget(btnBenchmark);
        dgl->addLayout(bl);
        docCountLabel = new QLabel("文档数: 0"); docCountLabel->setStyleSheet("font-weight:bold;"); dgl->addWidget(docCountLabel);
        fileList = new QListWidget(); fileList->setMinimumWidth(250); fileList->setAlternatingRowColors(true); dgl->addWidget(fileList);
        ml2->addWidget(dg); sp->addWidget(mw);
        // 右
        QWidget* rw = new QWidget(); QVBoxLayout* rl = new QVBoxLayout(rw); rl->setContentsMargins(5,0,0,0);
        QGroupBox* sg = new QGroupBox("\u641c\u7d22"); QHBoxLayout* sl = new QHBoxLayout(sg);
        sl->addWidget(new QLabel("查询词:")); searchInput = new QLineEdit(); searchInput->setPlaceholderText("输入关键词..."); sl->addWidget(searchInput);
        sl->addWidget(new QLabel("Top-K:")); topKSpin = new QSpinBox(); topKSpin->setRange(1,100); topKSpin->setValue(10); sl->addWidget(topKSpin);
        btnSearch = new QPushButton("搜索"); sl->addWidget(btnSearch); rl->addWidget(sg);
        QGroupBox* rg = new QGroupBox("搜索结果（点击文件名打开文件）"); QVBoxLayout* rgl = new QVBoxLayout(rg);
        resultText = new QTextBrowser(); resultText->setOpenExternalLinks(false); resultText->setOpenLinks(false); resultText->setFont(QFont("Microsoft YaHei",10)); rgl->addWidget(resultText);


        QHBoxLayout* pl = new QHBoxLayout(); pageLabel = new QLabel(""); pageLabel->setStyleSheet("color:#666;"); pl->addWidget(pageLabel);
        btnFirst = new QPushButton("首页"); btnPrev = new QPushButton("上页"); btnNext = new QPushButton("下页"); btnLast = new QPushButton("末页");
        for (auto b : {btnFirst, btnPrev, btnNext, btnLast}) { b->setFixedWidth(60); pl->addWidget(b); }
        pl->addStretch(); pl->addWidget(new QLabel("跳转:")); jumpInput = new QLineEdit(); jumpInput->setFixedWidth(40); pl->addWidget(jumpInput);
        btnJump = new QPushButton("GO"); btnJump->setFixedWidth(30); pl->addWidget(btnJump);
        rgl->addLayout(pl); rl->addWidget(rg); sp->addWidget(rw);
        sp->setSizes({320,280,600});
        statusLabel = new QLabel("就绪"); statusBar()->addWidget(statusLabel);
        progressBar = new QProgressBar(); progressBar->setMaximumWidth(200); progressBar->setVisible(false); statusBar()->addPermanentWidget(progressBar);
        // 信号
        connect(btnFolder, &QPushButton::clicked, this, &MainWindow::onLoadFolder);
        connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteFile);
        connect(btnDeleteAll, &QPushButton::clicked, this, &MainWindow::onDeleteAll);
        connect(btnBenchmark, &QPushButton::clicked, this, &MainWindow::onBenchmark);
        connect(btnSearch, &QPushButton::clicked, this, &MainWindow::onSearch);
        connect(searchInput, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
        connect(fileList, &QListWidget::itemClicked, this, &MainWindow::onFileClicked);
        connect(fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::onFileDoubleClicked);
        connect(btnFirst, &QPushButton::clicked, this, [this]{if(currentPage>1){currentPage=1;showPage();}});
        connect(btnPrev, &QPushButton::clicked, this, [this]{if(currentPage>1){currentPage--;showPage();}});
        connect(btnNext, &QPushButton::clicked, this, [this]{if(currentPage<totalPages){currentPage++;showPage();}});
        connect(btnLast, &QPushButton::clicked, this, [this]{if(currentPage<totalPages){currentPage=totalPages;showPage();}});
        connect(btnJump, &QPushButton::clicked, this, [this]{bool ok;int p=jumpInput->text().toInt(&ok);if(ok&&p>=1&&p<=totalPages){currentPage=p;showPage();}});
        connect(btnBackGlobal, &QPushButton::clicked, this, [this]{
    showGlobalFreq();
    freqInfo->setText("全局词频统计");
});
connect(resultText, &QTextBrowser::anchorClicked, this, [this](const QUrl& url){
    QString fp = url.toLocalFile();
    if (fp.isEmpty()) fp = url.toString();
    // 手动处理反斜杠和URL编码
    fp.replace("file:///", "");
    fp.replace("%5C", "\\").replace("%2F", "/");
    if (!fp.isEmpty() && QFile::exists(fp))
        QDesktopServices::openUrl(QUrl::fromLocalFile(fp));
});


        setStyleSheet("QGroupBox{border:1px solid #ccc;border-radius:6px;margin-top:8px;padding-top:8px;font-weight:bold;}QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 5px;}QPushButton{padding:4px 10px;border:1px solid #aaa;border-radius:4px;background:#f8f8f8;}QPushButton:hover{background:#e8e8e8;}QPushButton:disabled{color:#aaa;background:#f0f0f0;}");
    }

    void refreshAll() {
    int n = GetDocCount ? GetDocCount() : 0;
    docCountLabel->setText(QString("文档数: %1").arg(n));
    fileList->clear();
    if (n > 0 && GetFileList) {
        std::vector<char> buf(1024*1024);
        int cnt = GetFileList(buf.data(), (int)buf.size());
        if (cnt > 0) {
            auto items = parseJson(QString::fromUtf8(buf.data()));
            for (auto& item : items) {
                int docId = item["docId"].toInt();
                QString fp = item["file"];
                QString name = fp.mid(qMax(fp.lastIndexOf(QChar('\\')), fp.lastIndexOf(QChar('/'))) + 1);
                QListWidgetItem* li = new QListWidgetItem(QString("  %1  (%2)").arg(name).arg(item["wordCount"]));
                li->setData(Qt::UserRole, docId);
                li->setData(Qt::UserRole+1, fp);
                fileList->addItem(li);
            }
        }
    }
    showGlobalFreq();
    currentPage = 1; totalPages = 1;
    allResults.clear(); resultText->clear(); pageLabel->setText("");
}

void showGlobalFreq() {
    freqText->clear();
    int n = GetDocCount ? GetDocCount() : 0;
    if (n == 0) { freqInfo->setText("点击文件查看词频"); freqStat->setText(""); return; }
    if (!GetGlobalWordFreq) return;
    std::vector<char> buf(1024*1024);
    int cnt = GetGlobalWordFreq(buf.data(), (int)buf.size());
    if (cnt <= 0) return;
    auto items = parseJson(QString::fromUtf8(buf.data()));
    QTextCursor cur = freqText->textCursor(); QTextCharFormat f;
    f.setFontPointSize(10); f.setFontWeight(QFont::Bold); f.setForeground(QColor("#000"));
    cur.insertText("全局词频统计\n", f);
    f.setFontPointSize(9); f.setFontWeight(QFont::Normal); f.setForeground(QColor("#888"));
    cur.insertText(QString("唯一词数: %1\n\n").arg(cnt), f);
    f.setForeground(QColor("#003366")); f.setFontWeight(QFont::Bold);
    cur.insertText("  排名  词                频次\n  "+QString(40,QChar(0x2500))+"\n", f);
    for (int i = 0; i < (int)items.size() && i < 500; i++) {
        int rank = items[i]["rank"].toInt();
        f.setForeground(rank <= 5 ? QColor("#C00") : QColor("#333"));
        f.setFontWeight(rank <= 5 ? QFont::Bold : QFont::Normal); f.setFontPointSize(9);
        cur.insertText(QString("  %1  %2  %3\n").arg(rank,5).arg(items[i]["word"],16).arg(items[i]["freq"].toInt(),8), f);
    }
    freqInfo->setText("全局词频统计");
    freqStat->setText(QString("前 %1 | 唯一: %2").arg(min(cnt,500)).arg(cnt));
}



    void showSearchResults(const QString& json) {
        resultText->clear();
        allResults = parseJson(json);
        if (allResults.empty()) {
            QTextCursor cur = resultText->textCursor(); QTextCharFormat f;
            f.setForeground(QColor("#888")); f.setFontPointSize(11);
            cur.insertText("无匹配结果", f);
            pageLabel->setText(""); currentPage = 1; totalPages = 1; return;
        }
        currentPage = 1;
        totalPages = ((int)allResults.size() + pageSize - 1) / pageSize;
        showPage();
    }

    void showPage() {
    resultText->clear();
    if (allResults.empty()) return;
    int s = (currentPage-1)*pageSize, e = min(s+pageSize, (int)allResults.size());
    QString html;
    html += QString("<p style='color:#999;font-size:9pt;'>共 %1 条 | 显示第 %2-%3 条 | 第 %4/%5 页</p>")
        .arg(allResults.size()).arg(s+1).arg(e).arg(currentPage).arg(totalPages);
    html += "<hr>";
    for (int i = s; i < e; i++) {
        int idx = i-s;
        QString rankColor = (idx<3) ? "#C00" : "#333";
        QString rankWeight = (idx<3) ? "bold" : "normal";
        QString rankSize = (idx<3) ? "11pt" : "10pt";
        html += QString("<p style='margin:4px 0;'>");
        html += QString("<span style='color:%1;font-weight:%2;font-size:%3;'><b>#%4</b></span> ")
            .arg(rankColor).arg(rankWeight).arg(rankSize).arg(i+1);
        // 文件名作为可点击链接
        QString fp = allResults[i]["file"];
        html += QString("<a href='%1' style='color:#06C;font-weight:bold;font-size:9pt;text-decoration:underline;'>%2</a> ")
            .arg(fp).arg(fp.mid(qMax(fp.lastIndexOf(QChar('\\')), fp.lastIndexOf(QChar('/'))) + 1));
        html += QString("<span style='color:#888;font-size:8pt;'>(得分: %1)</span><br>")
            .arg(allResults[i]["score"].toDouble(),0,'f',4);
        QString snip = allResults[i]["snippet"];
        snip.replace("\\n","<br>");
        html += QString("<span style='color:#444;font-size:9pt;margin-left:20px;'>%1</span>").arg(snip);
        html += "</p><hr style='border:0;border-top:1px dotted #ddd;'>";
    }
    resultText->setHtml(html);
    pageLabel->setText(QString("第 %1/%2 页").arg(currentPage).arg(totalPages));
    btnFirst->setEnabled(currentPage>1); btnPrev->setEnabled(currentPage>1);
    btnNext->setEnabled(currentPage<totalPages); btnLast->setEnabled(currentPage<totalPages);
}


    void setButtonsEnabled(bool e) {
        for (auto b : {btnFolder, btnDelete, btnDeleteAll, btnBenchmark, btnSearch}) b->setEnabled(e);
    }

    QPushButton *btnFolder, *btnDelete, *btnDeleteAll, *btnBenchmark, *btnSearch, *btnBackGlobal;
QPushButton *btnFirst, *btnPrev, *btnNext, *btnLast, *btnJump;
QLineEdit *searchInput, *jumpInput; QSpinBox *topKSpin;
QListWidget *fileList; QTextBrowser *resultText; QTextEdit *freqText;
QLabel *freqInfo, *freqStat, *docCountLabel, *statusLabel, *pageLabel;
QProgressBar* progressBar;

    vector<map<QString,QString>> allResults;
    int currentPage = 1, pageSize = 10, totalPages = 1;
};
int main(int argc, char* argv[]) {
    fprintf(stderr, "start\n"); fflush(stderr);
    QApplication app(argc, argv);
    fprintf(stderr, "app created\n"); fflush(stderr);
    if (!loadDLL()) {
        fprintf(stderr, "DLL FAILED\n"); fflush(stderr);
        return -1;
    }
    fprintf(stderr, "DLL OK\n"); fflush(stderr);
    MainWindow w;
    fprintf(stderr, "window created\n"); fflush(stderr);
    w.show();
    fprintf(stderr, "window shown\n"); fflush(stderr);
    return app.exec();
}


#include "gui_main.moc"
