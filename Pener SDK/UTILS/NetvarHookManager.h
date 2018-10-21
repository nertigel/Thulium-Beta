#pragma once

namespace SDK
{
	class CRecvProxyData;
	struct RecvTable;
}
namespace UTILS
{
	typedef void(*NetvarHookCallback)(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut);

	class CNetvarHookManager
	{
	public:
		void Hook(std::string table, std::string prop, NetvarHookCallback callback);
		uintptr_t GetOffset(std::string table, std::string prop);

	private:
		uintptr_t RecursivelyFindOffset(SDK::RecvTable* table, std::string table_name, std::string prop_name);

	}; extern CNetvarHookManager netvar_hook_manager;
}