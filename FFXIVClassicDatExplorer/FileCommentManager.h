#pragma once

#include <cstdint>
#include <string>

#include <map>

/**
 * @brief 管理从文件ID到文件用途注释的工具。
 *
 * 注释的风格是模仿文件路径的，将各类资源以类似路径的方式进行注释，例如：
 * music/bgm/Free.ogg <-> 0x12345678 (此仅为示例，非实际名称或映射)
 * maps/lanocia.gtex <-> 0x23456789 (此仅为示例，非实际名称或映射)
 * 这种注释方式有助于在工具中快速识别文件的用途和类型，尤其是在没有原始文件名的情况下。
 * 并且允许工具以树状图的方式展示文件结构，提升用户资源定位体验。
 */
class FileCommentManager
{
public:
	FileCommentManager();
	~FileCommentManager();

	/**
	 * @brief 获取指定文件ID的注释。
	 * @param fileId 文件ID
	 * @return 注释字符串，如果没有找到则返回空字符串。
	 */
	std::wstring GetComment(uint32_t fileId) const;

	/**
	 * @brief 获取给定注释的文件ID。
	 * @param comment 
	 * @return 
	 */
	uint32_t GetFileIdByComment(const std::wstring& comment) const;

	static FileCommentManager& GetInstance();
private:
	void LoadFromFile_();

	std::map<uint32_t, std::wstring> fileIdToComment_;
	std::map<std::wstring, uint32_t> commentToFileId_;
};

