#include <QApplication>
#include <QMainWindow>
#include <QPushButton>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QMainWindow w;
    w.setWindowTitle("Test Window - 纯Qt测试");
    w.resize(400, 300);
    w.show();
    return app.exec();
}
