#include "pch.h"
#include "Enumerator.h"

using namespace std;
using namespace mega;

Enumerator::Enumerator(MegaNodeList* nodeList) : _nodeList(nodeList), _idx(0)
{
}

BOOL Enumerator::FindFirst(WIN32_FIND_DATAW* FindData)
{
	_idx = 0;
	MegaNode* node = GetNextNode();

	if (node != nullptr)
	{
		SetInfo(node, FindData);
		return TRUE;
	}

	return FALSE;
}

BOOL Enumerator::FindNext(WIN32_FIND_DATAW* FindData)
{
	MegaNode* node = GetNextNode();

	if (node != nullptr)
	{
		SetInfo(node, FindData);
		return TRUE;
	}
	
	return FALSE;
}

MegaNode* Enumerator::GetNextNode()
{
	MegaNode* nextNode = nullptr;

	for (; _idx < _nodeList->size(); _idx++)
	{
		MegaNode* node = _nodeList->get(_idx);
		if (node->getType() == MegaNode::TYPE_FILE || node->getType() == MegaNode::TYPE_FOLDER)
		{
			nextNode = node;
			_idx++;
			break;
		}
	}

	return nextNode;
}

void Enumerator::UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll = t * 10000000I64 + 116444736000000000I64;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

void Enumerator::SetInfo(mega::MegaNode* node, WIN32_FIND_DATAW* FindData)
{
	// Set name
	const char* name = node->getName();
	MultiByteToWideChar(CP_UTF8, 0, name, static_cast<int>(strlen(name)) + 1, FindData->cFileName, MAX_PATH);
	FindData->cAlternateFileName[0] = 0;

	// Set attributes
	FindData->dwFileAttributes = 0;
	if (node->isFolder())
	{
		FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	}

	// Set file size
	if (node->isFile())
	{
		const int64_t size = node->getSize();
		FindData->nFileSizeHigh = size >> 32;
		FindData->nFileSizeLow = size & 0xffffffff;
	}
	else
	{
		FindData->nFileSizeHigh = 0;
		FindData->nFileSizeLow = 0;
	}

	// Set creation and modification times
	UnixTimeToFileTime(node->getCreationTime(), &FindData->ftCreationTime);
	UnixTimeToFileTime(node->getModificationTime(), &FindData->ftLastWriteTime);
}
