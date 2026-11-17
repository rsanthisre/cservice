
#pragma once
#include <string>

class SvcInstaller
{
public:
     void RunService();
     void InstallService();
     void UninstallService();

     SvcInstaller() {}
     ~SvcInstaller() {}

private:
     void InstallService(std::wstring wsServiceName,
         std::wstring wsDisplayName,
        DWORD dwStartType,
         std::wstring wsDependencies,
         std::wstring wsAccount,
         std::wstring wsPassword);

     void UninstallService(std::wstring wsServiceName);

   
};
