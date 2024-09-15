#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	const wchar_t *T(const wchar_t *t);

	const wchar_t *I(int i);

	void SetTTable(const wchar_t *(*tbl)[6]);

	void LoadTTable(const wchar_t *fileName);

	void UnloadTTable();

#ifdef __cplusplus
}
#endif // __cplusplus

