#pragma once
#include "manager.h"
#include <string>
class Shell{
    Manager mng;
    std::string handleInput(std::string& input);
public:
    Shell();
    ~Shell();

    void run(); // ����shell�������������Manager
};

