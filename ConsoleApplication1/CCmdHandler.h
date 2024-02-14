#pragma once

#ifndef __CMD_HANDLER_H__
#define __CMD_HANDLER_H__

//修改自 https://blog.csdn.net/explorer114/article/details/79951972
//不愧是CSDN，代码质量还是比较低的 =_=

#include <Windows.h>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

struct cmdParam {
    int iTimeOut; // 超时时间，单位毫秒
    std::string szCommand; // 命令行要执行的命令
    std::function<void(const std::string& szCommand, BOOL hResultCode, const std::string& szResult, DWORD exitCode)> OnCmdEvent; // 命令执行后的回调

    // 默认构造函数
    cmdParam() : iTimeOut(0) {}

    // 自定义拷贝构造函数
    cmdParam(const cmdParam& other)
        : iTimeOut(other.iTimeOut), // 直接复制基本类型
        szCommand(other.szCommand), // std::string 提供自己的拷贝构造函数
        OnCmdEvent(other.OnCmdEvent) // std::function 也提供自己的拷贝构造函数
    {
    }

    // 如果需要，还可以定义拷贝赋值运算符
    cmdParam& operator=(const cmdParam& other) {
        if (this != &other) { // 自赋值检查
            iTimeOut = other.iTimeOut;
            szCommand = other.szCommand;
            OnCmdEvent = other.OnCmdEvent;
        }
        return *this;
    }
};


/* buffer的最大长度 */
#define PIPE_BUFFER_SIZE 8192


class cmdHandler
{
private:
    BOOL m_bInit;
    STARTUPINFOA m_startupInfo;
    PROCESS_INFORMATION m_processInfo;
    DWORD m_dwErrorCode;
    HANDLE m_hPipeRead;
    HANDLE m_hPipeWrite;
    SECURITY_ATTRIBUTES m_saOutPipe;
    char m_szPipeOut[PIPE_BUFFER_SIZE];
    bool isInfinite;
public:
    cmdHandler();
    ~cmdHandler();
    /*
    * 初始化接口，调用其余接口之前调用
    */
    BOOL Initalize();
    /*
    * 结束接口
    */
    void Finish();
    /*
    * 执行命令接口，接口调用成功返回S_OK
    * param[in] pCommmandParam: 指向一个CHCmdParam命令参数结构的指针
    */
    BOOL HandleCommand(cmdParam* pCommmandParam);
    /*
    * 返回错误码，便于差距接口调用失败后产生什么错误
    */
    DWORD GetErrorCode() const { return m_dwErrorCode; }
    /*
    * 设置HandleCommand的等待时间为无限
    */
    void SetWaitTimeInfinite(bool _isInfinite);
};

#endif // !__CMD_HANDLER_H__
