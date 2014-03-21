#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "client.h"
#include "zlog.h"
#include <QMessageBox>
#include <QInputDialog>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); 
    this->m_wrong = false;

    zlog_init("../zlog.conf");
	category = zlog_get_category("default");

    int priority = QInputDialog::getInteger(this, "Priority", "Please input a priority.");

    int status = init_client (&client, priority);  
    if (status < 0) {
        QMessageBox msg (QMessageBox::Warning, "Warning", "Server is not running! About to exit!");
        msg.exec();
        zlog_fini ();
        QApplication::quit();
    }

    this->setAttribute (Qt::WA_PaintOutsidePaintEvent, true);

    client.main_window = this;
    client.run (&client); 
}

MainWindow::~MainWindow()
{
    delete ui;
    destroy_client (&client);
    zlog_fini ();
}

void MainWindow::on_btPlay_clicked()
{
    QString text = this->ui->tPlay->text();
    if (text.isEmpty() || text.isNull()) {
        QMessageBox msg (QMessageBox::Warning, "Warning", "Please type a movie name!");
        msg.exec();
        return;
    }


    char *data = text.toAscii().data();
    client.start (&client, data, true);
}


void MainWindow::on_btStop_clicked()
{
    QString text = this->ui->tPlay->text();

    if (text.isEmpty() || text.isNull()) {
        QMessageBox msg (QMessageBox::Warning, "Warning", "Please type a movie name!");
        msg.exec();
        return;
    }


    char *data = text.toAscii().data();
    client.stop (&client, data);

}

void MainWindow::on_btSeek_clicked()
{
    QString text = this->ui->tPlay->text();

    if (text.isEmpty() || text.isNull()) {
        QMessageBox msg (QMessageBox::Warning, "Warning", "Please type a movie name!");
        msg.exec();
        return;
    }


    char *data = text.toAscii().data();

    QString frame = this->ui->tSeek->text();
    int frame_number = frame.toInt();

    client.seek (&client, data, frame_number);
}

void MainWindow::draw (void *data, int len) {
    bool status = this->pixmap.loadFromData((const uchar *)data,len,"PPM");
    assert (status == true);

    update ();
}

void MainWindow::paintEvent (QPaintEvent *event) {
    //if ((event->rect()).isNull ())
    //    return;
    // if (this->m_wrong == true) {
    //     QMessageBox msg (QMessageBox::Warning, "Warning", "Movie not found!");
    //     msg.exec();
    //     this->m_wrong = false;
    //     return;
    // }

	QPainter painter (this);
	QRect rect = this->ui->frMovie->contentsRect();
    assert (rect.isNull () == false);
    if (this->pixmap.isNull ())
        return;
    painter.drawPixmap (rect, this->pixmap);
}