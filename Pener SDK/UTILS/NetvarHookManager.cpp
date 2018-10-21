#include "../includes.h"

#include "interfaces.h"
#include "../SDK/IClient.h"
#include "../SDK/RecvData.h"

#include "NetvarHookManager.h"

namespace UTILS
{
	CNetvarHookManager netvar_hook_manager;
	void CNetvarHookManager::Hook(std::string table_to_hook, std::string prop_to_hook, NetvarHookCallback callback)
	{
		auto client_class = INTERFACES::Client->GetAllClasses();

		// go through all classes
		while (client_class)
		{
			std::string table_name = client_class->m_pRecvTable->m_pNetTableName;

			if (table_name == table_to_hook)
			{
				for (int i = 0; i < client_class->m_pRecvTable->m_nProps; i++)
				{
					auto& prop = client_class->m_pRecvTable->m_pProps[i];
					std::string prop_name = prop.m_pVarName;

					if (prop_name == prop_to_hook)
					{
						prop.m_ProxyFn = callback;
					}
				}
			}

			client_class = client_class->m_pNext; // go to the next class
		}	
	}

	uintptr_t CNetvarHookManager::RecursivelyFindOffset(SDK::RecvTable* table, std::string table_name, std::string prop_name)
	{
		for (int i = 0; i < table->m_nProps; i++)
		{
			if (table_name == table->m_pNetTableName && prop_name == table->m_pProps[i].m_pVarName)
			{
				LOG(std::string("Successfully found offset ") + table->m_pNetTableName + " " + table->m_pProps[i].m_pVarName);
				return table->m_pProps[i].m_Offset;
			}

			if (!table->m_pProps[i].m_pDataTable)
				continue;

			auto return_value = RecursivelyFindOffset(table->m_pProps[i].m_pDataTable, table_name, prop_name);
			if (return_value)
				return return_value;
		}

		return 0;
	}

	uintptr_t CNetvarHookManager::GetOffset(std::string table, std::string prop)
	{
		auto client_class = INTERFACES::Client->GetAllClasses();

		// go through all classes
		while (client_class)
		{
			auto return_value = RecursivelyFindOffset(client_class->m_pRecvTable, table, prop);
			if (return_value)
				return return_value;

			client_class = client_class->m_pNext; // go to the next class
		}

		LOG("** Failed to get offset - " + prop);
		return 0;
	}
}