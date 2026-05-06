#ifndef SUDOKUGAME_H
#define SUDOKUGAME_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include <QStyleFactory>
#include <QTimer>


class SudokuGame : public QMainWindow
{
    Q_OBJECT

public:
    SudokuGame(QWidget *parent = nullptr);
    ~SudokuGame();

private slots:
    void onCellSelected(int row, int col);
    void onNumberButtonClicked(int number);
    void onDeleteButtonClicked();
    void onNewGame();
    void onResetGame();
    void onClearAll();
    void onCheckGame();
    void onTimerTick();

private:
    // UI组件
    QTableWidget* m_table;
    QPushButton* m_numberButtons[10];
    QPushButton* m_deleteButton;
    QPushButton* m_newGameButton;
    QPushButton* m_resetButton;
    QPushButton* m_clearButton;
    QPushButton* m_checkButton;
    QLabel* m_statusLabel;
    QTimer* m_timer;
    QLabel* m_timerLabel;
    int m_elapsedSeconds;

    // 游戏数据
    int m_board[9][9];      // 当前盘面 (0表示空格)
    bool m_fixed[9][9];     // 是否为预设题目格 (不可编辑)
    int m_originalBoard[9][9]; // 当前题目的原始预设盘面 (用于重置)

    int m_currentRow;       // 当前选中的行
    int m_currentCol;       // 当前选中的列

    // 预设题目库 (三个不同难度的题目, 0表示空格)
    int m_presetBoards[9][9];

    // 初始化UI
    void setupUI();
    // 初始化游戏盘面 (从预设题目中选择一个，随机或默认第一个)
    void initGame(int presetIndex = -1);
    // 生成题目
    void generate_test();
    // 刷新整个表格显示 (根据m_board和m_fixed更新)
    void refreshTableDisplay();
    // 高亮冲突格子 (根据当前盘面检测行列和宫格冲突)
    void highlightConflicts();
    // 检查指定数字在当前位置是否与同行/列/宫冲突 (用于辅助高亮)
    bool isConflict(int row, int col, int value) const;
    // 计算所有冲突格子并返回冲突标记数组
    void findAllConflicts(bool conflict[9][9]) const;
    // 重置当前盘面为原始题目状态 (清除所有用户填入的数字)
    void resetToOriginal();
    // 检查是否胜利 (所有格子已填且无冲突)
    bool isVictory() const;
    // 随机选择一个预设题目索引
    int getRandomPresetIndex() const;
    // 设置单元格样式 (背景色、文字颜色等)
    void setCellStyle(int row, int col, bool isFixed, bool hasConflict);
    // 更新状态栏信息
    void updateStatusMessage(const QString& msg);
    // 计时器
    void startGameTimer();
    void stopGameTimer();
    void updateTimerDisplay();
};

#endif // SUDOKUGAME_H