#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config() // Конструктор класса Config.
    {
        reload(); // Вызов метода reload() при создании объекта для загрузки начальной конфигурации.
    }

    void reload() // Метод для перезагрузки конфигурационных данных из файла.
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    auto operator()(const string &setting_dir, const string &setting_name) const
    {
       // Этот оператор позволяет использовать объект класса Config как функцию для получения значений из конфигурационного файла.
       return config[setting_dir][setting_name];
    }

  private:
    json config;
};
