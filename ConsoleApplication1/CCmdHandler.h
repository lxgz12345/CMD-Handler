#pragma once

#ifndef __CMD_HANDLER_H__
#define __CMD_HANDLER_H__

//�޸��� https://blog.csdn.net/explorer114/article/details/79951972
//������CSDN�������������ǱȽϵ͵� =_=

#include <Windows.h>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

struct cmdParam {
    int iTimeOut; // ��ʱʱ�䣬��λ����
    std::string szCommand; // ������Ҫִ�е�����
    std::function<void(const std::string& szCommand, BOOL hResultCode, const std::string& szResult, DWORD exitCode)> OnCmdEvent; // ����ִ�к�Ļص�

    // Ĭ�Ϲ��캯��
    cmdParam() : iTimeOut(0) {}

    // �Զ��忽�����캯��
    cmdParam(const cmdParam& other)
        : iTimeOut(other.iTimeOut), // ֱ�Ӹ��ƻ�������
        szCommand(other.szCommand), // std::string �ṩ�Լ��Ŀ������캯��
        OnCmdEvent(other.OnCmdEvent) // std::function Ҳ�ṩ�Լ��Ŀ������캯��
    {
    }

    // �����Ҫ�������Զ��忽����ֵ�����
    cmdParam& operator=(const cmdParam& other) {
        if (this != &other) { // �Ը�ֵ���
            iTimeOut = other.iTimeOut;
            szCommand = other.szCommand;
            OnCmdEvent = other.OnCmdEvent;
        }
        return *this;
    }
};


/* buffer����󳤶� */
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
    * ��ʼ���ӿڣ���������ӿ�֮ǰ����
    */
    BOOL Initalize();
    /*
    * �����ӿ�
    */
    void Finish();
    /*
    * ִ������ӿڣ��ӿڵ��óɹ�����S_OK
    * param[in] pCommmandParam: ָ��һ��CHCmdParam��������ṹ��ָ��
    */
    BOOL HandleCommand(cmdParam* pCommmandParam);
    /*
    * ���ش����룬���ڲ��ӿڵ���ʧ�ܺ����ʲô����
    */
    DWORD GetErrorCode() const { return m_dwErrorCode; }
    /*
    * ����HandleCommand�ĵȴ�ʱ��Ϊ����
    */
    void SetWaitTimeInfinite(bool _isInfinite);
};

#endif // !__CMD_HANDLER_H__
