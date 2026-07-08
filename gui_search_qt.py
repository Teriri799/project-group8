import os
import sys
import re
import math
import time
import threading
import subprocess
import collections
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QTextEdit, QListWidget, QListWidgetItem,
    QSplitter, QGroupBox, QMessageBox, QFileDialog, QSpinBox,
    QLineEdit, QStatusBar, QFrame, QTabWidget, QTableWidget,
    QTableWidgetItem, QHeaderView, QProgressBar, QInputDialog
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QTimer
from PyQt5.QtGui import QFont, QColor, QTextCursor, QIcon, QTextCharFormat

# 导入文本提取模块
import extract_text

# ========== 停用词 ==========
STOP_WORDS = set({
    "的","了","是","在","和","之","就","都","我","你","他","她","它","们",
    "这","那","有","中","上","个","人","大","小","多","少","去","来","不",
    "与","也","又","还","要","把","被","让","对","从","到","以","为",
    "很","会","能","可","但","而","或","一","二","三","四","五","六","七","八","九","十",
    "a","an","the","or","and","on","in","at","is","are","was","were","be",
    "been","being","have","has","had","do","does","did","but","if","so",
    "no","not","of","for","with","as","by","to","it","its","this","that",
    "年","月","日","时","分","秒"
})

def tokenize(text):
    """分词：提取字母/数字/中文字符，过滤停用词和单字"""
    words = re.findall(r'[a-zA-Z]+|[0-9]+|[\u4e00-\u9fff]+', text.lower())
    return [w for w in words if len(w) > 1 and w not in STOP_WORDS]

# ========== 搜索引擎核心 ==========
class SimpleSearchEngine:
    def __init__(self):
        self.docs = []
        self.invert_index = collections.defaultdict(list)
        self.doc_len = []
        self.total_docs = 0
        self.ready = False

    def clear(self):
        self.docs.clear()
        self.invert_index.clear()
        self.doc_len.clear()
        self.total_docs = 0
        self.ready = False

    def _rebuild_index(self):
        self.invert_index.clear()
        self.doc_len.clear()
        for doc in self.docs:
            self.doc_len.append(len(doc["words"]))
            for word, tf in doc["word_freq"].items():
                self.invert_index[word].append((doc["id"], tf))
        self.total_docs = len(self.docs)
        self.ready = len(self.docs) > 0

    def add_document(self, filepath, content):
        if not content:
            return False
        words = tokenize(content)
        if not words:
            return False
        
        doc_id = len(self.docs)
        word_freq = collections.Counter(words)
        
        self.docs.append({
            "id": doc_id,
            "file": filepath,
            "words": words,
            "content": content[:200],
            "word_freq": word_freq
        })
        self.doc_len.append(len(words))
        
        for word, tf in word_freq.items():
            self.invert_index[word].append((doc_id, tf))
        
        self.total_docs = len(self.docs)
        self.ready = True
        return True

    def remove_document(self, doc_id):
        if doc_id < 0 or doc_id >= len(self.docs):
            return False
        self.docs.pop(doc_id)
        self._rebuild_index()
        return True

    def remove_document_by_path(self, filepath):
        for i, doc in enumerate(self.docs):
            if doc["file"] == filepath:
                return self.remove_document(i)
        return False

    def load_folder(self, folder_path):
        count = 0
        for root, _, files in os.walk(folder_path):
            for fname in files:
                fpath = os.path.join(root, fname)
                try:
                    content = extract_text.extract_text(fpath)
                    if content and self.add_document(fpath, content):
                        count += 1
                except:
                    pass
        return count

    def get_doc_word_freq(self, doc_id):
        if doc_id < 0 or doc_id >= len(self.docs):
            return []
        freq = self.docs[doc_id]["word_freq"]
        return sorted(freq.items(), key=lambda x: (-x[1], x[0]))

    def get_global_word_freq(self, top_n=500):
        global_freq = collections.Counter()
        for doc in self.docs:
            global_freq.update(doc["word_freq"])
        return global_freq.most_common(top_n)

    def search(self, query, topK=10):
        if not self.ready or not query:
            return []
        
        query_words = tokenize(query)
        if not query_words:
            return []
        
        candidate_docs = None
        for word in query_words:
            if word not in self.invert_index:
                return []
            doc_set = {pid[0] for pid in self.invert_index[word]}
            if candidate_docs is None:
                candidate_docs = doc_set
            else:
                candidate_docs &= doc_set
            if not candidate_docs:
                return []
        
        N = self.total_docs
        scores = []
        for doc_id in candidate_docs:
            score = 0.0
            for word in query_words:
                df = len(self.invert_index[word])
                idf = math.log(1 + N / (1 + df))
                tf = 0
                for pid, cnt in self.invert_index[word]:
                    if pid == doc_id:
                        tf = cnt / self.doc_len[doc_id]
                        break
                score += tf * idf
            scores.append((doc_id, score))
        
        scores.sort(key=lambda x: (-x[1], x[0]))
        scores = scores[:topK]
        
        results = []
        for rank, (doc_id, score) in enumerate(scores, 1):
            doc = self.docs[doc_id]
            results.append({
                "rank": rank,
                "docId": doc_id,
                "file": doc["file"],
                "score": round(score, 6),
                "snippet": doc["content"][:150]
            })
        return results
    
    def benchmark_search(self, query, topK=10):
        """执行单次搜索并返回耗时(秒)和结果数量"""
        start = time.perf_counter()
        results = self.search(query, topK)
        elapsed = time.perf_counter() - start
        return elapsed, len(results)


# ========== 后台线程 ==========
class LoadThread(QThread):
    finished = pyqtSignal(int)
    status = pyqtSignal(str)
    
    def __init__(self, engine, folder=None, files=None):
        super().__init__()
        self.engine = engine
        self.folder = folder
        self.files = files
    
    def run(self):
        if self.folder:
            self.status.emit(f"正在加载文件夹: {self.folder}")
            n = self.engine.load_folder(self.folder)
            self.finished.emit(n)
        elif self.files:
            n = 0
            for fp in self.files:
                try:
                    content = extract_text.extract_text(fp)
                    if self.engine.add_document(fp, content):
                        n += 1
                except:
                    pass
            self.finished.emit(n)


class BenchmarkThread(QThread):
    progress = pyqtSignal(int, int)  # 当前, 总数
    result = pyqtSignal(str)
    finished = pyqtSignal(dict)
    
    def __init__(self, engine, queries, topK=10):
        super().__init__()
        self.engine = engine
        self.queries = queries
        self.topK = topK
    
    def run(self):
        times = []
        total_results = 0
        details = []
        
        total = len(self.queries)
        for i, q in enumerate(self.queries):
            q = q.strip()
            if not q:
                continue
            elapsed, count = self.engine.benchmark_search(q, self.topK)
            times.append(elapsed)
            total_results += count
            details.append((q, elapsed, count))
            self.progress.emit(i + 1, total)
        
        if not times:
            self.finished.emit({})
            return
        
        total_time = sum(times)
        avg_time = total_time / len(times)
        fastest = min(times)
        slowest = max(times)
        fastest_q = details[times.index(fastest)][0]
        slowest_q = details[times.index(slowest)][0]
        
        # 生成报告
        report = []
        report.append("=" * 60)
        report.append("            🔬 BENCHMARK 批量测试报告")
        report.append("=" * 60)
        report.append(f"  测试时间: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        report.append(f"  文档总数: {self.engine.total_docs}")
        report.append(f"  检索次数: {len(times)}")
        report.append(f"  总运行时长: {total_time:.4f} 秒")
        report.append(f"  单次平均耗时: {avg_time:.6f} 秒")
        report.append(f"  最快检索: {fastest:.6f} 秒 (关键词: {fastest_q})")
        report.append(f"  最慢检索: {slowest:.6f} 秒 (关键词: {slowest_q})")
        report.append("-" * 60)
        report.append(f"  {'关键词':<20} {'耗时(秒)':<15} {'结果数':<10}")
        report.append("-" * 60)
        for q, t, c in details:
            report.append(f"  {q:<20} {t:<15.6f} {c:<10}")
        report.append("=" * 60)
        
        self.result.emit("\n".join(report))
        
        self.finished.emit({
            "total_queries": len(times),
            "total_time": total_time,
            "avg_time": avg_time,
            "fastest": fastest,
            "fastest_q": fastest_q,
            "slowest": slowest,
            "slowest_q": slowest_q,
            "details": details
        })


# ========== 主窗口 ==========
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.engine = SimpleSearchEngine()
        self.all_results = []
        self.current_page = 1
        self.page_size = 20
        self.total_pages = 1
        
        self.init_ui()
        self.setup_styles()
    
    def init_ui(self):
        self.setWindowTitle("搜索引擎 - Qt 界面")
        self.setGeometry(100, 100, 1200, 800)
        
        # 中央部件
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(8, 8, 8, 8)
        
        # 主分割器：左侧词频 + 右侧内容
        main_splitter = QSplitter(Qt.Horizontal)
        main_layout.addWidget(main_splitter)
        
        # ===== 左侧：词频面板 =====
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 5, 0)
        
        freq_group = QGroupBox("📊 词频统计")
        freq_layout = QVBoxLayout(freq_group)
        
        self.freq_info = QLabel("点击文件查看词频")
        self.freq_info.setStyleSheet("color: #888; font-size: 12px;")
        freq_layout.addWidget(self.freq_info)
        
        self.freq_text = QTextEdit()
        self.freq_text.setReadOnly(True)
        self.freq_text.setFont(QFont("Consolas", 9))
        self.freq_text.setMinimumWidth(320)
        freq_layout.addWidget(self.freq_text)
        
        self.freq_stat = QLabel("")
        self.freq_stat.setStyleSheet("color: #888; font-size: 11px;")
        freq_layout.addWidget(self.freq_stat)
        
        left_layout.addWidget(freq_group)
        left_widget.setMinimumWidth(320)
        main_splitter.addWidget(left_widget)
        
        # ===== 右侧主体 =====
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setContentsMargins(5, 0, 0, 0)
        
        # === 文档管理 ===
        doc_group = QGroupBox("📁 文档管理")
        doc_layout = QVBoxLayout(doc_group)
        
        btn_layout = QHBoxLayout()
        self.btn_folder = QPushButton("📂 选择文件夹")
        self.btn_file = QPushButton("📄 添加文件")
        self.btn_test = QPushButton("🧪 text_data")
        self.btn_global = QPushButton("🌐 全局词频")
        self.btn_del = QPushButton("❌ 删除选中")
        self.btn_clear = QPushButton("🗑 清空所有")
        
        for btn in [self.btn_folder, self.btn_file, self.btn_test, 
                    self.btn_global, self.btn_del, self.btn_clear]:
            btn_layout.addWidget(btn)
        
        self.doc_count_label = QLabel("文档数: 0")
        self.doc_count_label.setStyleSheet("color: #555; font-weight: bold;")
        btn_layout.addWidget(self.doc_count_label)
        btn_layout.addStretch()
        
        doc_layout.addLayout(btn_layout)
        
        # 文件列表
        self.doc_list = QListWidget()
        self.doc_list.setMinimumHeight(100)
        doc_layout.addWidget(self.doc_list)
        
        right_layout.addWidget(doc_group)
        
        # === 搜索 ===
        search_group = QGroupBox("🔍 搜索")
        search_layout = QHBoxLayout(search_group)
        
        search_layout.addWidget(QLabel("查询词:"))
        self.query_input = QLineEdit()
        self.query_input.setPlaceholderText("输入搜索关键词...")
        self.query_input.returnPressed.connect(self.do_search)
        search_layout.addWidget(self.query_input)
        
        search_layout.addWidget(QLabel(" TOP-K:"))
        self.topk_spin = QSpinBox()
        self.topk_spin.setRange(1, 100)
        self.topk_spin.setValue(10)
        search_layout.addWidget(self.topk_spin)
        
        self.btn_search = QPushButton("🔍 搜索")
        search_layout.addWidget(self.btn_search)
        
        self.btn_benchmark = QPushButton("⚡ Benchmark")
        self.btn_benchmark.setStyleSheet("""
            QPushButton {
                background-color: #FF6600; color: white; font-weight: bold;
                padding: 6px 16px; border-radius: 4px;
            }
            QPushButton:hover { background-color: #FF8533; }
            QPushButton:disabled { background-color: #ccc; }
        """)
        search_layout.addWidget(self.btn_benchmark)
        
        right_layout.addWidget(search_group)
        
        # === 搜索结果 ===
        result_group = QGroupBox("📄 搜索结果 (双击路径打开文件)")
        result_layout = QVBoxLayout(result_group)
        
        self.result_text = QTextEdit()
        self.result_text.setReadOnly(True)
        self.result_text.setFont(QFont("Microsoft YaHei", 10))
        self.result_text.setMouseTracking(True)
        result_layout.addWidget(self.result_text)
        
        # 分页控件
        pager_layout = QHBoxLayout()
        self.page_label = QLabel("")
        self.page_label.setStyleSheet("color: #666; font-size: 12px;")
        pager_layout.addWidget(self.page_label)
        
        self.btn_first = QPushButton("⏮ 首页")
        self.btn_prev = QPushButton("◀ 上页")
        self.btn_next = QPushButton("下页 ▶")
        self.btn_last = QPushButton("末页 ⏭")
        
        for btn in [self.btn_first, self.btn_prev, self.btn_next, self.btn_last]:
            btn.setFixedWidth(80)
            pager_layout.addWidget(btn)
        
        pager_layout.addStretch()
        pager_layout.addWidget(QLabel("跳转至:"))
        self.jump_input = QLineEdit()
        self.jump_input.setFixedWidth(50)
        self.jump_input.returnPressed.connect(self.jump_to_page)
        pager_layout.addWidget(self.jump_input)
        self.btn_jump = QPushButton("GO")
        self.btn_jump.setFixedWidth(40)
        pager_layout.addWidget(self.btn_jump)
        
        result_layout.addLayout(pager_layout)
        right_layout.addWidget(result_group)
        
        main_splitter.addWidget(right_widget)
        main_splitter.setSizes([350, 850])
        
        # 状态栏
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_label = QLabel("就绪 | 点击文件查看词频 | 双击路径打开文件")
        self.status_bar.addWidget(self.status_label)
        
        # 进度条
        self.progress_bar = QProgressBar()
        self.progress_bar.setMaximumWidth(200)
        self.progress_bar.setVisible(False)
        self.status_bar.addPermanentWidget(self.progress_bar)
        
        # ===== 信号连接 =====
        self.btn_folder.clicked.connect(self.load_folder)
        self.btn_file.clicked.connect(self.add_file)
        self.btn_test.clicked.connect(self.load_test)
        self.btn_global.clicked.connect(self.show_global_freq)
        self.btn_del.clicked.connect(self.delete_selected)
        self.btn_clear.clicked.connect(self.clear_all)
        self.btn_search.clicked.connect(self.do_search)
        self.btn_benchmark.clicked.connect(self.run_benchmark)
        self.btn_first.clicked.connect(self.first_page)
        self.btn_prev.clicked.connect(self.prev_page)
        self.btn_next.clicked.connect(self.next_page)
        self.btn_last.clicked.connect(self.last_page)
        self.btn_jump.clicked.connect(self.jump_to_page)
        self.doc_list.itemClicked.connect(self.on_file_click)
        
        # 双击打开文件 - 用 QTimer 检测双击
        self.result_text.mouseDoubleClickEvent = self.on_result_double_click
    
    def setup_styles(self):
        self.setStyleSheet("""
            QGroupBox {
                font-weight: bold; border: 1px solid #ccc;
                border-radius: 6px; margin-top: 10px; padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin; left: 10px; padding: 0 5px;
            }
            QPushButton {
                padding: 5px 12px; border: 1px solid #aaa;
                border-radius: 4px; background-color: #f8f8f8;
            }
            QPushButton:hover { background-color: #e8e8e8; }
            QPushButton:disabled { color: #aaa; background-color: #f0f0f0; }
            QLineEdit { padding: 4px 8px; border: 1px solid #ccc; border-radius: 4px; }
            QListWidget { border: 1px solid #ddd; border-radius: 4px; }
            QTextEdit { border: 1px solid #ddd; border-radius: 4px; }
            QSpinBox { padding: 3px; border: 1px solid #ccc; border-radius: 3px; }
        """)
    
    # ---------- 文档管理 ----------
    def update_doc_count(self):
        self.doc_count_label.setText(f"文档数: {self.engine.total_docs}")
    
    def refresh_file_list(self):
        self.doc_list.clear()
        if not self.engine.docs:
            return
        
        # 文件夹汇总
        folders = collections.Counter()
        for doc in self.engine.docs:
            folder = os.path.dirname(doc["file"])
            folders[folder] += 1
        
        for folder, count in folders.items():
            item = QListWidgetItem(f"📁 {os.path.basename(folder)} ({count} 个文档)")
            item.setData(Qt.UserRole, {"type": "folder"})
            item.setForeground(QColor("#555"))
            self.doc_list.addItem(item)
        
        # 每个文档
        for doc in self.engine.docs:
            basename = os.path.basename(doc["file"])
            wc = len(doc["words"])
            uc = len(doc["word_freq"])
            item = QListWidgetItem(f"    📄 {basename}  (词:{wc} 唯一:{uc})")
            item.setData(Qt.UserRole, {"type": "doc", "docId": doc["id"], "file": doc["file"]})
            self.doc_list.addItem(item)
    
    def load_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "选择文档文件夹")
        if not folder:
            return
        self._start_load(folder=folder)
    
    def add_file(self):
        files, _ = QFileDialog.getOpenFileNames(self, "选择文档文件", "", 
            "文本文件 (*.txt *.md *.csv *.py *.cpp *.h);;所有文件 (*)")
        if not files:
            return
        self._start_load(files=files)
    
    def load_test(self):
        td = os.path.join(os.path.dirname(__file__), "text_data")
        if os.path.isdir(td):
            self._start_load(folder=td)
        else:
            QMessageBox.information(self, "提示", "text_data 目录不存在")
    
    def _start_load(self, folder=None, files=None):
        self.status_label.setText("⏳ 加载中...")
        self.set_buttons_enabled(False)
        
        self.load_thread = LoadThread(self.engine, folder, files)
        self.load_thread.finished.connect(self._on_load_finished)
        self.load_thread.status.connect(self.status_label.setText)
        self.load_thread.start()
    
    def _on_load_finished(self, count):
        self.refresh_file_list()
        self.update_doc_count()
        self.set_buttons_enabled(True)
        self.status_label.setText(f"✅ 加载完成！当前共 {self.engine.total_docs} 个文档")
        self.show_global_freq()
    
    def set_buttons_enabled(self, enabled):
        for btn in [self.btn_folder, self.btn_file, self.btn_test, 
                    self.btn_global, self.btn_del, self.btn_clear,
                    self.btn_search, self.btn_benchmark]:
            btn.setEnabled(enabled)
    
    def delete_selected(self):
        items = self.doc_list.selectedItems()
        if not items:
            QMessageBox.information(self, "提示", "请先选择要删除的文档")
            return
        
        item = items[0]
        data = item.data(Qt.UserRole)
        if not data or data["type"] != "doc":
            QMessageBox.information(self, "提示", "请选择一个具体的文档文件")
            return
        
        basename = os.path.basename(data["file"])
        reply = QMessageBox.question(self, "确认删除", f"确定删除文档吗？\n{basename}",
                                     QMessageBox.Yes | QMessageBox.No)
        if reply != QMessageBox.Yes:
            return
        
        self.engine.remove_document(data["docId"])
        self.refresh_file_list()
        self.update_doc_count()
        self.all_results = []
        self.current_page = 1
        self.total_pages = 1
        self.result_text.clear()
        self.page_label.setText("")
        self.show_global_freq()
        self.status_label.setText(f"✅ 已删除: {basename}，剩余 {self.engine.total_docs} 个文档")
    
    def clear_all(self):
        if self.engine.total_docs == 0:
            return
        reply = QMessageBox.question(self, "确认清空", 
            f"确定清空所有 {self.engine.total_docs} 个文档吗？",
            QMessageBox.Yes | QMessageBox.No)
        if reply != QMessageBox.Yes:
            return
        
        self.engine.clear()
        self.doc_list.clear()
        self.all_results = []
        self.current_page = 1
        self.total_pages = 1
        self.result_text.clear()
        self.page_label.setText("")
        self.freq_text.clear()
        self.freq_info.setText("点击文件查看词频")
        self.freq_stat.setText("")
        self.update_doc_count()
        self.status_label.setText("已清空所有数据")
    
    # ---------- 词频 ----------
    def on_file_click(self, item):
        data = item.data(Qt.UserRole)
        if data and data["type"] == "doc":
            self.show_doc_freq(data["docId"])
        else:
            self.show_global_freq()
    
    def show_doc_freq(self, doc_id):
        if doc_id < 0 or doc_id >= len(self.engine.docs):
            return
        
        doc = self.engine.docs[doc_id]
        freq_list = self.engine.get_doc_word_freq(doc_id)
        
        self.freq_text.clear()
        cursor = self.freq_text.textCursor()
        
        basename = os.path.basename(doc["file"])
        self._append_colored(cursor, f"📄 {basename}\n", QColor("#000"), bold=True, size=11)
        self._append_colored(cursor, f"总词数: {len(doc['words'])}  |  唯一词: {len(doc['word_freq'])}\n\n", QColor("#888"), size=10)
        
        self._append_colored(cursor, f"{'排名':<5} {'词':<16} {'词频':<8} {'占比%':<8}\n", QColor("#003366"), bold=True)
        self._append_colored(cursor, "─" * 42 + "\n", QColor("#003366"))
        
        total = len(doc['words'])
        for rank, (word, count) in enumerate(freq_list[:100], 1):
            ratio = count / total * 100 if total > 0 else 0
            line = f"{rank:<5} {word:<16} {count:<8} {ratio:.2f}%\n"
            color = QColor("#CC0000") if rank <= 5 else QColor("#333")
            self._append_colored(cursor, line, color, bold=(rank<=5))
        
        if len(freq_list) > 100:
            self._append_colored(cursor, f"\n... 还有 {len(freq_list) - 100} 个词未显示", QColor("#888"))
        
        self.freq_info.setText(f"📊 {basename} - 词频统计")
        self.freq_stat.setText(f"共 {len(freq_list)} 个唯一词 | 总词 {total}")
    
    def show_global_freq(self, highlight_words=None):
        global_freq = self.engine.get_global_word_freq(500)
        if not global_freq:
            self.freq_text.clear()
            self.freq_info.setText("暂无数据")
            self.freq_stat.setText("")
            return
        
        self.freq_text.clear()
        cursor = self.freq_text.textCursor()
        
        total_words = sum(len(doc["words"]) for doc in self.engine.docs)
        total_unique = len(set(w for doc in self.engine.docs for w in doc["words"]))
        
        self._append_colored(cursor, "🌐 全局词频统计\n", QColor("#000"), bold=True, size=11)
        self._append_colored(cursor, f"总词数: {total_words}  |  全局唯一词: {total_unique}\n\n", QColor("#888"), size=10)
        
        self._append_colored(cursor, f"{'排名':<5} {'词':<16} {'频次':<8} {'占比%':<8}\n", QColor("#003366"), bold=True)
        self._append_colored(cursor, "─" * 42 + "\n", QColor("#003366"))
        
        highlight_set = set(highlight_words) if highlight_words else set()
        first_match = None
        
        for i, (word, count) in enumerate(global_freq):
            ratio = count / total_words * 100 if total_words > 0 else 0
            line = f"{i+1:<5} {word:<16} {count:<8} {ratio:.2f}%\n"
            
            if word in highlight_set:
                if first_match is None:
                    first_match = i
                self._append_colored(cursor, line, QColor("#006600"), bold=True, bg=QColor("#E8FFE8"))
            elif i < 5:
                self._append_colored(cursor, line, QColor("#CC0000"), bold=True)
            else:
                self._append_colored(cursor, line, QColor("#333"))
        
        # 跳转到第一个匹配词
        if first_match is not None:
            block = cursor.document().findBlockByNumber(first_match + 4)
            if block:
                cursor.setPosition(block.position())
                self.freq_text.setTextCursor(cursor)
                self.freq_text.ensureCursorVisible()
            match_str = ", ".join(highlight_words) if highlight_words else ""
            self.freq_info.setText(f"🌐 全局词频 → 已跳至「{match_str}」位置")
        else:
            self.freq_info.setText("🌐 全局词频统计")
        
        if highlight_words and not highlight_set.intersection(w for w, _ in global_freq):
            self.freq_info.setText(f"🌐 全局词频 - 查询词不在前500词中")
    
    def _append_colored(self, cursor, text, color, bold=False, size=9, bg=None):
        fmt = QTextCharFormat()
        fmt.setForeground(color)
        if bold:
            fmt.setFontWeight(QFont.Bold)
        if bg:
            fmt.setBackground(bg)
        fmt.setFontPointSize(size)
        cursor.insertText(text, fmt)
    
    # ---------- 搜索 ----------
    def do_search(self):
        q = self.query_input.text().strip()
        if not q:
            QMessageBox.information(self, "提示", "请输入查询词")
            return
        if self.engine.total_docs == 0:
            QMessageBox.information(self, "提示", "请先加载文档")
            return
        
        topK = self.topk_spin.value()
        self.all_results = self.engine.search(q, topK)
        
        self.current_page = 1
        self.total_pages = max(1, (len(self.all_results) + self.page_size - 1) // self.page_size) if self.all_results else 1
        self.show_page()
        
        query_words = tokenize(q)
        self.show_global_freq(highlight_words=query_words)
        
        self.status_label.setText(f"🔍 搜索「{q}」完成，共 {len(self.all_results)} 条结果")
    
    def show_page(self):
        self.result_text.clear()
        cursor = self.result_text.textCursor()
        
        if not self.all_results:
            self._append_rt(cursor, "  ✨ 无匹配结果，请尝试其他关键词", QColor("#888"), size=10)
        else:
            s = (self.current_page - 1) * self.page_size
            e = min(s + self.page_size, len(self.all_results))
            total = len(self.all_results)
            
            # 页头
            info = f"  共 {total} 条结果  |  显示第 {s+1}-{e} 条  |  第 {self.current_page}/{self.total_pages} 页\n"
            self._append_rt(cursor, info, QColor("#999"), size=9)
            self._append_rt(cursor, "─" * 60 + "\n", QColor("#ddd"), size=8)
            
            for i, r in enumerate(self.all_results[s:e], start=s+1):
                fp = r["file"]
                sc = r["score"]
                rk = r["rank"]
                sn = r["snippet"]
                
                # 排名
                rank_color = QColor("#CC0000") if rk <= 3 else QColor("#333")
                rank_size = 12 if rk <= 3 else 11
                self._append_rt(cursor, f"  #{rk}  ", rank_color, bold=(rk<=3), size=rank_size)
                
                # 文件路径（蓝色，用下划线标记为可点击）
                self._append_rt(cursor, fp, QColor("#0066CC"), bold=True, size=10, underline=True)
                self._append_rt(cursor, f"  得分: {sc:.4f}\n", QColor("#888"), size=9)
                
                # 摘要
                self._append_rt(cursor, "    摘要: ", QColor("#999"), size=9)
                self._append_rt(cursor, f"{sn}\n", QColor("#555"), size=10)
                
                # 分隔
                self._append_rt(cursor, "  " + "·" * 56 + "\n", QColor("#ddd"), size=8)
        
        # 更新分页按钮
        self.page_label.setText(f"第 {self.current_page}/{self.total_pages} 页")
        self._update_pager_buttons()
    
    def _append_rt(self, cursor, text, color, bold=False, size=10, underline=False):
        fmt = QTextCharFormat()
        fmt.setForeground(color)
        if bold:
            fmt.setFontWeight(QFont.Bold)
        if underline:
            fmt.setUnderlineStyle(QTextCharFormat.SingleUnderline)
        fmt.setFontPointSize(size)
        cursor.insertText(text, fmt)
    
    def _update_pager_buttons(self):
        if self.total_pages <= 1:
            for btn in [self.btn_first, self.btn_prev, self.btn_next, self.btn_last]:
                btn.setEnabled(False)
        else:
            self.btn_first.setEnabled(self.current_page > 1)
            self.btn_prev.setEnabled(self.current_page > 1)
            self.btn_next.setEnabled(self.current_page < self.total_pages)
            self.btn_last.setEnabled(self.current_page < self.total_pages)
    
    def first_page(self):
        if self.current_page != 1:
            self.current_page = 1
            self.show_page()
    
    def last_page(self):
        if self.current_page != self.total_pages:
            self.current_page = self.total_pages
            self.show_page()
    
    def next_page(self):
        if self.current_page < self.total_pages:
            self.current_page += 1
            self.show_page()
    
    def prev_page(self):
        if self.current_page > 1:
            self.current_page -= 1
            self.show_page()
    
    def jump_to_page(self):
        try:
            p = int(self.jump_input.text().strip())
            if 1 <= p <= self.total_pages:
                self.current_page = p
                self.show_page()
            else:
                QMessageBox.information(self, "提示", f"页码超出范围 (1-{self.total_pages})")
        except ValueError:
            QMessageBox.information(self, "提示", "请输入有效数字")
    
    # ---------- 双击打开文件 ----------
    def on_result_double_click(self, event):
        cursor = self.result_text.cursorForPosition(event.pos())
        # 获取当前行的文本，提取文件路径
        block = cursor.block()
        text = block.text()
        
        # 查找文本中的文件路径（以 # 开头的行中的第二个字段）
        if text and "  " in text:
            parts = text.strip().split()
            for p in parts:
                if os.path.exists(p):
                    self._open_file(p)
                    return
                # 尝试 basename 匹配
                for doc in self.engine.docs:
                    if os.path.basename(doc["file"]) == os.path.basename(p):
                        self._open_file(doc["file"])
                        return
        
        # 备用：遍历所有结果匹配 basename
        for r in self.all_results:
            if os.path.basename(r["file"]) in text:
                self._open_file(r["file"])
                return
    
    def _open_file(self, filepath):
        if os.path.exists(filepath):
            try:
                if sys.platform == 'win32':
                    os.startfile(filepath)
                else:
                    subprocess.run(['open' if sys.platform=='darwin' else 'xdg-open', filepath])
            except Exception as e:
                QMessageBox.critical(self, "打开失败", str(e))
        else:
            QMessageBox.warning(self, "文件不存在", f"找不到:\n{filepath}")
    
    # ---------- Benchmark ----------
    def run_benchmark(self):
        if self.engine.total_docs == 0:
            QMessageBox.information(self, "提示", "请先加载文档再运行 Benchmark")
            return
        
        # 选择关键词文件
        filepath, _ = QFileDialog.getOpenFileName(self, "选择关键词文件", "",
            "文本文件 (*.txt);;所有文件 (*)")
        if not filepath:
            return
        
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                queries = [line.strip() for line in f if line.strip()]
        except Exception as e:
            QMessageBox.critical(self, "读取失败", str(e))
            return
        
        if not queries:
            QMessageBox.information(self, "提示", "关键词文件为空")
            return
        
        # 确认
        reply = QMessageBox.question(self, "确认 Benchmark",
            f"将从「{os.path.basename(filepath)}」读取 {len(queries)} 个关键词进行批量测试\n\n"
            f"文档数: {self.engine.total_docs}\n"
            f"Top-K: {self.topk_spin.value()}\n\n是否开始？",
            QMessageBox.Yes | QMessageBox.No)
        if reply != QMessageBox.Yes:
            return
        
        # 执行 benchmark
        self.set_buttons_enabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setMaximum(len(queries))
        self.progress_bar.setValue(0)
        self.status_label.setText("⏳ Benchmark 测试中...")
        
        self.bench_thread = BenchmarkThread(self.engine, queries, self.topk_spin.value())
        self.bench_thread.progress.connect(lambda cur, total: self.progress_bar.setValue(cur))
        self.bench_thread.result.connect(self._on_benchmark_result)
        self.bench_thread.finished.connect(self._on_benchmark_finished)
        self.bench_thread.start()
    
    def _on_benchmark_result(self, report):
        # 显示在搜索结果区域
        self.result_text.clear()
        cursor = self.result_text.textCursor()
        self._append_rt(cursor, report, QColor("#000"), size=10)
        
        # 同时显示在状态栏
        self.status_label.setText("✅ Benchmark 测试完成！")
    
    def _on_benchmark_finished(self, stats):
        self.progress_bar.setVisible(False)
        self.set_buttons_enabled(True)


# ========== 启动 ==========
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
