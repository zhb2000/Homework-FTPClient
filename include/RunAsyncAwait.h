#ifndef RUN_ASYNC_AWAIT_H
#define RUN_ASYNC_AWAIT_H

#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <functional>

namespace utils
{
    /**
     * @brief 通过函数指针获取函数的返回类型
     * @author zhb
     * @details decltype(funcPtr) 即可推导出返回类型
     */
    template <class ReturnType, class... Args>
    ReturnType funcPtrRetType(ReturnType (*)(Args...));

    /**
     * @brief 异步执行某个函数（有返回值的函数）
     * @author zhb
     * @tparam ReturnType 函数返回类型，需要显式指定
     * @param func 需要异步执行的函数
     * @param args 变长参数
     * @return 函数的执行结果
     */
    template <class ReturnType, class Function, class... Args>
    inline ReturnType asyncAwait(Function func, Args &&... args)
    {
        QFuture<ReturnType> future = QtConcurrent::run(
            [&]() { return func(std::forward<Args>(args)...); });
        while (!future.isFinished())
            QApplication::processEvents();

        return future.result();
    }

    /**
     * @brief 异步执行某个函数（无返回值的函数）
     * @author zhb
     */
    template <class Function, class... Args>
    inline void asyncAwait(Function func, Args &&... args)
    {
        QFuture<void> future = QtConcurrent::run(
            [&]() { return func(std::forward<Args>(args)...); });
        while (!future.isFinished())
            QApplication::processEvents();
    }

} // namespace utils

#endif // RUN_ASYNC_AWAIT_H
