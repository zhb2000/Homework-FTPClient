#ifndef SCOPEGUARD_H
#define SCOPEGUARD_H

#include <functional>

namespace utils
{
    /**
     * @brief 离开作用域后自动释放资源
     * @author zhb
     */
    class ScopeGuard
    {
    public:
        /**
         * @param onExit 清理函数
         */
        explicit ScopeGuard(std::function<void()> onExit)
            : onExit_(onExit), dismissed_(false)
        {
        }

        ~ScopeGuard()
        {
            if (!dismissed_)
                onExit_();
        }

        /**
         * @brief 取消清理
         * @param setDismissed 是否取消清理
         */
        void dismiss(bool setDismissed = true) { dismissed_ = setDismissed; }

        //禁止复制
        ScopeGuard(ScopeGuard const &) = delete;
        ScopeGuard &operator=(ScopeGuard const &) = delete;

    private:
        //清理函数
        std::function<void()> onExit_;
        //是否取消清理
        bool dismissed_;
    };
} // namespace utils

#endif // SCOPEGUARD_H
