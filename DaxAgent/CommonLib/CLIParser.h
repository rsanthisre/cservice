#pragma once
#ifndef CLIPARSER_H
#define CLIPARSER_H
#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
namespace ADAX {
    class CLIParser {
    public:
        CLIParser(int argc, wchar_t* argv[]);

        bool HasSwitch(const std::wstring& switchName) const;

        std::wstring GetValue(const std::wstring& switchName, const std::wstring& defaultValue = L"") const;

    private:
        std::vector<std::wstring> args;
        std::unordered_map<std::wstring, bool> switches;
        std::unordered_map<std::wstring, std::wstring> values;

        void ParseArgs();
    };
}

#endif // CLIPARSER_H

