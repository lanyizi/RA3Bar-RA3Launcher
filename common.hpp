#pragma once

#include <cstddef>
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>
#include <locale>
#include "WindowsWrapper.hpp"
#include "CSFParser.hpp"
#include "ReplaysAndMods.hpp"

template<typename Predicate>
struct CaseInsensitiveCompare {
	bool operator()(wchar_t a, wchar_t b) const noexcept {
		return Predicate{}(std::tolower(a, std::locale::classic()), std::tolower(b, std::locale::classic()));
	};
};

template<typename InputIterator1, typename InputIterator2>
bool insensitiveEqual(InputIterator1 begin1, InputIterator1 end1,
                      InputIterator2 begin2, InputIterator2 end2) {
	return std::equal(begin1, end1, begin2, end2, CaseInsensitiveCompare<std::equal_to<>> {});
}

template<typename InputIterator1, typename InputIterator2>
InputIterator1 insensitiveSearch(InputIterator1 begin1, InputIterator1 end1,
                      InputIterator2 begin2, InputIterator2 end2) {
	return std::search(begin1, end1, begin2, end2, CaseInsensitiveCompare<std::equal_to<>> {});
}

struct StringLess {
	template<typename A, typename B>
	bool operator()(const A& a, const B& b) const noexcept {
		return std::lexicographical_compare(std::begin(a), std::end(a), std::begin(b), std::end(b),
		                                    CaseInsensitiveCompare<std::less<>> {});
	}
};

template<typename T>
void readFile(HANDLE file, std::vector<T>& buffer, std::size_t count) {
	buffer.resize(buffer.size() + count);
	auto bufferStart = buffer.data() + buffer.size() - count;
	auto totalBytesRead = typename std::vector<T>::size_type{0};
	while(totalBytesRead < count) {
		using namespace Windows;
		auto bytesRead = DWORD{};
		ReadFile(file, bufferStart + totalBytesRead, count - totalBytesRead, &bytesRead, nullptr)
		        >> checkWin32Result("ReadFile", errorValue, false);
		totalBytesRead += bytesRead;

		if(bytesRead == 0) {
			throw std::runtime_error("Read 0 bytes from file");
		}
	}
}

template<typename T>
std::vector<T> readFile(HANDLE file, std::size_t count) {
	auto buffer = std::vector<T> {};
	readFile(file, buffer, count);
	return buffer;
}

template<typename T>
std::vector<T> readEntireFile(HANDLE file) {
	auto buffer = std::vector<T> {};
	readFile(file, buffer, Windows::getFileSize(file));
	return buffer;
}

template<typename T>
void writeEntireFile(HANDLE file, const std::vector<T>& buffer) {
	auto totalBytesWritten = typename std::vector<T>::size_type{0};
	while(totalBytesWritten < buffer.size()) {
		using namespace Windows;
		auto bytesWritten = DWORD{};
		WriteFile(file, buffer.data() + totalBytesWritten, buffer.size() - totalBytesWritten, &bytesWritten, nullptr)
		        >> checkWin32Result("WriteFile", errorValue, false);
		totalBytesWritten += bytesWritten;

		if(bytesWritten == 0) {
			throw std::runtime_error("Wrote 0 bytes to file");
		}
	}
}

inline void fixReplayByFileName(const std::wstring& fileName) {
	using namespace Windows;
	using namespace ReplaysAndMods;
	auto fixedFileContent = std::vector<char> {};

	{
		auto file = createFile(fileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING);
		auto fileSize = getFileSize(file.get());
		const auto fileContent = readFile<char>(file.get(), fileSize);
		fixedFileContent = fixReplay(Input::Range{std::begin(fileContent), std::end(fileContent)});
	}

	auto outputName = fileName + L".RA3BARLAUNCHER_FIX_REPLAY" + replayExtension;

	auto backupFileName = fileName + L".original";
	while(fileExists(backupFileName + replayExtension)) {
		backupFileName += L'l';
	}
	backupFileName += replayExtension;

	{
		auto output = createFile(outputName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, CREATE_NEW);
		writeEntireFile(output.get(), fixedFileContent);
	}

	ReplaceFileW(fileName.c_str(), outputName.c_str(), backupFileName.c_str(), REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr)
	        >> checkWin32Result("ReplaceFileW", errorValue, 0);
}

inline Windows::RegistryKey getRa3RegistryKey(HKEY base, REGSAM access = KEY_READ) {
	return Windows::openRegistryKey(base, L"Software\\Electronic Arts\\Electronic Arts\\Red Alert 3", access | KEY_READ);
}

struct LanguageData {
	using ContainerType = std::map<std::wstring, std::wstring, StringLess>;
	std::wstring languageName;
	ContainerType table;
};

inline std::vector<std::wstring> getAllLanguages(const std::wstring& ra3Path) {
	using namespace Windows;
	auto csfs = findAllMatchingFiles(concatenatePath(ra3Path, L"Launcher\\*.csf"));
	for(auto& fileName : csfs) {
		constexpr auto csfExtentionSize = std::wstring_view{L".csf"}.size();
		fileName.erase(fileName.size() - csfExtentionSize);
	}
	return csfs;
}

inline std::vector<std::wstring> getSkudefs(const std::wstring& ra3Path, const std::wstring& languageName) {
	using namespace Windows;
	return findAllMatchingFiles(concatenatePath(ra3Path, L"ra3_" + languageName + L"_*.skudef"));
}

inline void setLanguageToRegistry(const std::wstring& language) {
	try {
		Windows::setRegistryString(getRa3RegistryKey(HKEY_CURRENT_USER, KEY_WRITE).get(), L"Language", language);
	}
	catch(...) { }
}

template<typename InputIterator>
LanguageData::ContainerType loadCSFStrings(InputIterator begin, InputIterator end) {
	auto emplacer = [](LanguageData::ContainerType& container, std::pair<std::string, std::wstring>&& newValue) {
		for(auto& character : newValue.first) {
			character = std::tolower(character, std::locale::classic());
		}
		container.emplace_hint(std::end(container), Windows::toWide(newValue.first), std::move(newValue.second));
	};
	return MyCSF::readCSF<LanguageData::ContainerType>(Input::Range{begin, end}, emplacer);
}

inline LanguageData loadLanguageData(const std::wstring& ra3Path, const std::wstring& languageName) {
	using namespace Windows;
	setLanguageToRegistry(languageName);
	auto fileName = concatenatePath(ra3Path, L"Launcher\\" + languageName + L".csf");
	auto csfContent = readEntireFile<char>(createFile(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING).get());
	auto strings = loadCSFStrings(std::begin(csfContent), std::end(csfContent));
	return {languageName, strings};
}

inline LanguageData loadPreferredLanguageData(const std::wstring& ra3Path) {
	auto language = std::wstring{L"english"};
	try {
		language = Windows::getRegistryString(getRa3RegistryKey(HKEY_CURRENT_USER).get(), L"Language");
	}
	catch(...) { }
	auto allLanguages = getAllLanguages(ra3Path);
	if(allLanguages.empty()) {
		throw std::runtime_error("Please install at least one language pack.");
	}
	if(auto position = std::find(std::begin(allLanguages), std::end(allLanguages), language);
	        position != std::end(allLanguages)) {
		return loadLanguageData(ra3Path, *position);
	}
	return loadLanguageData(ra3Path, allLanguages.front());
}

