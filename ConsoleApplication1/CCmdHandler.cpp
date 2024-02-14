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

    // ���ð�ȫ���ԣ��������̳�
    ZeroMemory(&m_saOutPipe, sizeof(m_saOutPipe));
    m_saOutPipe.nLength = sizeof(SECURITY_ATTRIBUTES);
    m_saOutPipe.bInheritHandle = TRUE; // ���������̳�
    m_saOutPipe.lpSecurityDescriptor = NULL; // ʹ��Ĭ�ϰ�ȫ������

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
    ZeroMemory(&m_startupInfo, sizeof(STARTUPINFOA)); // �����ʼ��
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

    // ʹ������ָ���Զ��رս��̺��߳̾��
    std::unique_ptr<void, decltype(CloseHandleDeleter)> hThread(nullptr, CloseHandleDeleter);
    std::unique_ptr<void, decltype(CloseHandleDeleter)> hProcess(nullptr, CloseHandleDeleter);

    std::string commandLine = "cmd.exe /c " + pCommandParam->szCommand;
    std::vector<char> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back(0); // ȷ���ַ����� null ��β

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &m_startupInfo, &m_processInfo)) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "�޷���������", 0L);
        return FALSE; // �޷���������
    }

    // ��������ָ���Թ����µľ��
    hThread.reset(m_processInfo.hThread);
    hProcess.reset(m_processInfo.hProcess);

    if (WAIT_TIMEOUT == WaitForSingleObject(m_processInfo.hProcess, INFINITE)) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "����ִ�г�ʱ", 0L);
        return FALSE; // ����������ʱ
    }

    // ���ܵ����Ƿ������ݿɶ�
    if (!PeekNamedPipe(m_hPipeRead, NULL, 0, NULL, &dwReadLen, NULL) || dwReadLen <= 0) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "Ԥ���ܵ�ʧ��", 0L);
        return FALSE; // ���ܵ�ʧ��
    }

    std::vector<char> pipeOutput(dwReadLen + 1); // +1 Ϊ�� null-terminator
    if (!ReadFile(m_hPipeRead, pipeOutput.data(), dwReadLen, &dwStdLen, NULL)) {
        m_dwErrorCode = GetLastError();
        if (pCommandParam->OnCmdEvent)
            pCommandParam->OnCmdEvent(pCommandParam->szCommand, FALSE, "��ȡ�ܵ�ʧ��", 0L);
        return FALSE; // ��ȡ�ܵ�����ʧ��
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(m_processInfo.hProcess, &exitCode);

    pipeOutput[dwStdLen] = '\0'; // ȷ������� null-terminated
    if (pCommandParam->OnCmdEvent)
        pCommandParam->OnCmdEvent(pCommandParam->szCommand, TRUE, pipeOutput.data(), exitCode);

    return TRUE; // ����ִ�гɹ�
}

void cmdHandler::SetWaitTimeInfinite(bool _isInfinite) {
    this->isInfinite = _isInfinite;
}