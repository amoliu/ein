#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <ros/ros.h>

#include "ein.h"
#include "window_QT.h"
#include "stackmodel.h"
#include "windowmanager.h"

using namespace std;
namespace Ui {
class MainWindow;
}

void updateLastKey(QKeyEvent * evnt);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, shared_ptr<MachineState> ms = 0);
    ~MainWindow();
    Q_INVOKABLE void updateImage(const Mat image) ;
    void setMouseCallBack(EinMouseCallback m, void* param);
    void keyPressEvent(QKeyEvent *evnt);
    void addWindow(EinWindow * window);
public slots:
    void rosSpin();

private:
    Ui::MainWindow *ui;
    StackModel * stackModel;

    shared_ptr<MachineState> ms;
    DefaultEinViewPort myView;
    WindowManager windowManager;
};

#endif // MAINWINDOW_H