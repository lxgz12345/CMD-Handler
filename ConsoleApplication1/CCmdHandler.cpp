#include "CCmdHandler.h"
#include <tchar.h>

#define CHECK_INIT_STATE if (!m_bInit) return FALSE;

cmdHandler::cmdHandler()
    : m_bInit(FALSE)
    , m_dwErrorCode(0)
    , m_hPipeRead(NULL)
    , m_hPipeWrite(NULL)
    , m_startupInfo({ 0 })
    , m_processInfo({ 0 })
    , m_saOutPipe({ 0 })
    , isInfinite(false)
{
    ZeroMemory(m_szPipeOut, sizeof(m_szPipeOut));
}


cmdHandler::~cmdHandler()
{
    Finish();
}

BOOL cmdHandler::Initalize()
{
    if (m_bInit) return TRUE;
    m_bInit = TRUE;

    // 设置安全属性，允许句柄继承
    ZeroMemory(&m_saOutPipe, sizeof(m_saOutPipe));
    m_saOutPipe.nLength = sizeof(SECURITY_ATTRIBUTES);
    m_saOutPipe.bInheritHandle = TRUE; // 允许句柄被继承
    m_saOutPipe.lpSecurityDescriptor = NULL; // 使用默认安全描述符

    if (0 == CreatePipe(&m_hPipeRead, &m_hPipeWrite, &m_saOutPipe, PIPE_BUFFER_SIZE))
    {
        m_dwErrorCode = GetLastError();
        return FALSE;
    }
    return TRUE;
}

void cmdHandler::Finish()
{
    if (!m_bInit) return;
    if (m_hPipeRead)
    {
        CloseHandle(m_hPipeRead);
        m_hPipeRead = NULL;
    }
    if (m_hPipeWrite)
    {
        CloseHandle(m_hPipeWrite);
        m_hPipeWrite = NULL;
    }
    m_bInit = FALSE;
}

BOOL cmdHandler::HandleCommand(cmdParam* pCommandParam)
{
    if (!m_bInit || !pCommandParam) return FALSE;
    auto CloseHandleDeleter = [](HANDLE handle) {
        if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
    };

    DWORD dwReadLen = 0;
    DWORD dwStdLen = 0;
    ZeroMemory(&m_startupInfo, sizeof(STARTUPINFOA)); // 清零初始化
    m_startupInfo.cb = sizeof(STARTUPINFO);
    m_startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    m_startupInfo.hStdOutput = m_hPipeWrite;
    m_startupInfo.hStdError = m_hPipeWrite;
    m_startupInfo.wShowWindow = SW_HIDE;
    
    DWORD dTimeOut;
    if (isInfinite)
        dTimeOut = INFINITE;
    else
        dTimeOut = pCommandParam->iTimeOut >= 3000 ? pCommandParam->iTimeOut : 5000;

    // 使用智能指针自动关闭进程和线程句柄
    std::unique_ptr<void, decltype(CloseHandleDeleter)> hThread(nullptr, CloseHandleDeleter);
    std::unique_ptr<void, decltype(CloseHandleDeleter)> hProcess(nullptr, CloseHandleDeleter);

    std::string commandLine = "cmd.exe /c " + pCommandParam->szCommand;
    std::vector<char> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back(0); // 确保字符串以 null 结尾

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &m_startupInfo, &m_processInfo)) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "无法创建进程", 0L);
        return FALSE; // 无法创建进程
    }

    // 重置智能指针以管理新的句柄
    hThread.reset(m_processInfo.hThread);
    hProcess.reset(m_processInfo.hProcess);

    if (WAIT_TIMEOUT == WaitForSingleObject(m_processInfo.hProcess, INFINITE)) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "命令执行超时", 0L);
        return FALSE; // 进程启动超时
    }

    // 检查管道中是否有数据可读
    if (!PeekNamedPipe(m_hPipeRead, NULL, 0, NULL, &dwReadLen, NULL) || dwReadLen <= 0) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "预览管道失败", 0L);
        return FALSE; // 检查管道失败
    }

    std::vector<char> pipeOutput(dwReadLen + 1); // +1 为了 null-terminator
    if (!ReadFile(m_hPipeRead, pipeOutput.data(), dwReadLen, &dwStdLen, NULL)) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "读取管道失败", 0L);
        return FALSE; // 读取管道数据失败
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(m_processInfo.hProcess, &exitCode);

    pipeOutput[dwStdLen] = '\0'; // 确保输出是 null-terminated
    if (pCommandParam->OnCmdEvent)
        pCommandParam->OnCmdEvent(pCommandParam->szCommand, TRUE, pipeOutput.data(), exitCode);

    return TRUE; // 命令执行成功
}

void cmdHandler::SetWaitTimeInfinite(bool _isInfinite) {
    this->isInfinite = _isInfinite;
}