#pragma once

#include <iostream>
#include <string>
#include <time.h>

// 时间戳类
class Timestamp
{
public:
    Timestamp();                            // 用当前时间初始化对象
    Timestamp(uint64_t secsinceepoch);      // 用一个整数表示的时间初始化对象

    static Timestamp now();                 // 返回当前时间的Timestamp对象

    time_t toint() const;                   // 返回整数表示的时间
    std::string tostring() const;           // 返回字符串表示的时间，格式：yyy-mm-dd hh24:mi:ss

private:
    time_t secsinceepoch_;                  // 整数表示的时间(从1970到现在的秒数)
};