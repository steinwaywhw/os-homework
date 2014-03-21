#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>

extern "C" {
	#include "client.h"
	#include "zlog.h"
}


namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    void draw (void *data, int len);
    //void wrong ();

    ~MainWindow();

protected:
	virtual void paintEvent(QPaintEvent *event);
    
private slots:
    void on_btPlay_clicked();
    void on_btStop_clicked();
    void on_btSeek_clicked();

private:
    Ui::MainWindow *ui;
    client_t client;
    zlog_category_t *category;
    QPixmap pixmap;
    bool m_wrong;

};

#endif // MAINWINDOW_H
