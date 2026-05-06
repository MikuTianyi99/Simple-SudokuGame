#include <QApplication>
#include "SudokuGame.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    SudokuGame game;
    game.show();
    return app.exec();
}