#pragma once
#include <stdlib.h>

// Тип для представления позиции на доске (8 бит целое число)
typedef int8_t POS_T;

// Структура для представления хода в игре
struct move_pos
{
    POS_T x, y;             // Координаты начальной клетки (откуда)
    POS_T x2, y2;           // Координаты конечной клетки (куда)
    POS_T xb = -1, yb = -1; // Координаты взятой фигуры (если есть)

    // Конструктор для создания хода без взятия фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    // Конструктор для создания хода с взятием фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }
    // Оператор сравнения на равенство двух ходов
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // Оператор сравнения на неравенство двух ходов
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other); // Используем оператор == для определения неравенства
    }
};
