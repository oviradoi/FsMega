#pragma once

#include <memory>

#include "megaapi.h"

class Enumerator final
{
public:
	Enumerator(mega::MegaNodeList* nodeList);
	Enumerator(Enumerator&) = delete;
	Enumerator(Enumerator&&) = delete;
	Enumerator& operator=(const Enumerator&) = delete;
	Enumerator& operator=(const Enumerator&&) = delete;
	~Enumerator() = default;
	
	BOOL FindFirst(WIN32_FIND_DATAW* FindData);
	BOOL FindNext(WIN32_FIND_DATAW* FindData);

private:
	static void SetInfo(mega::MegaNode* node, WIN32_FIND_DATAW* FindData);
	static void UnixTimeToFileTime(time_t t, LPFILETIME pft);
	mega::MegaNode* GetNextNode();

private:
	std::unique_ptr<mega::MegaNodeList> _nodeList;
	int _idx;
};