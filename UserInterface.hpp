#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <optional>
#include "Common.hpp"

using LanguageList = std::vector<std::wstring>;

enum class SplashScreenResult : INT_PTR {
	normal,
	clicked,
};

struct LaunchOptions {
	enum LoadFileType {
		noFile,
		replay,
		mod,
	};
	LoadFileType loadFileType;
	std::wstring fileToBeLoaded;
	std::wstring extraCommandLine;
};

void displayErrorMessage(const std::exception& error, const LanguageData& languageData);
void notifyCantUpdate(const LanguageData& languageData);
void notifyIsNotReplay(const LanguageData& languageData);
bool askUserIfFixReplay(const std::wstring& replayName, const LanguageData& languageData);
void notifyFixReplaySucceeded(const LanguageData& languageData);
void notifyFixReplayFailed(const std::exception& errorMessage, const LanguageData& languageData);
void notifyNoCommentatorAvailable(const LanguageData& languageData);
void notifyGameVersionNotFound(const std::wstring& version, const LanguageData& languageData);
void notifyReplayModNotFound(const LanguageData& languageData);
void notifyReplayModAmbiguity(const LanguageData& languageData);
void notifyModNotFound(const LanguageData& languageData);

SplashScreenResult displaySplashScreen(const std::wstring& ra3Path, const LanguageData& languageData);
std::optional<LaunchOptions> runControlCenter(const std::wstring& ra3Path, const std::wstring& userCommandLine, LanguageData& languageData, HBITMAP customBackground);


