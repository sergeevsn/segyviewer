#include "MainWindow.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    // Проверяем аргументы командной строки
    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        QFileInfo fileInfo(filePath);
        
        // Проверяем, что файл существует и имеет правильное расширение
        if (fileInfo.exists() && fileInfo.isFile()) {
            QString suffix = fileInfo.suffix().toLower();
            if (suffix == "sgy" || suffix == "segy") {
                // Открываем файл
                w.openFile(filePath);
            } else {
                QMessageBox::warning(&w, "Invalid File Type", 
                    "Please provide a SEG-Y file with .sgy or .segy extension.");
            }
        } else {
            QMessageBox::warning(&w, "File Not Found", 
                QString("The file '%1' does not exist or is not accessible.").arg(filePath));
        }
    }

    return app.exec();
}

