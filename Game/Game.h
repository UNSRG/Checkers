#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Функция для запуска игры в шашки
    int play()
    {
        auto start = chrono::steady_clock::now(); // Запоминаем время начала игры
        if (is_replay) // Если это повторная игра (например, после отката хода)
        {
            logic = Logic(&board, &config); // Пересоздаем объект логики игры
            config.reload(); // Перезагружаем конфигурацию из файла
            board.redraw(); // Перерисовываем доску
        }
        else
        {
            board.start_draw(); // Начинаем рисовать доску сначала
        }
        is_replay = false; // Сбрасываем флаг повторной игры

        int turn_num = -1; // Номер текущего хода
        bool is_quit = false; // Флаг выхода из игры
        const int Max_turns = config("Game", "MaxNumTurns"); // Максимальное количество ходов в игре
        while (++turn_num < Max_turns) // Цикл по всем ходам до достижения максимального количества ходов
        {
            beat_series = 0; // Сброс серии взятий
            logic.find_turns(turn_num % 2); // Находим возможные ходы для текущего игрока (0 - белые, 1 - черные)
            if (logic.turns.empty()) // Если нет доступных ходов, завершаем игру
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel")); // Устанавливаем глубину поиска для бота
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot"))) // Если текущий игрок не бот
            {
                auto resp = player_turn(turn_num % 2); // Выполняем ход игрока
                if (resp == Response::QUIT) // Если игрок выбрал выход
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // Если игрок выбрал повторную игру
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // Если игрок выбрал откат хода
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback(); // Откатываем ход
                        --turn_num; // Уменьшаем номер хода
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback(); // Откатываем ход
                    --turn_num; // Уменьшаем номер хода
                    beat_series = 0; // Сбрасываем серию взятий
                }
            }
            else
                bot_turn(turn_num % 2); // Выполняем ход бота
        }
        auto end = chrono::steady_clock::now(); // Запоминаем время окончания игры
        ofstream fout(project_path + "log.txt", ios_base::app); // Открываем файл лога для записи
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n"; // Записываем время игры в лог
        fout.close(); // Закрываем файл лога

        if (is_replay) // Если это повторная игра, запускаем игру снова
            return play();
        if (is_quit) // Если игрок выбрал выход, завершаем игру
            return 0;
        int res = 2; // Результат игры (по умолчанию ничья)
        if (turn_num == Max_turns) // Если достигнуто максимальное количество ходов, объявляем ничью
        {
            res = 0;
        }
        else if (turn_num % 2) // Если последний ход был сделан черными, объявляем победу белых
        {
            res = 1;
        }
        board.show_final(res); // Показываем результат игры на доске
        auto resp = hand.wait(); // Ждем действия игрока
        if (resp == Response::REPLAY) // Если игрок выбрал повторную игру, запускаем игру снова
        {
            is_replay = true;
            return play();
        }
        return res; // Возвращаем результат игры
    }

  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // Запоминаем время начала хода бота

        auto delay_ms = config("Bot", "BotDelayMS"); // Получаем задержку перед ходом бота из конфигурации
        // new thread for equal delay for each turn
        // Создаем новый поток для равномерной задержки каждого хода
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color); // Находим лучшие ходы для бота
        th.join(); // Ожидаем завершения потока задержки
        bool is_first = true; // Флаг первого хода в серии взятий
        // making moves
        // Выполняем найденные ходы
        for (auto turn : turns)
        {
            if (!is_first) // Если это не первый ход, добавляем задержку
            {
                SDL_Delay(delay_ms); 
            }
            is_first = false; // Сбрасываем флаг первого хода
            beat_series += (turn.xb != -1); // Увеличиваем серию взятий, если есть взятие
            board.move_piece(turn, beat_series); // Выполняем ход на доске
        }

        auto end = chrono::steady_clock::now(); // Запоминаем время окончания хода бота
        ofstream fout(project_path + "log.txt", ios_base::app); // Открываем файл лога для записи
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n"; // Записываем время хода бота в лог
        fout.close(); // Закрываем файл лога
    }

    Response player_turn(const bool color)
    {
        // return 1 if quit
        // Вектор для хранения выделенных ячеек
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns) // Проходим по всем доступным ходам
        {
            cells.emplace_back(turn.x, turn.y); // Добавляем координаты ходов в вектор
        }
        board.highlight_cells(cells); // Выделяем ячейки на доске
        move_pos pos = {-1, -1, -1, -1}; // Координаты выбранного хода
        POS_T x = -1, y = -1; // Координаты выбранной фигуры
        // trying to make first move
        // Пытаемся сделать первый ход
        while (true)
        {
            auto resp = hand.get_cell(); // Получаем координаты выбранной ячейки
            if (get<0>(resp) != Response::CELL) // Если это не выбор ячейки, возвращаем соответствующий ответ
                return get<0>(resp);
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // Координаты выбранной ячейки

            bool is_correct = false;  // Флаг корректности хода
            for (auto turn : logic.turns) // Проходим по всем доступным ходам
            {
                if (turn.x == cell.first && turn.y == cell.second) // Если выбранная ячейка совпадает с началом хода
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second}) // Если выбранная ячейка совпадает с концом хода
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1) // Если ход выбран, выходим из цикла
                break;
            if (!is_correct) // Если ход некорректен
            {
                if (x != -1) // Если уже была выбрана фигура, очищаем выделение
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y); // Устанавливаем активную фигуру
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns) // Проходим по всем доступным ходам
            {
                if (turn.x == x && turn.y == y) // Если ход начинается с выбранной фигуры
                {
                    cells2.emplace_back(turn.x2, turn.y2); // Добавляем координаты конца хода в вектор
                }
            }
            board.highlight_cells(cells2); // Выделяем возможные ходы для выбранной фигуры
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1); // Выполняем ход на доске
        if (pos.xb == -1) // Если не было взятия, возвращаем OK
            return Response::OK;
        // continue beating while can
        // Продолжаем серию взятий, пока это возможно
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2); // Находим доступные ходы для текущей позиции
            if (!logic.have_beats) // Если нет доступных взятий, завершаем серию
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns) // Проходим по всем доступным ходам
            {
                cells.emplace_back(turn.x2, turn.y2); // Добавляем координаты конца хода в вектор
            }
            board.highlight_cells(cells); // Выделяем возможные ходы для текущей позиции
            board.set_active(pos.x2, pos.y2); // Устанавливаем активную фигуру
            // trying to make move
            // Пытаемся сделать следующий ход в серии взятий
            while (true)
            {
                auto resp = hand.get_cell(); // Получаем координаты выбранной ячейки
                if (get<0>(resp) != Response::CELL) // Если это не выбор ячейки, возвращаем соответствующий ответ
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // Координаты выбранной ячейки

                bool is_correct = false; // Флаг корректности хода
                for (auto turn : logic.turns) // Проходим по всем доступным ходам
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second) // Если выбранная ячейка совпадает с концом хода
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct) // Если ход некорректен, продолжаем цикл
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1; // Увеличиваем серию взятий
                board.move_piece(pos, beat_series); // Выполняем ход на доске
                break;
            }
        }

        return Response::OK; // Возвращаем OK, если все ходы выполнены успешно
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
