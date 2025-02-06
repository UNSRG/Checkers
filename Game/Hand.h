#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
// Класс Hand отвечает за обработку ввода игрока (клик мыши, закрытие окна и т.д.)
class Hand
{
  public:
    Hand(Board *board) : board(board)
    {
    }
    // Метод для получения выбранной ячейки на доске
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;  // Структура для хранения событий SDL
        Response resp = Response::OK; // Переменная для хранения типа ответа
        int x = -1, y = -1; // Координаты клика мыши
        int xc = -1, yc = -1; // Вычисленные координаты клетки на доске
        while (true) // Бесконечный цикл для ожидания события
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие нового события
            {
                switch (windowEvent.type) // Обрабатываем тип события
                {
                case SDL_QUIT: // Если игрок закрыл окно
                    resp = Response::QUIT; // Устанавливаем ответ QUIT
                    break;
                case SDL_MOUSEBUTTONDOWN:  // Если игрок нажал кнопку мыши
                    x = windowEvent.motion.x; // Получаем координату X клика
                    y = windowEvent.motion.y; // Получаем координату Y клика
                    xc = int(y / (board->H / 10) - 1); // Вычисляем координату X клетки на доске
                    yc = int(x / (board->W / 10) - 1); // Вычисляем координату Y клетки на доске
                    // Обработка специальных клеток для отката хода и повторной игры
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Устанавливаем ответ BACK (откат хода)
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Устанавливаем ответ REPLAY (повторная игра)
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Устанавливаем ответ CELL (выбор клетки)
                    }
                    else
                    {
                        xc = -1; // Сбрасываем координаты, если они некорректны
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // Обработка событий окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) // Если размер окна изменился
                    {
                        board->reset_window_size(); // Сбрасываем размер окна
                        break;
                    }
                }
                if (resp != Response::OK) // Если ответ не равен OK, выходим из цикла
                    break;
            }
        }
        return {resp, xc, yc}; // Возвращаем ответ и координаты клетки
    }
    // Метод для ожидания действия игрока (например, выбора повторной игры)
    Response wait() const
    {
        SDL_Event windowEvent; // Структура для хранения событий SDL
        Response resp = Response::OK; // Переменная для хранения типа ответа
        while (true) // Бесконечный цикл для ожидания события
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие нового события
            {
                switch (windowEvent.type) // Обрабатываем тип события
                {
                case SDL_QUIT: // Если игрок закрыл окно
                    resp = Response::QUIT; // Устанавливаем ответ QUIT
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: // Если размер окна изменился
                    board->reset_window_size(); // Сбрасываем размер окна
                    break;
                case SDL_MOUSEBUTTONDOWN: // Если игрок нажал кнопку мыши
                { 
                    int x = windowEvent.motion.x; // Получаем координату X клика
                    int y = windowEvent.motion.y; // Получаем координату Y клика
                    int xc = int(y / (board->H / 10) - 1); // Вычисляем координату X клетки на доске
                    int yc = int(x / (board->W / 10) - 1); // Вычисляем координату Y клетки на доске
                    if (xc == -1 && yc == 8) // Обработка специальной клетки для повторной игры
                        resp = Response::REPLAY; // Устанавливаем ответ REPLAY
                }
                break;
                }
                if (resp != Response::OK) // Если ответ не равен OK, выходим из цикла
                    break;
            }
        }
        return resp; // Возвращаем ответ
    }

  private:
    Board *board; // Указатель на объект доски
};
