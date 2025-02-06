#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }
    // Удалена реализация функции find_best_turns()
    vector<move_pos> find_best_turns(const bool color)
  

private:
    // Функция для выполнения хода на доске
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // Если есть взятие
            mtx[turn.xb][turn.yb] = 0; // Удаляем взятую фигуру
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7)) // Преобразование в дамку при достижении противоположного края доски
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем фигуру на новую позицию
        mtx[turn.x][turn.y] = 0; // Удаляем фигуру с начальной позиции
        return mtx; // Возвращаем обновленную матрицу доски
    }

    // Функция для вычисления оценки текущего состояния доски
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - кто является максимизирующим игроком
        double w = 0, wq = 0, b = 0, bq = 0; // Счетчики для белых и черных фигур и дамок
        for (POS_T i = 0; i < 8; ++i) // Проходим по всем клеткам доски
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // Подсчет белых фигур
                wq += (mtx[i][j] == 3); // Подсчет белых дамок
                b += (mtx[i][j] == 2); // Подсчет черных фигур
                bq += (mtx[i][j] == 4); // Подсчет черных дамок
                if (scoring_mode == "NumberAndPotential") // Если используется режим оценки "NumberAndPotential"
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Добавляем потенциал для белых фигур
                    b += 0.05 * (mtx[i][j] == 2) * (i); // Добавляем потенциал для черных фигур
                }
            }
        }
        if (!first_bot_color) // Если первый бот играет черными
        {
            swap(b, w); // Меняем местами счетчики для черных и белых фигур
            swap(bq, wq); // Меняем местами счетчики для черных и белых дамок
        }
        if (w + wq == 0) // Если нет белых фигур и дамок
            return INF; // Возвращаем бесконечность
        if (b + bq == 0) // Если нет черных фигур и дамок
            return 0; // Возвращаем ноль
        int q_coef = 4; // Коэффициент для дамок
        if (scoring_mode == "NumberAndPotential") // Если используется режим оценки "NumberAndPotential"
        {
            q_coef = 5; // Увеличиваем коэффициент для дамок
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Возвращаем оценку текущего состояния доски
    }
    // Удалена реализация функции find_best_turn()
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    
    // Удалена реализация функции find_best_turns_rec()
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    
public:
    // Нахождение всех возможных ходов для заданного цвета на текущей доске
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board()); // Вызов вспомогательной функции с текущей матрицей доски
    }

    // Нахождение всех возможных ходов для фигуры на заданных координатах на текущей доске
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board()); // Вызов вспомогательной функции с текущей матрицей доски
    }

private:
    // Вспомогательная функция для поиска всех возможных ходов для заданного цвета на заданной матрице доски
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns; // Вектор для хранения найденных ходов
        bool have_beats_before = false; // Флаг наличия взятий до начала поиска
        for (POS_T i = 0; i < 8; ++i) // Проходим по всем клеткам доски
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color) // Если клетка занята фигурой заданного цвета
                {
                    find_turns(i, j, mtx); // Находим все возможные ходы для этой фигуры
                    if (have_beats && !have_beats_before) // Если есть взятия и это первое взятие
                    {
                        have_beats_before = true;
                        res_turns.clear(); // Очищаем предыдущие ходы
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before) // Добавляем найденные ходы в результирующий вектор
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns; // Сохраняем найденные ходы
        shuffle(turns.begin(), turns.end(), rand_eng); // Перемешиваем ходы для случайности
        have_beats = have_beats_before; // Обновляем флаг наличия взятий
    }

    // Вспомогательная функция для поиска всех возможных ходов для фигуры на заданных координатах на заданной матрице доски
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear(); // Очищаем вектор ходов
        have_beats = false; // Сбрасываем флаг наличия взятий
        POS_T type = mtx[x][y]; // Тип фигуры на заданных координатах
        // Проверка взятий
        switch (type)
        {
        case 1:  // Белая фигура
        case 2: // Черная фигура
            // Проверка обычных фигур
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7) // Проверка выхода за границы доски
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2; // Координаты взятой фигуры
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2) // Проверка возможности взятия
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb); // Добавляем ход в вектор
                }
            }
            break;
        default:
            // Дамка
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2]) // Если клетка занята
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))  // Проверка возможности взятия
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2) // Если есть взятие
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb); // Добавляем ход в вектор
                        }
                    }
                }
            }
            break;
        }
        // Проверка других ходов
        if (!turns.empty())
        {
            have_beats = true; // Устанавливаем флаг наличия взятий
            return;
        }
        switch (type)
        {
        case 1: // Белая фигура
        case 2: // Черная фигура
            // Проверка обычных фигур
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j]) // Проверка выхода за границы доски и занятости клетки
                    continue;
                turns.emplace_back(x, y, i, j); // Добавляем ход в вектор
            }
            break;
        }
        default: // Дамка
            // Проверка дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2]) // Если клетка занята
                            break;
                        turns.emplace_back(x, y, i2, j2); // Добавляем ход в вектор
                    }
                }
            }
            break;
        }
    }

public:
    vector<move_pos> turns; // Вектор для хранения всех возможных ходов
    bool have_beats; // Флаг наличия взятийl
    int Max_depth;// Максимальная глубина поиска

private:
    default_random_engine rand_eng; // Генератор случайных чисел
    string scoring_mode; // Режим оценки текущего состояния доски
    string optimization; // Уровень оптимизации алгоритма
    vector<move_pos> next_move; // Вектор для хранения следующих ходов
    vector<int> next_best_state; // Вектор для хранения следующих состояний доски
    Board* board; // Указатель на объект доски
    Config* config; // Указатель на объект конфигурации
};