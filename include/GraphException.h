// GraphException.h - 图模块异常类

#ifndef CAMPUSNAVIGATION_GRAPHEXCEPTION_H
#define CAMPUSNAVIGATION_GRAPHEXCEPTION_H

#include<exception>
#include<string>
#include <utility>

namespace Graph {

    class GraphException : public std::exception {
    private:
        std::string message;  // 自定义错误消息

    public:
        // 构造函数接受一个错误消息
        explicit GraphException(std::string msg) : message(std::move(msg)) {}

        // 重载 what() 方法，返回错误消息
        const char *what() const noexcept override {
            return message.c_str();
        }
    };

} // Graph

#endif //CAMPUSNAVIGATION_GRAPHEXCEPTION_H
