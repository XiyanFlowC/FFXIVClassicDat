#include "FileCommentManager.h"

FileCommentManager::FileCommentManager()
{
	LoadFromFile_();
}

FileCommentManager::~FileCommentManager()
{
}

FileCommentManager &FileCommentManager::GetInstance()
{
	static FileCommentManager _inst;
	return _inst;
}

std::wstring FileCommentManager::GetComment(uint32_t fileId) const
{
	auto it = fileIdToComment_.find(fileId);
	if (it != fileIdToComment_.end())
		return it->second;
	return {};
}

uint32_t FileCommentManager::GetFileIdByComment(const std::wstring& comment) const
{
	auto it = commentToFileId_.find(comment);
	if (it != commentToFileId_.end())
		return it->second;
	return 0;
}

void FileCommentManager::LoadFromFile_()
{
	// TODO: load from a comments file when available
}


