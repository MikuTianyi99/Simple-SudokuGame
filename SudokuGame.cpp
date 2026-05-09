#include "SudokuGame.h"
#include "SudokuDelegate.h"
#include <QDebug>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <QMediaPlayer>
#include <QAudioOutput>
SudokuGame::SudokuGame(QWidget *parent)
    : QMainWindow(parent)
    , m_currentRow(-1)
    , m_currentCol(-1)
    , m_elapsedSeconds(0)
{
    setupUI();
    initGame(getRandomPresetIndex());
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SudokuGame::onTimerTick);
    startGameTimer();
    m_audioOutput = new QAudioOutput(this);
    m_music?m_audioOutput->setVolume(0.3):m_audioOutput->setVolume(0.0);
    m_backgroundMusic = new QMediaPlayer(this);
    m_backgroundMusic->setAudioOutput(m_audioOutput);
    m_backgroundMusic->setSource(QUrl("qrc:/icons/background.mp3"));
    m_backgroundMusic->setLoops(QMediaPlayer::Infinite);
    m_writeSoundEffect = new QSoundEffect(this);
    m_writeSoundEffect->setSource(QUrl("qrc:/icons/write.wav"));
    m_writeSoundEffect->setVolume(0.5f);
    m_delSoundEffect = new QSoundEffect(this);
    m_delSoundEffect->setSource(QUrl("qrc:/icons/del.wav"));
    m_delSoundEffect->setVolume(0.5f);
    m_backgroundMusic->play();
}

SudokuGame::~SudokuGame()
{
}

void SudokuGame::setupUI()
{
    // 中央部件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);


    // 主布局：水平布局
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 左侧：数独表格
    m_table = new QTableWidget(9, 9, this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setFocusPolicy(Qt::StrongFocus);
    m_table->setItemDelegate(new SudokuDelegate(this));
    m_table->setShowGrid(false);

    // 固定单元格大小，保持正方形
    int cellSize = 70;
    for (int i = 0; i < 9; ++i) {
        m_table->setRowHeight(i, cellSize);
        m_table->setColumnWidth(i, cellSize);
    }
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget { gridline-color: #aaa; }"
        "QTableWidget::item { border: 1px solid #aaa; padding: 0px; }"
        );

    connect(m_table, &QTableWidget::currentCellChanged,
            this, &SudokuGame::onCellSelected);

    mainLayout->addWidget(m_table, 1);  // 棋盘占用额外空间

    // 右侧：控制面板
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(240);  // 固定宽度，避免挤压棋盘
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(12);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // 数字键盘 (3x3 网格)
    QWidget* numPadWidget = new QWidget(this);
    QGridLayout* numPadLayout = new QGridLayout(numPadWidget);
    numPadLayout->setSpacing(8);

    QString buttonStyle = "QPushButton { font-size: 25px; font-weight: bold; "
                          "min-width: 60px; min-height: 60px; "
                          "background-color: #f0f0f0; border-radius: 8px; }"
                          "QPushButton:hover { background-color: #ddd; }"
                          "QPushButton:pressed { background-color: #bbb; }";

    // 创建 1~9 按钮，按 3x3 排列
    for (int i = 1; i <= 9; ++i) {
        m_numberButtons[i] = new QPushButton(QString::number(i), this);
        m_numberButtons[i]->setStyleSheet(buttonStyle);
        connect(m_numberButtons[i], &QPushButton::clicked, [this, i]() {
            onNumberButtonClicked(i);
        });
        int row = (i - 1) / 3;
        int col = (i - 1) % 3;
        numPadLayout->addWidget(m_numberButtons[i], row, col);
    }
    rightLayout->addWidget(numPadWidget);

    // 删除按钮 (单独一行)
    m_deleteButton = new QPushButton("⌫ 删除", this);
    m_deleteButton->setToolTip("清除当前选中的格子");
    m_deleteButton->setStyleSheet("QPushButton { font-size: 20px; min-height: 50px; "
                                  "background-color: #e0e0e0; border-radius: 6px; }"
                                  "QPushButton:hover { background-color: #ccc; }");
    connect(m_deleteButton, &QPushButton::clicked, this, &SudokuGame::onDeleteButtonClicked);
    rightLayout->addWidget(m_deleteButton);

    // 功能按钮 (垂直排列)
    QWidget* funcWidget = new QWidget(this);
    QVBoxLayout* funcLayout = new QVBoxLayout(funcWidget);
    funcLayout->setSpacing(10);

    m_newGameButton = new QPushButton("新游戏", this);
    m_resetButton   = new QPushButton("重置", this);
    m_clearButton   = new QPushButton("清除所有", this);
    m_checkButton   = new QPushButton("检查", this);

    QString funcButtonStyle = "QPushButton { font-size: 15px; min-height: 45px; "
                              "background-color: #5c9ce0; color: white; border-radius: 6px; }"
                              "QPushButton:hover { background-color: #4a8bc8; }"
                              "QPushButton:pressed { background-color: #3a7bb0; }";

    m_newGameButton->setStyleSheet(funcButtonStyle);
    m_resetButton->setStyleSheet(funcButtonStyle);
    m_clearButton->setStyleSheet(funcButtonStyle);
    m_checkButton->setStyleSheet(funcButtonStyle);

    funcLayout->addWidget(m_newGameButton);
    funcLayout->addWidget(m_resetButton);
    funcLayout->addWidget(m_clearButton);
    funcLayout->addWidget(m_checkButton);
    funcLayout->addStretch();  // 底部弹簧，使按钮靠上

    rightLayout->addWidget(funcWidget);
    rightLayout->addStretch();  // 让整个面板内容从顶部开始排列

    mainLayout->addWidget(rightPanel);

    // 计时器
    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #2c3e50; padding: 5px; background: #ecf0f1; border-radius: 6px; }");
    m_timerLabel->setFixedHeight(50);
    rightLayout->addWidget(m_timerLabel);

    // 状态栏（底部全局）
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);          // 水平+垂直居中
    m_statusLabel->setStyleSheet("QLabel { font-size: 14px; color: #2c3e50; padding: 8px; background: #ecf0f1; border-radius: 4px; }");
    m_statusLabel->setWordWrap(true);                      // 允许换行
    m_statusLabel->setFixedHeight(60);                     // 固定高度
    rightLayout->addWidget(m_statusLabel);

    // 连接功能按钮信号
    connect(m_newGameButton, &QPushButton::clicked, this, &SudokuGame::onNewGame);
    connect(m_resetButton, &QPushButton::clicked, this, &SudokuGame::onResetGame);
    connect(m_clearButton, &QPushButton::clicked, this, &SudokuGame::onClearAll);
    connect(m_checkButton, &QPushButton::clicked, this, &SudokuGame::onCheckGame);

    // 设置窗口属性
    setWindowTitle("数独");
    setMinimumSize(907, 652);
    setMaximumSize(907,652);
    resize(907, 652);
    setStyleSheet("QMainWindow { background-color: #f5f5f5; }");

    updateStatusMessage("点击新游戏以开始");
}

int SudokuGame::getRandomPresetIndex() const
{
    static bool seedInitialized = false;
    if (!seedInitialized) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seedInitialized = true;
    }
    return std::rand() % 3;
}

void SudokuGame::initGame(int presetIndex)
{
    generate_test();

    // 复制预设题目到原始盘面 和 当前盘面
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            int val = m_presetBoards[i][j];
            m_originalBoard[i][j] = val;
            m_board[i][j] = val;
            m_fixed[i][j] = (val != 0); // 非0数字为预设不可编辑
        }
    }

    // 刷新显示
    refreshTableDisplay();
    updateStatusMessage("新游戏已开始！");
}

void SudokuGame::refreshTableDisplay()
{
    // 先清除所有单元格项
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            QTableWidgetItem* item = m_table->item(i, j);
            if (!item) {
                item = new QTableWidgetItem();
                m_table->setItem(i, j, item);
            }
            // 设置显示文本
            if (m_board[i][j] != 0) {
                item->setText(QString::number(m_board[i][j]));
            } else {
                item->setText("");
            }
            // 设置文字居中
            item->setTextAlignment(Qt::AlignCenter);
            // 设置字体
            QFont font = item->font();
            font.setPointSize(40);
            font.setBold(m_fixed[i][j]);
            item->setFont(font);
        }
    }
    // 之后进行冲突高亮 (会覆盖背景色)
    highlightConflicts();

    // 如果之前有选中的单元格，恢复选中状态
    if (m_currentRow >= 0 && m_currentRow < 9 && m_currentCol >= 0 && m_currentCol < 9) {
        m_table->setCurrentCell(m_currentRow, m_currentCol);
    }
}

void SudokuGame::findAllConflicts(bool conflict[9][9]) const
{
    // 初始化冲突数组为false
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            conflict[i][j] = false;

    // 检查每一行
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            int value = m_board[row][col];
            if (value == 0) continue;
            // 检查同行右侧格子
            for (int k = col + 1; k < 9; ++k) {
                if (m_board[row][k] == value) {
                    conflict[row][col] = true;
                    conflict[row][k] = true;
                }
            }
            // 检查同列下方格子
            for (int k = row + 1; k < 9; ++k) {
                if (m_board[k][col] == value) {
                    conflict[row][col] = true;
                    conflict[k][col] = true;
                }
            }
            // 检查所在宫格 (3x3)
            int startRow = (row / 3) * 3;
            int startCol = (col / 3) * 3;
            for (int i = startRow; i < startRow + 3; ++i) {
                for (int j = startCol; j < startCol + 3; ++j) {
                    if (i == row && j == col) continue;
                    if (m_board[i][j] == value) {
                        conflict[row][col] = true;
                        conflict[i][j] = true;
                    }
                }
            }
        }
    }
}

void SudokuGame::highlightConflicts()
{
    bool conflict[9][9];
    findAllConflicts(conflict);

    // 为每个单元格设置背景色
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            QTableWidgetItem* item = m_table->item(i, j);
            if (!item) continue;

            QColor bgColor;
            if (m_fixed[i][j]) {
                // 预设格子: 浅灰色背景
                bgColor = QColor(220, 220, 220);
            } else {
                // 可编辑格子: 白色背景
                bgColor = QColor(255, 255, 255);
            }

            // 如果有冲突，覆盖为浅红色
            if (conflict[i][j]) {
                bgColor = QColor(255, 200, 200);
            }

            // 设置背景色
            item->setBackground(bgColor);

            // 设置文字颜色: 冲突时深红色，否则黑色
            if (conflict[i][j]) {
                item->setForeground(QColor(180, 0, 0));
            } else {
                item->setForeground(QColor(0, 0, 0));
            }
        }
    }
}

bool SudokuGame::isConflict(int row, int col, int value) const
{
    if (value == 0) return false;
    // 检查行
    for (int c = 0; c < 9; ++c) {
        if (c != col && m_board[row][c] == value) return true;
    }
    // 检查列
    for (int r = 0; r < 9; ++r) {
        if (r != row && m_board[r][col] == value) return true;
    }
    // 检查宫
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = startRow; i < startRow + 3; ++i) {
        for (int j = startCol; j < startCol + 3; ++j) {
            if ((i != row || j != col) && m_board[i][j] == value) return true;
        }
    }
    return false;
}

void SudokuGame::resetToOriginal()
{
    // 将当前盘面恢复为原始预设盘面
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            m_board[i][j] = m_originalBoard[i][j];
            m_fixed[i][j] = (m_originalBoard[i][j] != 0);
        }
    }
    refreshTableDisplay();
    updateStatusMessage("已重置到初始状态");
}

bool SudokuGame::isVictory() const
{
    // 检查是否所有格子都已填满且无冲突
    bool conflict[9][9];
    findAllConflicts(conflict);

    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            if (m_board[i][j] == 0) return false;
            if (conflict[i][j]) return false;
        }
    }
    return true;
}

void SudokuGame::updateStatusMessage(const QString& msg)
{
    m_statusLabel->setText(msg);
}

// ===================== 槽函数实现 =====================

void SudokuGame::onCellSelected(int row, int col)
{
    if (row >= 0 && row < 9 && col >= 0 && col < 9) {
        m_currentRow = row;
        m_currentCol = col;
        // 可选: 显示当前选中的格子提示
        updateStatusMessage(QString("选中格子 (%1, %2)").arg(row+1).arg(col+1));
    }
}

void SudokuGame::onNumberButtonClicked(int number)
{
    if (m_soundEffect) m_writeSoundEffect->play();
    if (m_currentRow == -1 || m_currentCol == -1) {
        updateStatusMessage("请先点击一个格子！");
        return;
    }

    // 检查是否为预设格子
    if (m_fixed[m_currentRow][m_currentCol]) {
        updateStatusMessage("预设题目格子不能修改！");
        return;
    }

    // 修改值
    int oldValue = m_board[m_currentRow][m_currentCol];
    if (oldValue == number) {
        // 如果相同则不作处理
        return;
    }

    m_board[m_currentRow][m_currentCol] = number;
    refreshTableDisplay(); // 刷新显示并重新高亮冲突

    // 检查胜利
    if (isVictory()) {
        QMessageBox::information(this, "恭喜！", "你完成了数独！🎉 太棒了！");
        updateStatusMessage("游戏胜利！");
    } else {
        updateStatusMessage(QString("已在格子 (%1, %2) 填入 %3").arg(m_currentRow+1).arg(m_currentCol+1).arg(number));
    }
}

void SudokuGame::onDeleteButtonClicked()
{
    if (m_soundEffect) m_delSoundEffect->play();
    if (m_currentRow == -1 || m_currentCol == -1) {
        updateStatusMessage("请先点击一个格子！");
        return;
    }

    if (m_fixed[m_currentRow][m_currentCol]) {
        updateStatusMessage("预设题目格子不能清除！");
        return;
    }

    if (m_board[m_currentRow][m_currentCol] != 0) {
        m_board[m_currentRow][m_currentCol] = 0;
        refreshTableDisplay();
        updateStatusMessage(QString("已清除格子 (%1, %2)").arg(m_currentRow+1).arg(m_currentCol+1));
    } else {
        updateStatusMessage("当前格子已经是空的");
    }
}

void SudokuGame::onNewGame()
{
    // 随机选择新题目
    int newPreset = getRandomPresetIndex();
    initGame(newPreset);
    m_currentRow = -1;
    m_currentCol = -1;
    startGameTimer();
}

void SudokuGame::onResetGame()
{
    resetToOriginal();
}

void SudokuGame::onClearAll()
{
<<<<<<< Updated upstream
    // 清除所有用户填写的数字 (非预设格子置零)
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            if (!m_fixed[i][j]) {
                m_board[i][j] = 0;
            }
        }
=======
    stopGameTimer();
    // 暂停菜单
    QMessageBox msgBox1;
    msgBox1.setWindowTitle("游戏菜单");
    msgBox1.setText("游戏菜单");
    QPushButton* cancelBtn = msgBox1.addButton("取消" ,QMessageBox::ActionRole);
    QPushButton* musicBtn = msgBox1.addButton("开启或关闭音乐", QMessageBox::ActionRole);
    QPushButton* soundeffectBtn = msgBox1.addButton("开启或关闭音效", QMessageBox::ActionRole);
    QPushButton* answerBtn = msgBox1.addButton("查看答案", QMessageBox::ActionRole);
    msgBox1.setDefaultButton(cancelBtn);
    msgBox1.exec();
    if (msgBox1.clickedButton() == answerBtn){
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                if (!m_fixed[i][j])
                    m_board[i][j] = m_answerBoards[i][j];
            }
        }
        refreshTableDisplay();
        QTimer::singleShot(1500, this, &SudokuGame::onNewGame_NoCancel);
    }
    if (msgBox1.clickedButton() == musicBtn){
        if (m_music){
            m_music = 0;
            m_audioOutput->setVolume(0.0);
        }else{
            m_music = 1;
            m_audioOutput->setVolume(0.3);
        }
        resumeGameTimer();
    }
    if (msgBox1.clickedButton() == soundeffectBtn){
        m_soundEffect?m_soundEffect = 0:m_soundEffect = 1;
        resumeGameTimer();
    }
    if (msgBox1.clickedButton() == cancelBtn){
        resumeGameTimer();
>>>>>>> Stashed changes
    }
    refreshTableDisplay();
    updateStatusMessage("已清除所有用户填入的数字");
}

void SudokuGame::onCheckGame()
{
    bool conflict[9][9];
    findAllConflicts(conflict);
    bool hasConflict = false;
    bool hasEmpty = false;

    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            if (m_board[i][j] == 0) hasEmpty = true;
            if (conflict[i][j]) hasConflict = true;
        }
    }

    if (hasConflict) {
        QMessageBox::warning(this, "检查结果", "当前盘面存在冲突（红色格子），请修正后再继续。");
        updateStatusMessage("存在冲突，请修改红色格子");
    } else if (hasEmpty) {
        QMessageBox::information(this, "检查结果", "盘面尚未填满，继续加油！");
        updateStatusMessage("盘面未完成，继续努力");
    } else {
        QMessageBox::information(this, "检查结果", "完美！所有格子正确且无冲突，你赢了！");
        updateStatusMessage("游戏胜利！🎉");
    }
}

void SudokuGame::startGameTimer()
{
    stopGameTimer();                 // 先停止之前的计时
    m_elapsedSeconds = 0;
    updateTimerDisplay();
    m_timer->start(1000);            // 每秒触发
}

void SudokuGame::stopGameTimer()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void SudokuGame::updateTimerDisplay()
{
    int minutes = m_elapsedSeconds / 60;
    int seconds = m_elapsedSeconds % 60;
    QString timeStr = QString("%1:%2")
                          .arg(minutes, 2, 10, QChar('0'))
                          .arg(seconds, 2, 10, QChar('0'));
    m_timerLabel->setText(timeStr);
}

void SudokuGame::onTimerTick()
{
    m_elapsedSeconds++;
    updateTimerDisplay();
}

<<<<<<< Updated upstream
// 以下为题目生成部分
=======

// 题目生成

>>>>>>> Stashed changes
// 题目标准模板
const int INIT_BOARD[9][9] = {
    {9, 4, 5, 3, 2, 7, 1, 8, 6},
    {6, 8, 3, 1, 9, 5, 7, 4, 2},
    {2, 1, 7, 8, 6, 4, 9, 3, 5},
    {8, 2, 4, 5, 3, 9, 6, 7, 1},
    {5, 7, 1, 2, 8, 6, 3, 9, 4},
    {3, 6, 9, 7, 4, 1, 5, 2, 8},
    {7, 9, 2, 6, 1, 8, 4, 5, 3},
    {1, 5, 8, 4, 7, 3, 2, 6, 9},
    {4, 3, 6, 9, 5, 2, 8, 1, 7}
};

// 应用数字映射（随机置换 1~9）
void applyMapping(int board[9][9], const int mapping[10]) {
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            board[i][j] = mapping[board[i][j]];
}

// 随机打乱行（块间 + 块内）
void shuffleRows(int board[9][9], std::mt19937& rng) {
    // 块顺序排列（三个块：0-2, 3-5, 6-8）
    std::vector<int> blockOrder = {0, 1, 2};
    std::shuffle(blockOrder.begin(), blockOrder.end(), rng);

    int tmpBoard[9][9];
    // 先按新块顺序复制行
    for (int block = 0; block < 3; ++block) {
        int srcBlock = blockOrder[block];
        for (int rowInBlock = 0; rowInBlock < 3; ++rowInBlock) {
            int srcRow = srcBlock * 3 + rowInBlock;
            int dstRow = block * 3 + rowInBlock;
            for (int col = 0; col < 9; ++col)
                tmpBoard[dstRow][col] = board[srcRow][col];
        }
    }
    // 复制回原数组
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            board[i][j] = tmpBoard[i][j];

    // 每个块内部随机排列三行的顺序
    for (int block = 0; block < 3; ++block) {
        std::vector<int> rowOrder = {0, 1, 2};
        std::shuffle(rowOrder.begin(), rowOrder.end(), rng);
        // 暂存当前块的三行
        int blockRows[3][9];
        int base = block * 3;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 9; ++j)
                blockRows[i][j] = board[base + i][j];
        // 按新顺序放回
        for (int i = 0; i < 3; ++i) {
            int src = rowOrder[i];
            for (int j = 0; j < 9; ++j)
                board[base + i][j] = blockRows[src][j];
        }
    }
}

// 随机打乱列（块间 + 块内）
void shuffleCols(int board[9][9], std::mt19937& rng) {
    // 块顺序排列（三个块：0-2, 3-5, 6-8）
    std::vector<int> blockOrder = {0, 1, 2};
    std::shuffle(blockOrder.begin(), blockOrder.end(), rng);

    int tmpBoard[9][9];
    // 先按新块顺序复制列
    for (int block = 0; block < 3; ++block) {
        int srcBlock = blockOrder[block];
        for (int colInBlock = 0; colInBlock < 3; ++colInBlock) {
            int srcCol = srcBlock * 3 + colInBlock;
            int dstCol = block * 3 + colInBlock;
            for (int row = 0; row < 9; ++row)
                tmpBoard[row][dstCol] = board[row][srcCol];
        }
    }
    // 复制回原数组
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            board[i][j] = tmpBoard[i][j];

    // 每个块内部随机排列三列的顺序
    for (int block = 0; block < 3; ++block) {
        std::vector<int> colOrder = {0, 1, 2};
        std::shuffle(colOrder.begin(), colOrder.end(), rng);
        // 暂存当前块的三列
        int blockCols[3][9];
        int base = block * 3;
        for (int j = 0; j < 3; ++j)
            for (int i = 0; i < 9; ++i)
                blockCols[j][i] = board[i][base + j];
        // 按新顺序放回
        for (int j = 0; j < 3; ++j) {
            int src = colOrder[j];
            for (int i = 0; i < 9; ++i)
                board[i][base + j] = blockCols[src][i];
        }
    }
}

void SudokuGame::generate_test(){
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::mt19937 rng(static_cast<unsigned>(seed));

    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            m_presetBoards[i][j] = INIT_BOARD[i][j];

    int mapping[10];
    std::vector<int> digits = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(digits.begin(), digits.end(), rng);
    for (int i = 1; i <= 9; ++i)
        mapping[i] = digits[i-1];
    applyMapping(m_presetBoards, mapping);
    shuffleRows(m_presetBoards, rng);
    shuffleCols(m_presetBoards, rng);

    std::uniform_int_distribution<> dis(40, 55);
    std::mt19937 gen(static_cast<unsigned>(seed));
    int line[9];
    // 生成随机数
    int randomNumber = dis(gen);
    //std::cout << randomNumber << " ";
    int arrange = randomNumber/9;
    int intrad = arrange*9;
    int plus = randomNumber - intrad;
    //std::cout << arrange << " ";
    for (int i = 0;i<9;i++){
        line[i] = arrange;
        //std::cout << line[i] << " ";
    }
    for (int i = 0;i<plus;i++){
        line[i]++;
    }

    bool used[9] = {0};
    int count;

    for (int i = 0;i<9;i++){
        count = 0;
        for (int j = 0;j<9;j++){
            used[j] = 0;
        }
        while (count < line[i]){
            randomNumber = dis(gen);
            int index = randomNumber%9;
            if (!used[index]){
                m_presetBoards[i][index] = 0;
                used[index] = 1;
                count++;
            }
        }
    }
}




