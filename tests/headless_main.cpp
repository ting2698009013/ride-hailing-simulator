#include "tests/SelfTest.h"

#include <iostream>

int main() {
    std::wcout << L"========== 开始无界面核心自检 ==========" << std::endl;
    const bool allPassed = SelfTest::runAll();
    std::wcout << L"========== 自检结束："
               << (allPassed ? L"全部通过" : L"存在失败")
               << L" ==========" << std::endl;
    return allPassed ? 0 : 1;
}
