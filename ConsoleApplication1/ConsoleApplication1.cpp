#include <iostream>
#include <string>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <memory>
#include "CCmdHandler.h"

void CmdEvent(const std::string& szCommand, int hResultCode, const std::string& szResult, DWORD exitCode) {
    auto trimTrailingNewline = [](std::string& str) {
        if (!str.empty() && str.back() == '\n') 
            str.pop_back(); // 删除最后一个字符（'\n'）
        if (!str.empty() && str.back() == '\r') 
            str.pop_back(); // 如果现在最后一个字符是 '\r'，也删除它
    };
    auto isWhiteSpace = [](const std::string& str) {
        return std::all_of(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
    };

    std::cout << "执行命令：" << szCommand << std::endl;
    std::string ok = hResultCode == TRUE ? "成功" : "失败";
    std::string ret = isWhiteSpace(szResult) ? "空" : szResult;
    trimTrailingNewline(ret);
    std::cout << "执行" << ok << "，exitCode " << exitCode << "，返回：" << ret << std::endl << std::endl;
};

int main() {
    setlocale(LC_ALL, "chs");

    cmdHandler cmd;
    if (!cmd.Initalize()) {
        std::cerr << "CMD初始化失败 " << cmd.GetErrorCode() << std::endl;
        return 1;
    }

    while (true) {
        std::cout << "请输入命令：";
        std::string input;
        std::getline(std::cin, input);
        if (input.empty()) {
            continue;
        }
        if (input == "quit") {
            std::cout << "退出" << std::endl;
            break;
        }

        std::unique_ptr<cmdParam> cmdp(new cmdParam);
        cmdp->szCommand = input;
        cmdp->iTimeOut = 5000;
        cmdp->OnCmdEvent = CmdEvent;
        cmd.HandleCommand(cmdp.get());
    }
}
