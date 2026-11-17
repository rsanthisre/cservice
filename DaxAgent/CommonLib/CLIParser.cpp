#include "stdafx.h"
#include "CLIParser.h"
#include <algorithm>
#include "WinUtils.h"
#include <unordered_map>
using namespace ADAX;

namespace
{
    bool LooksLikeSwitch(const std::wstring& token)
    {
        return !token.empty() && (token[0] == L'-' || token[0] == L'/');
    }
}

CLIParser::CLIParser(int argc, wchar_t* argv[]) {
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    ParseArgs();
}

void CLIParser::ParseArgs() {
    for (size_t i = 0; i < args.size(); ++i) {
        if (!LooksLikeSwitch(args[i]))
        {
            continue; // bare positional token (e.g. argv[0], the exe path); not a switch
        }

        std::wstring switchName = args[i];
        ADAX::WinUtils::ToLower(switchName);
        switches[switchName] = true;

        if (i + 1 < args.size() && !LooksLikeSwitch(args[i + 1]))
        {
            values[switchName] = args[i + 1];
            ++i; // consume the value so it isn't misread as its own switch
        }
    }
}

bool CLIParser::HasSwitch(const std::wstring& switchName) const {
    std::wstring key = switchName;
    ADAX::WinUtils::ToLower(key);
    return switches.find(key) != switches.end();
}

std::wstring CLIParser::GetValue(const std::wstring& switchName, const std::wstring& defaultValue) const {
    std::wstring key = switchName;
    ADAX::WinUtils::ToLower(key);
    auto it = values.find(key);
    return (it != values.end()) ? it->second : defaultValue;
}
