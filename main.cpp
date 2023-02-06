#include <windows.h>
#include <cstring>
#include <vector>
#include <string>
#include "WindowsWrapper.hpp"
#include "UserInterface.hpp"
#include "ReplaysAndMods.hpp"
#include "resource.h"

std::wstring rebuildArgument(std::wstring_view argument) {
	auto result = std::wstring{};
	auto needQuotes = false;
	//[n (0 or more) slashes][quote] -> [n * 2 slashes][slash][quote]
	for(auto i = std::size_t{0}; i < argument.size(); ++i) {
		if(argument[i] == L'\\') {
			auto afterSlash = argument.find_first_not_of(L'\\', i);
			if(afterSlash < argument.size() and argument[afterSlash] == L'\"') {
				result += L'\\';
			}
		}
		else if(argument[i] == L'\"') {
			result += L'\\';
		}
		else if(argument[i] == L' ' or argument[i] == L'\t') {
			needQuotes = true;
		}
		result += argument[i];
	}

	if(needQuotes) {
		result = L'\"' + result + L'\"';
	}

	return result;
};

std::vector<std::wstring> getArguments(const std::wstring& commandLine) {
	if(commandLine.empty()) {
		return {};
	}

	using namespace Windows;

	auto count = 0;
	auto arguments = LocalMemory<LPWSTR[]> {
		CommandLineToArgvW(commandLine.c_str(), &count)
		        >> checkWin32Result("CommandLineToArgvW", errorValue, nullptr)
	};

	auto result = std::vector<std::wstring> {};
	for(auto i = 0; i < count; ++i) {
		result.emplace_back(arguments[i]);
	}

	return result;
}

std::wstring parseItemInCommandLine(const std::wstring& item) {
	using namespace Windows;
	auto count = 0;
	auto parsed = LocalMemory<LPWSTR[]> {
		CommandLineToArgvW(item.c_str(), &count)
		        >> checkWin32Result("CommandLineToArgvW", errorValue, nullptr)
	};
	if(count != 1) {
		throw std::runtime_error("parseItemInCommandLine CommandLineToArgvW -> count != 1");
	}
	return parsed[0];
}

enum class ExtractMode {
	extractName,
	extractValue,
};

enum class ActionAfterExtract {
	keepItem,
	removeItem,
};

template<typename Predicate>
std::optional<std::wstring> extractArgument(std::vector<std::wstring>& arguments, Predicate predicate,
        ExtractMode extractMode, ActionAfterExtract actionAfterExtract = ActionAfterExtract::keepItem) {

	auto position = std::find_if(std::begin(arguments), std::end(arguments), predicate);
	auto valuePosition = position;
	if(position == std::end(arguments)) {
		return std::nullopt;
	}

	if(extractMode == ExtractMode::extractValue) {
		valuePosition = std::next(valuePosition);
	}

	auto value = std::optional<std::wstring> {};

	if(valuePosition != std::end(arguments)) {
		value = *valuePosition;
	}

	if(actionAfterExtract == ActionAfterExtract::removeItem) {
		if(valuePosition != position and valuePosition != std::end(arguments)) {
			arguments.erase(valuePosition);
		}
		arguments.erase(position);
	}

	return value;
}

std::optional<int> parseInt(const std::optional<std::wstring>& string) {
	if(not string.has_value()) {
        return std::nullopt;
	}
	try {
        return std::stoi(string.value());
	}
    catch(...) {
        return std::nullopt;
    }
}

std::optional<std::wstring> getItemFromSkudef(const std::wstring& skudefPath, std::wstring_view itemName) {
	auto fileBuffer = readEntireFile<char>(Windows::createFile(skudefPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING).get());
	auto skudefContent = Windows::toWide({fileBuffer.data(), fileBuffer.size()});
	auto position = insensitiveSearch(std::begin(skudefContent), std::end(skudefContent),
	                                  std::begin(itemName), std::end(itemName)) - std::begin(skudefContent);
	if(static_cast<std::size_t>(position) >= skudefContent.size()) {
		return std::nullopt;
	}

	auto itemBegin = skudefContent.find_first_not_of(L' ',  position + itemName.size());
	auto itemEnd = skudefContent.find_first_of(L"\r\n", itemBegin);
	if(itemBegin >= skudefContent.size()) {
		return std::nullopt;
	}

	return skudefContent.substr(itemBegin, itemEnd - itemBegin);
}

HWND findRA3Window(HANDLE processHandle) {
	auto processIDAndWindow = std::pair{GetProcessId(processHandle), HWND{}};
	struct EnumWindowsCallback {
		static BOOL CALLBACK function(HWND window, LPARAM dataAddress) {
			auto processID = DWORD{};
			GetWindowThreadProcessId(window, &processID);
			auto& data = *reinterpret_cast<decltype(processIDAndWindow)*>(dataAddress);
			if(data.first != processID) {
				return TRUE;
			}
			if((GetWindow(window, GW_OWNER) != nullptr) or (not IsWindowVisible(window))) {
				return TRUE;
			}
            auto classNameBuffer = std::array<wchar_t, 256>{};
			using namespace std::literals;
			auto constexpr ra3ClassName = L"41DAF790-16F5-4881-8754-59FD8CF3B8D2"sv;
			if(not GetClassNameW(window, classNameBuffer.data(), classNameBuffer.size())) {
				return TRUE;
			}
			classNameBuffer[0] = ra3ClassName[0]; // 我的红警3魔改过（
			if(classNameBuffer.data() != ra3ClassName) {
				return TRUE;
			}
			// 这是红警3进程的窗口，完美匹配，不用继续找了
			data.second = window;
			return FALSE;
		}
	};
	EnumWindows(&EnumWindowsCallback::function, reinterpret_cast<LPARAM>(&processIDAndWindow));
	return processIDAndWindow.second;
}

int main() {

	//default language data,
	//gettext will use default embbed ra3bar - english language pack, if available
	auto languageData = LanguageData{};

	try {
		using namespace Windows;
		auto pathLength = GetCurrentDirectoryW(0, nullptr) >> checkWin32Result("GetCurrentDirectoryW", errorValue, 0);
		auto ra3Path = std::wstring{pathLength, L'\0', std::wstring::allocator_type{}};
		GetCurrentDirectoryW(ra3Path.size(), ra3Path.data()) >> checkWin32Result("GetCurrentDirectoryW", errorValue, 0);
		ra3Path.erase(std::min(ra3Path.size(), ra3Path.find(L'\0')));
		ra3Path.back() == L'\\' ? static_cast<void>(NULL) : ra3Path.push_back(L'\\');

		if(getSkudefs(ra3Path, L"*").empty()) {
			try {
				ra3Path = getRegistryString(getRa3RegistryKey(HKEY_LOCAL_MACHINE).get(), L"Install Dir");
			}
			catch(...) {
				MessageBoxW(nullptr, L"Game installation not found. You can try to put this program inside your RA3 folder.", nullptr, MB_TOPMOST|MB_ICONEXCLAMATION);
				return 1;
			}
			ra3Path.back() == L'\\' ? static_cast<void>(NULL) : ra3Path.push_back(L'\\');
		}

		languageData = loadPreferredLanguageData(ra3Path);

		auto runControlCenterWhenNeeded = [&ra3Path, &languageData](bool canRun, const std::wstring& userOptions, HBITMAP customBackground) {
			if(not canRun) {
				return std::optional<LaunchOptions> {};
			}
			return runControlCenter(ra3Path, userOptions, languageData, customBackground);
		};


		auto stringEqual = [](std::wstring_view a, std::wstring_view b) {
			return insensitiveEqual(std::begin(a), std::end(a), std::begin(b), std::end(b));
		};
		auto checkWithString = [stringEqual](std::wstring_view s) { return std::bind(stringEqual, s, std::placeholders::_1); };

		auto getInitialUserOptions = [runControlCenterWhenNeeded, checkWithString] {
			auto userOptions = std::wstring{GetCommandLineW()};
			auto initialArguments = getArguments(userOptions);
			userOptions.clear();
			for(auto i = std::size_t{1}; i < initialArguments.size(); ++i) {
				userOptions += L' ';
				userOptions += rebuildArgument(initialArguments[i]);
			}
			auto options = std::optional<LaunchOptions>{{LaunchOptions::noFile, {}, userOptions}};
			auto uiFlag = extractArgument(initialArguments, checkWithString(L"-ui"),
			                              ExtractMode::extractName, ActionAfterExtract::keepItem).has_value();
			if(uiFlag) {
				options = runControlCenterWhenNeeded(uiFlag, userOptions, nullptr);
			}
			return std::pair{std::move(options), uiFlag};
		};

		auto arbg = loadImage<BitmapHandle>(GetModuleHandle(nullptr), AR_BACKGROUND);
		auto customBackground = HBITMAP{nullptr};

		auto userOptions = std::wstring{};

		for(auto [options, uiFlag] = getInitialUserOptions();
		        options.has_value();
		        options = runControlCenterWhenNeeded(uiFlag, options.value().extraCommandLine, customBackground)) {
			customBackground = nullptr;

			const auto& [loadFileType, loadFilePath, extraCommandLine] = options.value();
			auto arguments = getArguments(extraCommandLine);

			auto getAndRemoveArgument = [&arguments](auto predicate, ExtractMode mode) {
				return extractArgument(arguments, predicate, mode, ActionAfterExtract::removeItem);
			};
			auto getArgument = [checkWithString, &arguments](std::wstring_view name, ExtractMode mode) {
				return extractArgument(arguments, checkWithString(name), mode, ActionAfterExtract::keepItem);
			};

			auto ev = getAndRemoveArgument(checkWithString(L"-runver"), ExtractMode::extractValue);
			auto em = getAndRemoveArgument(checkWithString(L"-modConfig"), ExtractMode::extractValue);
			auto er = getAndRemoveArgument(checkWithString(L"-replayGame"), ExtractMode::extractValue);
			auto ex = parseInt(getAndRemoveArgument(checkWithString(L"-x"), ExtractMode::extractValue));
			auto ey = parseInt(getAndRemoveArgument(checkWithString(L"-y"), ExtractMode::extractValue));
			auto et = getAndRemoveArgument(checkWithString(L"-allowAlwaysOnTop"), ExtractMode::extractName);
			auto windowed    = getArgument(L"-win", ExtractMode::extractName);
			auto fullscreen  = getArgument(L"-fullscreen", ExtractMode::extractName);
			auto resolutionX = parseInt(getArgument(L"-xres", ExtractMode::extractValue));
			auto resolutionY = parseInt(getArgument(L"-yres", ExtractMode::extractValue));
			if(not er.has_value()) {
				er = getAndRemoveArgument(checkWithString(L"-file"), ExtractMode::extractValue);
			}
			if(not er.has_value()) {
				auto predicate = [stringEqual](const std::wstring& argument) {
					namespace Replays = ReplaysAndMods;
					if(argument.size() < Replays::replayExtension.size()) {
						return false;
					}
					auto extensionBegin = argument.size() - Replays::replayExtension.size();
					if(not stringEqual(Replays::replayExtension, std::wstring_view{argument}.substr(extensionBegin))) {
						return false;
					}
					try {
						auto file = Windows::createFile(argument, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
						auto head = readFile<char>(file.get(), Replays::replayHeaderMagic.size());
						if(not std::equal(std::begin(head), std::end(head),
						                  std::begin(Replays::replayHeaderMagic), std::end(Replays::replayHeaderMagic))) {
							return false;
						}
					}
					catch(...) {
						return false;
					}
					return true;
				};
				er = extractArgument(arguments, predicate, ExtractMode::extractName, ActionAfterExtract::keepItem);
			}

			if(loadFileType == LaunchOptions::mod) {
				er = std::nullopt;
				em = loadFilePath;
			}

			if(loadFileType == LaunchOptions::replay) {
				ev = std::nullopt;
				em = std::nullopt;
				er = loadFilePath;
			}

			if(er.has_value()) {
				auto replayDetails = ReplaysAndMods::ReplayDetails{};
				try {
					replayDetails = ReplaysAndMods::getReplayDetails(er.value());
				}
				catch(...) {
					notifyIsNotReplay(languageData);
					continue;
				}

				auto [version, subVersion] = replayDetails.gameVersion;
				ev = std::to_wstring(version) + L"." + std::to_wstring(subVersion);

				em.reset();
				auto replayMods = ReplaysAndMods::getModSkudefPathsFromReplay(replayDetails);
				if(replayMods.has_value()) {
					if(replayMods->empty()) {
						notifyReplayModNotFound(languageData);
						continue;
					}
					else if(replayMods->size() > 1) {
						notifyReplayModAmbiguity(languageData);
						continue;
					}
					em = replayMods->front();
				}

				auto placeholder = std::optional<std::wstring> {std::nullopt};
				if(not replayDetails.finalTimeCode.has_value()) {
					std::swap(placeholder, er);
					if(askUserIfFixReplay(placeholder.value(), languageData)) {
						try {
							fixReplayByFileName(placeholder.value());
							std::swap(er, placeholder);
						}
						catch(const std::exception& error) {
							notifyFixReplayFailed(error, languageData);
						}
					}
				}
				if((er.has_value()) and (replayDetails.hasCommentator == false)) {
					std::swap(placeholder, er);
					notifyNoCommentatorAvailable(languageData);
				}
			}

			if(em.has_value()) {
				const auto& modSkudef = em.value();
				if(not fileExists(modSkudef)) {
					notifyModNotFound(languageData);
					continue;
				}

				if(auto gameVersionForMod = getItemFromSkudef(modSkudef, L"mod-game");
				        gameVersionForMod.has_value()) {
					ev = gameVersionForMod;
				}

				constexpr auto arString = std::wstring_view{L"Armor Rush"};
				if(insensitiveSearch(std::begin(modSkudef), std::end(modSkudef),
				                     std::begin(arString), std::end(arString)) != std::end(modSkudef)) {
					customBackground = arbg.get();
				}
			}

			auto allSkudefs = getSkudefs(ra3Path, languageData.languageName);
			auto skudefMax = std::max_element(std::begin(allSkudefs), std::end(allSkudefs), [](const std::wstring& a, const std::wstring& b) {
				auto versionA = a.substr(a.rfind(L'_') + 1);
				auto versionB = b.substr(b.rfind(L'_') + 1);
				auto subVersionA = versionA.substr(versionA.find(L'.') + 1);
				auto subVersionB = versionB.substr(versionB.find(L'.') + 1);
				using std::pair;
				return pair{parseInt(versionA), parseInt(subVersionA)} < pair{parseInt(versionB), parseInt(subVersionB)};
			});
			if(skudefMax == std::end(allSkudefs)) {
				notifyGameVersionNotFound(L"?", languageData);
				continue;
			}

			auto gameConfig = ra3Path + *skudefMax;
			if(ev.has_value()) {
				const auto& version = ev.value();
				auto skudef = std::find_if(std::begin(allSkudefs), std::end(allSkudefs), [&version](const std::wstring& fileName) {
					return fileName.find(version, fileName.rfind('_')) != fileName.npos;
				});
				if(skudef == std::end(allSkudefs)) {
					notifyGameVersionNotFound(version, languageData);
					continue;
				}
				gameConfig = ra3Path + *skudef;
			}

			auto configArgument = L" -config " + rebuildArgument(gameConfig);
			auto modArgument = std::wstring{};
			auto replayArgument = std::wstring{};
			if(em.has_value()) {
				modArgument = L" -modConfig " + rebuildArgument(em.value());
			}
			if(er.has_value()) {
				replayArgument = L" -replayGame " + rebuildArgument(er.value());
			}

			auto otherArguments = std::wstring{};
			for(const auto& argument : arguments) {
				otherArguments += L' ';
				otherArguments += rebuildArgument(argument);
			}

			auto gameExe = getItemFromSkudef(gameConfig, L"set-exe").value();

			if(displaySplashScreen(ra3Path, languageData) == SplashScreenResult::clicked) {
				uiFlag = true;
				continue;
			}

			auto finalArguments = ra3Path + gameExe + otherArguments + modArgument + replayArgument + configArgument;
			auto game = createProcess(finalArguments, ra3Path.c_str());
			// 假如游戏以窗口化模式启动，等待游戏窗口出现后，可以自动调整窗口位置
			if(windowed.has_value()) {
				constexpr auto interval = 499;
				while(WaitForSingleObject(game.get(), interval) == WAIT_TIMEOUT) {
					if(WaitForInputIdle(game.get(), 1) != 0) {
						continue;
					}
					auto gameWindow = findRA3Window(game.get());
					if(gameWindow == nullptr) {
						continue;
					}
					if(SendMessageTimeoutW(gameWindow, WM_NULL, 0, 0, SMTO_ERRORONEXIT, interval, nullptr) == 0) {
						continue;
					}
					auto rect = getWindowRect(gameWindow);

					// 在全屏窗口化模式下，自动居中窗口
					if(fullscreen.has_value()) {
						auto desktop = getWindowRect(GetDesktopWindow());
						if(resolutionX.has_value()) {
							rect.left = (rectWidth(desktop) - resolutionX.value()) / 2;
						}
						if(resolutionY.has_value()) {
							rect.top = (rectHeight(desktop) - resolutionY.value()) / 2;
						}
					}

					// 假如玩家在命令行参数里指定了窗口位置，就将窗口移动到指定位置
					if(ex.has_value()) {
						rect.left = ex.value();
					}
					if(ey.has_value()) {
						rect.top = ey.value();
					}

					auto zOrderFlag = et.has_value() ? SWP_NOZORDER : 0;
					SetWindowPos(gameWindow, HWND_NOTOPMOST, rect.left, rect.top, 0, 0, zOrderFlag|SWP_ASYNCWINDOWPOS|SWP_NOSIZE);
					break;
				}
			}

			// 等待游戏结束
			WaitForSingleObject(game.get(), INFINITE) >> checkWin32Result("WaitForSingleOnject", errorValue, WAIT_FAILED);
			auto exitCode = DWORD{0};
			GetExitCodeProcess(game.get(), &exitCode) >> checkWin32Result("GetExitCodeProcess", errorValue, false);
			if(exitCode == 123456789) {
				notifyCantUpdate(languageData);
			}
		}
	}
	catch(std::exception& exception) {
		displayErrorMessage(exception, languageData);
		return 1;
	}
	return 0;
}


