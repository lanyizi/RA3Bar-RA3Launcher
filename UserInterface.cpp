//Everything related to user interface is inside this file...
#include <type_traits>
#include <utility>
#include <algorithm>
#include <tuple>
#include <array>
#include <vector>
#include <string>
#include <map>
#include <optional>
#include <cstdio>
#include <locale>
#include <Windows.h>
#include <CommCtrl.h>
#include <ShellApi.h>
#include "UserInterface.hpp"
#include "WindowsWrapper.hpp"
#include "ReplaysAndMods.hpp"
#include "resource.h"

using namespace Windows;
using namespace MyCSF;

enum ID  {
	errorMessage = 1,

	captionString,

	splashScreen,
	//buttons
	playGame,
	checkForUpdates,
	setLanguage,
	gameBrowser,
	readMe,
	visitEAWebsite,
	technicalSupport,
	deauthorize,
	quit,
	about,
	//
	commandLine,
	//set language window
	setLanguageDescription,
	setLanguageList,
	setLanguageOK,
	setLanguageCancel,
	//game browser window
	gameBrowserTabs,
	gameBrowserLaunchGame,
	gameBrowserCancel,
	//game browser mod window
	mods,
	modList,
	modListModName,
	modListModVersion,
	modFolder,
	//game browser replay window
	replays,
	replayList,
	replayListReplayName,
	replayListModName,
	replayListGameVersion,
	replayListDate,
	getReplayDescriptionBox,
	replayMatchInformation,
	replayMap,
	replayNumberOfPlayers,
	replayDescription,
	fixReplay,
	fixReplayWarning,
	fixReplaySucceeded,
	fixReplayFailed,
	replayFolder,
	//launcher / game browser launching game (replay)
	replayCantBePlayed,
	replayCantBeParsedText,
	replayDontHaveCommentator,
	replayNeedsToBeFixed,
	replayNeedsToBeFixedText,

	webSiteLink,
	eaSupportURL,

	useOriginalRA3Title,
	useOriginalRA3ToUpdate,
	useOriginalRA3ToDeauthorize,

	aboutText,
	resourceAuthors,

	gameVersionNotFound,
	noModsFound,
	multipleModsFound,
};

const std::wstring& getText(const LanguageData& data, ID id) {
	static auto labels = std::map<ID, std::wstring> {
		{errorMessage, L"RA3BarLauncher:ErrorMessage"},
		{captionString, L"Launcher:Caption"},
		{splashScreen, L"RA3BarLauncher:ClickSplashScreen"},
		{playGame, L"Launcher:Play"},
		{checkForUpdates, L"Launcher:CheckForUpdates"},
		{setLanguage, L"Launcher:SelectLanguage"},
		{gameBrowser, L"Launcher:ReplayBrowser"},
		{readMe, L"Launcher:Readme"},
		{visitEAWebsite, L"Launcher:Website"},
		{technicalSupport, L"Launcher:TechnicalSupport"},
		{deauthorize, L"Launcher:DeAuthorize"},
		{quit, L"Launcher:Quit"},
		{about, L"RA3BarLauncher:About"},
		{commandLine, L"RA3BarLauncher:CommandLine"},
		{setLanguageDescription, L"Launcher:SelectLanguageText"},
		{setLanguageOK, L"Dialog:OK"},
		{setLanguageCancel, L"Dialog:Cancel"},
		{gameBrowserLaunchGame, L"REPLAYBROWSER:WATCHREPLAY"},
		{gameBrowserCancel, L"Dialog:Cancel"},
		{mods, L"LAUNCHER:MODTAB"},
		{modListModName, L"MODBROWSER:NAMECOLUMN"},
		{modListModVersion, L"MODBROWSER:VERSIONCOLUMN"},
		{modFolder, L"RA3BarLauncher:OpenModFolder"},
		{replays, L"LAUNCHER:REPLAYTAB"},
		{replayListReplayName, L"REPLAYBROWSER:NAMECOLUMN"},
		{replayListModName, L"REPLAYBROWSER:MODCOLUMN"},
		{replayListGameVersion, L"REPLAYBROWSER:VERSIONCOLUMN"},
		{replayListDate, L"REPLAYBROWSER:DATECOLUMN"},
		{replayMatchInformation, L"REPLAYBROWSER:MATCHINFO"},
		{replayMap, L"REPLAYBROWSER:MAP"},
		{replayNumberOfPlayers, L"REPLAYBROWSER:NUMPLAYERS"},
		{replayDescription, L"REPLAYBROWSER:DESCRIPTION"},
		{fixReplay, L"RA3BarLauncher:FixReplay"},
		{fixReplayWarning, L"RA3BarLauncher:FixReplayWillReplaceOriginal"},
		{fixReplaySucceeded, L"RA3BarLauncher:FixReplaySuccess"},
		{fixReplayFailed, L"RA3BarLauncher:FixReplayFailure"},
		{replayFolder, L"RA3BarLauncher:OpenReplayFolder"},
		{webSiteLink, L"Launcher:URL"},
		{eaSupportURL, L"RA3BarLauncher:EASupportWebsite"},
		{useOriginalRA3Title, L"RA3BarLauncher:NeedOriginalLauncher"},
		{useOriginalRA3ToUpdate, L"RA3BarLauncher:UpdateNotSupported"},
		{useOriginalRA3ToDeauthorize, L"RA3BarLauncher:DeauthorizeNotSupported"},
		{replayCantBePlayed, L"RA3BarLauncher:ReplayCannotBePlayed"},
		{replayCantBeParsedText, L"RA3BarLauncher:ReplayCannotBeParsed"},
		{replayDontHaveCommentator, L"RA3BarLauncher:ReplayDoesNotHaveCommentator"},
		{replayNeedsToBeFixed, L"RA3BarLauncher:ReplayNeedsToBeFixed"},
		{replayNeedsToBeFixedText, L"RA3BarLauncher:ReplayNeedsToBeFixedText"},
		{aboutText, L"RA3BarLauncher:AboutText"},
		{resourceAuthors, L"RA3BarLauncher:ResourceAuthors"},
		{gameVersionNotFound, L"Launcher:CantFindVersionN_InstallPatches"},
		{noModsFound, L"REPLAYBROWSER:MODNOTINSTALLED"},
		{multipleModsFound, L"RA3BarLauncher:ReplayModAmbiguity"},
	};

	auto loadMyCSF = [](HMODULE module, LPCWSTR resource, LPCWSTR type) {
		auto data = loadBinaryDataResource<char>(module, resource, type);
		return loadCSFStrings(std::begin(data), std::end(data));
	};
	static auto myEnglish = loadMyCSF(GetModuleHandle(nullptr), MAKEINTRESOURCEW(BUILTIN_ENGLISH), RT_RCDATA);
	static auto myChinese = loadMyCSF(GetModuleHandle(nullptr), MAKEINTRESOURCEW(BUILTIN_CHINESE), RT_RCDATA);

	auto currentMyMap = &myEnglish;
	auto chinese = std::wstring_view{L"chinese"};
	if(insensitiveSearch(std::begin(data.languageName), std::end(data.languageName),
	                     std::begin(chinese), std::end(chinese)) != std::end(data.languageName)) {
		currentMyMap = &myChinese;
	}

	const auto& label = labels.at(id);

	auto string = data.table.find(label);
	if(string == data.table.end()) {
		string = currentMyMap->find(label);
		if(string == currentMyMap->end()) {
			static auto error = std::wstring{L"<ERROR NO CSFSTRING>"};
			return error;
		}
	}
	return string->second;
}

const std::wstring& getLanguageName(const LanguageData& data, const std::wstring& languageName) {

	auto languageTag = L"LanguageName:" + languageName;

	for(auto& character : languageTag) {
		character = std::tolower(character, std::locale::classic());
	}

	auto string = data.table.find(languageTag);
	if(string == data.table.end()) {
		static auto error = std::wstring{L"<ERROR LANGUAGE NAME>"};
		return error;
	}
	return string->second;
}

void displayErrorMessage(const std::exception& error, const LanguageData& languageData) {
	auto message = getText(languageData, errorMessage);
	message += L"\r\n";
	message += toWide(error.what());
	MessageBoxW(nullptr,
	            message.c_str(),
	            getText(languageData, captionString).c_str(),
	            MB_ICONERROR|MB_OK);
}

void notifyCantUpdate(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, useOriginalRA3ToUpdate).c_str(),
	            getText(languageData, useOriginalRA3Title).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}


void notifyIsNotReplay(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, replayCantBeParsedText).c_str(),
	            getText(languageData, replayCantBePlayed).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}

bool askUserIfFixReplay(const std::wstring& replayName, const LanguageData& languageData) {
	auto text = getText(languageData, replayNeedsToBeFixedText) + L"\r\n"
	            + getText(languageData, fixReplayWarning);
	auto answer = MessageBoxW(nullptr,
	                          text.c_str(),
	                          getText(languageData, replayCantBePlayed).c_str(),
	                          MB_ICONWARNING|MB_YESNO);
	return answer == IDYES;
}

void notifyFixReplaySucceeded(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, fixReplaySucceeded).c_str(),
	            getText(languageData, fixReplay).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}

void notifyFixReplayFailed(const std::exception& error, const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            (getText(languageData, fixReplayFailed) + L"\r\n" + toWide(error.what())).c_str(),
	            getText(languageData, fixReplay).c_str(),
	            MB_ICONERROR|MB_OK);
}

void notifyNoCommentatorAvailable(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, replayDontHaveCommentator).c_str(),
	            getText(languageData, replayCantBePlayed).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}

void notifyGameVersionNotFound(const std::wstring& gameVersion, const LanguageData& languageData) {
	constexpr auto placeholder = std::wstring_view{L"%s"};
	auto message = getText(languageData, gameVersionNotFound);
	message.replace(message.find(placeholder), placeholder.size(), gameVersion);
	MessageBoxW(nullptr, message.c_str(), getText(languageData, captionString).c_str(), MB_ICONERROR|MB_OK);
}

void notifyModNotFound(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, noModsFound).c_str(),
	            getText(languageData, captionString).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}
void notifyReplayModNotFound(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, noModsFound).c_str(),
	            getText(languageData, replayCantBePlayed).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}
void notifyReplayModAmbiguity(const LanguageData& languageData) {
	MessageBoxW(nullptr,
	            getText(languageData, multipleModsFound).c_str(),
	            getText(languageData, replayCantBePlayed).c_str(),
	            MB_ICONINFORMATION|MB_OK);
}

DWORD colorMultiply (DWORD color, std::uint8_t multiplier) {
	auto result = DWORD{};
	for(auto i = 0; i < 3; ++i) {
		auto component = ((color >> (i * 8)) bitand 0xFFu) / static_cast<double>(0xFFu);
		auto finalComponent = static_cast<std::uint8_t>(component * multiplier);
		result = result bitor (finalComponent << (i * 8));
	}
	return result;
}

LanguageData getNewLanguage(HWND controlCenter, const std::wstring& ra3Path, HICON icon, const LanguageData& languageData);
std::optional<LaunchOptions> runGameBrowser(HWND controlCenter, const std::wstring& ra3Path, HICON icon, const LanguageData& languageData);
void aboutWindow(HWND parent, HICON icon, const LanguageData& languageData);

void centerDialogBox (HWND parent, HWND dialogBox, UINT additionalFlags = 0) {
	auto parentRect = getWindowRect(parent);
	auto dialogBoxRect = getWindowRect(dialogBox);
	auto [left, top, right, bottom] = getWindowRect(GetDesktopWindow());

	auto halfWidth = (rectWidth(parentRect) - rectWidth(dialogBoxRect)) / 2;
	auto halfHeight = (rectHeight(parentRect) - rectHeight(dialogBoxRect)) / 2;
	auto [x, y] = std::pair{parentRect.left + halfWidth, parentRect.top + halfHeight};

	if(x < left) x = left;
	if(y + rectHeight(dialogBoxRect) > bottom) y = bottom - rectHeight(dialogBoxRect);
	if(x + rectWidth(dialogBoxRect) > right)  x = right - rectWidth(dialogBoxRect);
	if(y < top) y = top;

	SetWindowPos(dialogBox, nullptr, x, y, rectWidth(dialogBoxRect), rectHeight(dialogBoxRect),
	             SWP_NOZORDER|additionalFlags) >> checkWin32Result("SetWindowPos", errorValue, false);
}

void myBeginDialog(HWND dialogBox, const std::wstring& title, HICON icon, HWND center, DWORD commonControls, RECT clientArea) {
	auto controlsToBeInitialized = INITCOMMONCONTROLSEX{ sizeof(INITCOMMONCONTROLSEX), commonControls };
	InitCommonControlsEx(&controlsToBeInitialized) >> checkWin32Result("InitCommonControlsEx", errorValue, FALSE);

	auto realSize = setWindowRectByClientRect(dialogBox, rectWidth(clientArea), rectHeight(clientArea));
	SetWindowPos(dialogBox, nullptr, 0, 0, rectWidth(realSize), rectHeight(realSize), SWP_NOMOVE|SWP_NOZORDER)
	        >> checkWin32Result("SetWindowPos", errorValue, false);

	centerDialogBox(center, dialogBox);

	SetWindowTextW(dialogBox, title.c_str()) >> checkWin32Result("SetWindowTextW", errorValue, false);

	SendMessageW(dialogBox, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
	SendMessageW(dialogBox, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
}

FontHandle getNormalFont(std::optional<int> fontHeight = std::nullopt) {
	auto metrics = NONCLIENTMETRICSW{0};
	metrics.cbSize = sizeof(NONCLIENTMETRICSW);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &metrics, 0)
	        >> checkWin32Result("SystemParametersInfoW", errorValue, FALSE);
	auto& logicalFont = metrics.lfMessageFont;
	if(fontHeight.has_value()) {
		logicalFont.lfHeight = fontHeight.value();
		logicalFont.lfWidth = 0;
	}
	return createFontIndirect(logicalFont);
}

int getRealFontHeight(HDC deviceContext, HFONT font) {
	auto textMetrics = TEXTMETRIC{};
	auto previouslySelected = selectObject(deviceContext, font);
	GetTextMetrics(deviceContext, &textMetrics) >> checkWin32Result("GetTextMetrics", errorValue, false);
	return textMetrics.tmHeight;
}

FontHandle getAdjustedNormalFont(HDC deviceContext, int height) {
	auto adjustedHeight = height * height / getRealFontHeight(deviceContext, getNormalFont(height).get());
	return getNormalFont(adjustedHeight);
}

BitmapHandle loadImageFromLauncherPath(const std::wstring& ra3Path, std::wstring_view item, const RECT& imageRect) {
	return loadImage<BitmapHandle>((ra3Path + L"Launcher\\").append(item.data(), item.size()), {rectWidth(imageRect), rectHeight(imageRect)});
};

template<typename ValueGetter, typename... Alternatives>
typename std::conditional_t<std::is_invocable_v<ValueGetter>,
         std::invoke_result<ValueGetter>,
         std::enable_if<not std::is_invocable_v<ValueGetter>, ValueGetter>
         >::type
tryWith(ValueGetter&& valueGetter, Alternatives&&... alternatives) {
	try {
		if constexpr(std::is_invocable_v<ValueGetter>) {
			return valueGetter();
		}
		else {
			return std::forward<ValueGetter>(valueGetter);
		}
	}
	catch(...) {
		if constexpr(sizeof...(alternatives) == 0) {
			throw std::runtime_error("Alternatives exhausted.");
		}
		else {
			return tryWith(std::forward<Alternatives>(alternatives)...);
		}
	}
}

SplashScreenResult displaySplashScreen(const std::wstring& ra3Path, const LanguageData& languageData) {
	static constexpr auto splashRect = RECT{0, 0, 640, 480};
	static constexpr auto noticeBarRect = RECT{
		0,
		rectHeight(splashRect) * 75 / 100,
		rectWidth(splashRect),
		rectHeight(splashRect) * 80 / 100
	};
	constexpr auto timerInterval = 30;
	constexpr auto velocity = 1;
	constexpr auto lifeSpan = rectWidth(splashRect) * 3;
	constexpr auto virtualWidth = rectWidth(splashRect) * 2;

	auto icon = tryWith([&ra3Path] { return loadImage<IconHandle>(ra3Path + L"ra3.ico"); },
	                    [] { return loadImage<IconHandle>(GetModuleHandle(nullptr), DEFAULT_ICON); });
	auto backgroundBrush = BrushHandle{nullptr};
	auto backgroundBrushWithText = BrushHandle{nullptr};
	auto lastTickCount = GetTickCount();
	auto edge = LONG{0};

	auto handlers = ModalDialogBox::HandlerTable{};

	auto initializeBrushes = [&ra3Path, &languageData, &backgroundBrush, &backgroundBrushWithText](HDC deviceContext) {
		using std::bind;
		using std::ref;
		auto backgroundLoader = bind(loadImageFromLauncherPath, ref(ra3Path), std::placeholders::_1, ref(splashRect));
		auto background = tryWith(bind(backgroundLoader, languageData.languageName + L"_splash.bmp"),
		                          bind(backgroundLoader, L"splash.bmp"),
		                          createCompatibleBitmap(deviceContext, rectWidth(splashRect), rectHeight(splashRect)));
		backgroundBrush = createPatternBrush(background.get());

		auto bitmapBits = getBitmapBits(deviceContext, background.get(), {{rectWidth(splashRect), -rectHeight(splashRect)}});
		for(auto y = noticeBarRect.top; y < noticeBarRect.bottom; ++y) {
			auto row = y * rectWidth(splashRect);
			for(auto column = noticeBarRect.left; column < noticeBarRect.right; ++column) {
				auto& pixel = bitmapBits.buffer.at(row + column);
				pixel = colorMultiply(pixel, 0x7F);
			}
		}
		setBitmapBits(deviceContext, background.get(), bitmapBits);

		{
			auto compatibleContext = createCompatibleDeviceContext(deviceContext);
			auto previousBitmap = selectObject(compatibleContext.get(), background.get());
			auto font = getAdjustedNormalFont(compatibleContext.get(), rectHeight(noticeBarRect));
			auto previousFont = selectObject(compatibleContext.get(), font.get());
			auto previousMode = setBackgroundMode(compatibleContext.get(), TRANSPARENT);
			auto previousTextColor = setTextColor(compatibleContext.get(), RGB(255, 255, 255));
			auto textRect = noticeBarRect;
			DrawTextW(compatibleContext.get(), getText(languageData, splashScreen).c_str(), -1, &textRect, DT_CENTER)
			        >> checkWin32Result("DrawTextW", errorValue, 0);
		}

		backgroundBrushWithText = createPatternBrush(background.get());
	};

	handlers[WM_INITDIALOG] = [&languageData, &icon, &lastTickCount, initializeBrushes](HWND window, WPARAM, LPARAM) {
		myBeginDialog(window, getText(languageData, captionString), icon.get(),
		              GetDesktopWindow(), ICC_STANDARD_CLASSES, splashRect);
		initializeBrushes(getDeviceContext(window)->context);

		SetTimer(window, splashScreen, timerInterval, nullptr) >> checkWin32Result("SetTimer", errorValue, false);
		return TRUE;
	};

	handlers[WM_TIMER] = [&lastTickCount, &edge](HWND window, WPARAM timerID, LPARAM) {
		if(timerID == splashScreen) {
			auto oldTickCount = lastTickCount;
			lastTickCount = GetTickCount();
			edge += (lastTickCount - oldTickCount) * velocity;
			if(edge > lifeSpan) {
				KillTimer(window, timerID) >> checkWin32Result("KillTimer", errorValue, false);
				EndDialog(window, static_cast<INT_PTR>(SplashScreenResult::normal)) >> checkWin32Result("EndDialog", errorValue, false);
			}
			InvalidateRect(window, &noticeBarRect, false) >> checkWin32Result("InvalidateRect", errorValue, false);
			return TRUE;
		}
		return FALSE;
	};

	handlers[WM_LBUTTONDOWN] = [](HWND window, WPARAM, LPARAM) {
		EndDialog(window, static_cast<INT_PTR>(SplashScreenResult::clicked)) >> checkWin32Result("EndDialog", errorValue, false);
		return TRUE;
	};

	auto drawer = [&backgroundBrush, &backgroundBrushWithText, &edge](HDC context, const RECT& dirtyRect) {
		auto textRect = dirtyRect;
		textRect.left = std::clamp(edge, dirtyRect.left, dirtyRect.right);
		textRect.right = std::clamp(edge - virtualWidth, dirtyRect.left, dirtyRect.right);
		auto remainedLeft = dirtyRect;
		auto remainedRight = dirtyRect;
		remainedLeft.right = textRect.left;
		remainedRight.left = textRect.right;

		FillRect(context, &remainedLeft, backgroundBrush.get()) >> checkWin32Result("FillRect", errorValue, false);
		FillRect(context, &remainedRight, backgroundBrush.get()) >> checkWin32Result("FillRect", errorValue, false);
		FillRect(context, &textRect, backgroundBrushWithText.get()) >> checkWin32Result("FillRect", errorValue, false);
	};

	handlers[WM_PAINT] = [drawer](HWND window, WPARAM, LPARAM) {
		auto paint = beginPaint(window);
		auto dirtyRect = paint->paintStruct.rcPaint;
		if(paint->paintStruct.fErase) {
			dirtyRect = splashRect;
		}
		auto memoryContext = createCompatibleDeviceContext(paint->context);
		auto compatibleBitmap = createCompatibleBitmap(paint->context, rectWidth(splashRect), rectHeight(splashRect));
		auto previousMemoryBitmap = selectObject(memoryContext.get(), compatibleBitmap.get());

		drawer(memoryContext.get(), dirtyRect);
		auto [x, y, width, height] = std::tuple{dirtyRect.left, dirtyRect.top, rectWidth(dirtyRect), rectHeight(dirtyRect)};
		BitBlt(paint->context, x, y, width, height, memoryContext.get(), x, y, SRCCOPY)
		        >> checkWin32Result("BitBlt", errorValue, false);
		return TRUE;
	};

	return static_cast<SplashScreenResult>(modalDialogBox(handlers, WS_VISIBLE|WS_POPUP, 0));
}

std::optional<LaunchOptions> runControlCenter(const std::wstring& ra3Path, const std::wstring& userCommandLine, LanguageData& languageData, HBITMAP customBackground) {

	constexpr auto firstLineStart = 67;
	constexpr auto secondLineStart = 139;

	constexpr auto commandLineY = 470;
	constexpr auto commandLineWidth = 800 - 2 * firstLineStart;
	constexpr auto commandLineHeight = 20;
	constexpr auto commandLineTitleY = commandLineY - commandLineHeight - 2;

	constexpr auto buttonWidth = 121;
	constexpr auto buttonHeight = 34;
	constexpr auto buttonPadding = 12;

	static constexpr auto clientArea = RECT{0, 0, 800, 600};
	static constexpr auto allButtons = {
		playGame, checkForUpdates, setLanguage,	gameBrowser, readMe,
		visitEAWebsite, technicalSupport, deauthorize, quit, about,
	};

	auto icon = tryWith([&ra3Path] { return loadImage<IconHandle>(ra3Path + L"ra3.ico"); },
	                    [] { return loadImage<IconHandle>(GetModuleHandle(nullptr), DEFAULT_ICON); });

	auto backgroundBrushes = std::array<BrushHandle, 2> {};
	const auto& [mainBackground, commandLineBackground] = backgroundBrushes;

	auto buttonBrushes = std::array<BrushHandle, 3> {};
	auto buttonFont = getNormalFont();
	constexpr auto buttonTextFormats = DT_CENTER|DT_SINGLELINE|DT_VCENTER;
	auto commandLineFont = getNormalFont(commandLineHeight);

	auto handlers = ModalDialogBox::HandlerTable{};
	auto returnValue = std::optional<LaunchOptions> {};

	auto setUpBrushes = [&ra3Path, &languageData, customBackground, &backgroundBrushes, &buttonBrushes](HDC deviceContext) {
		using std::bind;
		using std::ref;
		auto backgroundLoader = bind(loadImageFromLauncherPath, ref(ra3Path), std::placeholders::_1, ref(clientArea));
		auto background = tryWith(bind(backgroundLoader, languageData.languageName + L"_cnc.bmp"),
		                          bind(backgroundLoader, L"cnc.bmp"),
		                          createCompatibleBitmap(deviceContext, rectWidth(clientArea), rectHeight(clientArea)));
		auto rawBackground = customBackground ? customBackground : background.get();
		backgroundBrushes[0] = createPatternBrush(rawBackground);

		auto bitmapBits = getBitmapBits(deviceContext, rawBackground, {{rectWidth(clientArea), -rectHeight(clientArea)}});
		for(auto y = commandLineY; y < commandLineY + commandLineHeight; ++y) {
			auto row = y * rectWidth(clientArea);
			for(auto column = firstLineStart; column < firstLineStart + commandLineWidth; ++column) {
				auto sp = (y - commandLineY) * rectWidth(clientArea) + (column - firstLineStart);
				bitmapBits.buffer.at(sp) = colorMultiply(bitmapBits.buffer.at(row + column), 150);
			}
		}
		auto duplicate = createCompatibleBitmap(deviceContext, rectWidth(clientArea), rectHeight(clientArea));
		setBitmapBits(deviceContext, duplicate.get(), bitmapBits);
		backgroundBrushes[1] = createPatternBrush(duplicate.get());

		auto maxDistance = 10;
		auto greyButton = std::vector<std::uint8_t> {};
		greyButton.resize(buttonWidth * buttonHeight);
		for(auto y = 0; y < buttonHeight; ++y) {
			auto row = y * buttonWidth;
			for(auto column = 0; column < buttonWidth; ++column) {
				auto [min, max] = std::minmax({std::min<double>(column, buttonWidth - column), std::min<double>(y, buttonHeight - y)});
				min = std::clamp(min / maxDistance, 0.0, 1.0);
				max = std::clamp(max / maxDistance, 0.0, 1.0);
				auto value = std::clamp(min + (2.0 - max) * max - 1.0, 0.0, 1.0);
				greyButton.at(row + column) = std::clamp<int>(255 * ((2.0 * value - 3.0) * value * value + 1.0), 0, 255);
			}
		}

		auto getButtonBrush = [deviceContext, &greyButton](DWORD backgroundColor, DWORD borderColor) {
			auto buttonBitmap = createCompatibleBitmap(deviceContext, buttonWidth, buttonHeight);
			auto bitmapBits = getBitmapBits(deviceContext, buttonBitmap.get());
			for(auto i = std::size_t{0}; i < bitmapBits.buffer.size(); ++i) {
				auto& color = bitmapBits.buffer.at(i);
				auto horizontalBorder = (i < buttonWidth) or (i / buttonWidth == buttonHeight - 1);
				auto verticalBorder = (i % buttonWidth == 0) or (i % buttonWidth == buttonWidth - 1);

				if(verticalBorder and horizontalBorder) {
					color = colorMultiply(borderColor, 0x7F);
				}
				else if(verticalBorder or horizontalBorder) {
					color = colorMultiply(borderColor, 0xFF);
				}
				else {
					color = colorMultiply(backgroundColor, greyButton.at(i));
				}
			}
			setBitmapBits(deviceContext, buttonBitmap.get(), bitmapBits);
			return createPatternBrush(buttonBitmap.get());
		};

		buttonBrushes[0] = getButtonBrush(RGB(12, 29, 129), RGB(0, 118, 230));
		buttonBrushes[1] = getButtonBrush(RGB(0, 230, 222), RGB(230, 230, 230));
		buttonBrushes[2] = getButtonBrush(RGB(0, 157, 230), RGB(0, 118, 230));
	};

	auto initializeCommandLineWindow = [&userCommandLine, &commandLineFont](HWND window) {
		auto commandLineWindow = createControl(window, commandLine, WC_EDITW, userCommandLine.c_str(),
		                                       WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, WS_EX_TRANSPARENT,
		                                       firstLineStart, commandLineY, commandLineWidth, commandLineHeight).release();
		commandLineFont = getAdjustedNormalFont(getDeviceContext(window)->context, commandLineHeight);
		SendMessageW(commandLineWindow, WM_SETFONT, reinterpret_cast<WPARAM>(commandLineFont.get()), true);
	};

	auto adjustButtonFont = [&languageData, &buttonFont](HWND window) {
		auto getTextRect = [&buttonFont](HDC deviceContext, std::wstring_view text) {
			auto textRect = RECT{0, 0, buttonWidth, buttonHeight};
			auto previousFont = selectObject(deviceContext, buttonFont.get());
			DrawTextW(deviceContext, text.data(), text.size(), &textRect, buttonTextFormats|DT_CALCRECT)
			        >> checkWin32Result("DrawText", errorValue, 0);
			return textRect;
		};

		buttonFont = getNormalFont();
		for(auto id : allButtons) {
			auto button = getControlByID(window, id);
			InvalidateRect(button, nullptr, false) >> checkWin32Result("InvalidateRect", errorValue, false);

			auto context = getDeviceContext(button);
			const auto& text = getText(languageData, id);
			auto textRect = getTextRect(context->context, text);
			if(rectWidth(textRect) <= buttonWidth) {
				continue;
			}
			auto realHeight = getRealFontHeight(context->context, buttonFont.get());
			auto adjustedHeight = realHeight * buttonWidth / rectWidth(textRect);
			buttonFont = getAdjustedNormalFont(context->context, adjustedHeight);
		}
	};

	handlers[WM_INITDIALOG] = [&languageData, &icon, setUpBrushes, initializeCommandLineWindow, adjustButtonFont](HWND window, WPARAM, LPARAM) {
		myBeginDialog(window, getText(languageData, captionString), icon.get(),
		              GetDesktopWindow(), ICC_STANDARD_CLASSES, clientArea);

		setUpBrushes(getDeviceContext(window)->context);

		initializeCommandLineWindow(window);

		auto currentX = 0;
		auto nextX = [&currentX] { return currentX += buttonWidth + buttonPadding; };

		currentX = firstLineStart - (buttonWidth + buttonPadding);
		for(auto id : {playGame, checkForUpdates, setLanguage, gameBrowser, readMe}) {
			createControl(window, id, WC_BUTTONW, {}, WS_VISIBLE, 0,
			              nextX(), 520, buttonWidth, buttonHeight).release();
		}

		currentX = secondLineStart - (buttonWidth + buttonPadding);
		for(auto id : {visitEAWebsite, technicalSupport, deauthorize, quit}) {
			createControl(window, id, WC_BUTTONW, {}, WS_VISIBLE, 0,
			              nextX(), 562, buttonWidth, buttonHeight).release();
		}

		createControl(window, about, WC_BUTTONW, {}, WS_VISIBLE, 0,
		              clientArea.right - buttonWidth - buttonPadding, clientArea.top + buttonPadding,
		              buttonWidth, buttonHeight).release();

		adjustButtonFont(window);
		return TRUE;
	};

	handlers[WM_CTLCOLOREDIT] = [&commandLineBackground](HWND window, WPARAM hdcAddress, LPARAM childAddress) -> INT_PTR {
		auto childWindow = reinterpret_cast<HWND>(childAddress);
		if(getControlID(childWindow) != commandLine) {
			return FALSE;
		}
		auto childDC = reinterpret_cast<HDC>(hdcAddress);
		setBackgroundMode(childDC, TRANSPARENT).release();
		setTextColor(childDC, RGB(255, 255, 255)).release();
		return reinterpret_cast<INT_PTR>(commandLineBackground.get()); //per https://msdn.microsoft.com/en-us/library/windows/desktop/bb761691(v=vs.85).aspx
	};


	handlers[WM_PAINT] = [&languageData, &mainBackground, &commandLineFont](HWND window, WPARAM, LPARAM) {
		auto paint = beginPaint(window);
		auto previousMode = setBackgroundMode(paint->context, TRANSPARENT);
		auto previousTextColor = setTextColor(paint->context, RGB(255, 255, 255));
		auto previouslySelected = selectObject(paint->context, commandLineFont.get());

		FillRect(paint->context, &paint->paintStruct.rcPaint, mainBackground.get()) >> checkWin32Result("FillRect", errorValue, false);
		TextOutW(paint->context, firstLineStart, commandLineTitleY,
		         getText(languageData, commandLine).c_str(), getText(languageData, commandLine).size()) >> checkWin32Result("TextOut", errorValue, false);
		return TRUE;
	};

	auto getCommandLines = [](HWND controlCenter) {
		auto commandLineWindow = getControlByID(controlCenter, commandLine);
		auto length = GetWindowTextLengthW(commandLineWindow);
		auto windowString = std::wstring{static_cast<std::size_t>(length + 1), L'\0', std::wstring::allocator_type{}};
		auto realLength = GetWindowTextW(commandLineWindow, windowString.data(), length + 1);
		windowString.resize(realLength);
		return windowString;
	};

	auto endControlCenter = [&returnValue](HWND window, std::optional<LaunchOptions> launchOptions) {
		returnValue = std::move(launchOptions);
		EndDialog(window, 0) >> checkWin32Result("EndDialog", errorValue, false);
	};

	handlers[WM_COMMAND] = [&ra3Path, &languageData, &icon, setUpBrushes, adjustButtonFont, getCommandLines, endControlCenter](HWND window, WPARAM codeAndIdentifier, LPARAM childWindow) {
		auto notificationCode = HIWORD(codeAndIdentifier);
		auto identifier = LOWORD(codeAndIdentifier);
		if(notificationCode != BN_CLICKED) {
			return FALSE;
		}
		switch(identifier) {
			case playGame: {
				endControlCenter(window, LaunchOptions{LaunchOptions::noFile, {}, getCommandLines(window)});
				break;
			}
			case checkForUpdates: {
				notifyCantUpdate(languageData);
				break;
			}
			case setLanguage: {
				languageData = getNewLanguage(window, ra3Path, icon.get(), languageData);
				SetWindowTextW(window, getText(languageData, captionString).c_str()) >> checkWin32Result("SetWindowTextW", successValue, true);
				setUpBrushes(getDeviceContext(window)->context);
				adjustButtonFont(window);
				InvalidateRect(window, nullptr, false) >> checkWin32Result("InvalidateRect", errorValue, false);
				break;
			}
			case gameBrowser: {
				auto launchOptions = runGameBrowser(window, ra3Path, icon.get(), languageData);
				if(launchOptions.has_value()) {
					launchOptions->extraCommandLine = getCommandLines(window);
					endControlCenter(window, std::move(launchOptions));
				}
				break;
			}
			case readMe: {
				try {
					shellExecute(window, L"open", getRegistryString(getRa3RegistryKey(HKEY_LOCAL_MACHINE).get(), L"Readme"));
				}
				catch(...) { }
				break;
			}
			case visitEAWebsite: {
				shellExecute(window, L"open", getText(languageData, webSiteLink));
				break;
			}
			case technicalSupport: {
				shellExecute(window, L"open", getText(languageData, eaSupportURL));
				break;
			}
			case deauthorize: {
				MessageBoxW(window,
				            getText(languageData, useOriginalRA3ToDeauthorize).c_str(),
				            getText(languageData, useOriginalRA3Title).c_str(),
				            MB_ICONINFORMATION|MB_OK);
				break;
			}
			case about: {
				aboutWindow(window, icon.get(), languageData);
				break;
			}
			case quit: {
				endControlCenter(window, std::nullopt);
				break;
			}
		}
		return TRUE;

	};



	handlers[WM_NOTIFY] = [&languageData, &buttonBrushes, &buttonFont](HWND window, WPARAM, LPARAM dataHeaderAddress) {
		const auto& dataHeader = *reinterpret_cast<const NMHDR*>(dataHeaderAddress);
		auto id = static_cast<ID>(dataHeader.idFrom);

		if(std::count(std::begin(allButtons), std::end(allButtons), id) == 0) {
			return FALSE;
		}

		if(dataHeader.code != NM_CUSTOMDRAW) {
			return FALSE;
		}

		const auto& customDraw = *reinterpret_cast<const NMCUSTOMDRAW*>(dataHeaderAddress);

		if(customDraw.dwDrawStage != CDDS_PREPAINT) {
			return FALSE;
		}

		const auto& buttonText = getText(languageData, id);

		auto previousMode = setBackgroundMode(customDraw.hdc, TRANSPARENT);
		auto previousColor = setTextColor(customDraw.hdc, RGB(255, 255, 255));

		auto buttonState = SendMessageW(dataHeader.hwndFrom, BM_GETSTATE, 0, 0);
		auto brush = HBRUSH{nullptr};
		if(buttonState bitand BST_PUSHED) {
			brush = buttonBrushes[2].get();
		}
		else if(buttonState bitand BST_HOT) {
			brush = buttonBrushes[1].get();
		}
		else {
			brush = buttonBrushes[0].get();
		}

		auto buttonRect = getClientRect(dataHeader.hwndFrom);

		if(brush != nullptr) {
			FillRect(customDraw.hdc, &buttonRect, brush) >> checkWin32Result("FillRect", errorValue, false);
		}

		auto previousFont = selectObject(customDraw.hdc, buttonFont.get());
		DrawTextW(customDraw.hdc, buttonText.c_str(), buttonText.size(), &buttonRect, buttonTextFormats);
		return CDRF_SKIPDEFAULT;
	};

	handlers[WM_CLOSE] = [endControlCenter](HWND window, WPARAM, LPARAM) {
		endControlCenter(window, std::nullopt);
		return TRUE;
	};

	modalDialogBox(handlers, WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX, WS_EX_OVERLAPPEDWINDOW);
	return returnValue;
}

LanguageData getNewLanguage(HWND controlCenter, const std::wstring& ra3Path, HICON icon, const LanguageData& languageData) {

	constexpr auto buttonWidth = 100;
	constexpr auto buttonHeight = 30;
	constexpr auto buttonPadding = 10;

	static constexpr auto bannerRect = RECT{0, 0, 480, 100};
	constexpr auto clientArea = RECT{0, 0, rectWidth(bannerRect), rectHeight(bannerRect) + 3 * buttonHeight + 2 * buttonPadding};

	constexpr auto textX = buttonPadding;
	constexpr auto textY = bannerRect.bottom;
	constexpr auto textWidth = rectWidth(clientArea) - 2 * textX;
	constexpr auto textHeight = buttonHeight;

	constexpr auto listWidth = rectWidth(clientArea) * 0.75;
	constexpr auto listHeight = buttonHeight;
	constexpr auto listX = (rectWidth(clientArea) - listWidth) / 2;
	constexpr auto listY = textY + textHeight;

	constexpr auto buttonX = buttonPadding;
	constexpr auto buttonY = listY + listHeight + buttonPadding;


	auto bannerLoader = std::bind(loadImageFromLauncherPath, std::ref(ra3Path), std::placeholders::_1, std::ref(bannerRect));
	auto banner480 = tryWith(std::bind(bannerLoader, languageData.languageName + L"_480banner.bmp"),
	                         std::bind(bannerLoader, L"480banner.bmp"),
	                         nullptr);
	auto banner480Brush = tryWith(std::bind(createPatternBrush, banner480.get()), nullptr);
	auto font = getNormalFont();

	auto languages = getAllLanguages(ra3Path);
	if(auto current = std::find(std::begin(languages), std::end(languages), languageData.languageName);
	        current != std::end(languages) and current != std::begin(languages)) {
		std::swap(*current, *std::begin(languages));
	}

	auto handlers = ModalDialogBox::HandlerTable{};

	handlers[WM_INITDIALOG] = [controlCenter, icon, &languageData, &font, &languages](HWND dialogBox, WPARAM wParam, LPARAM lParam) {
		myBeginDialog(dialogBox, getText(languageData, setLanguage), icon,
		              controlCenter, ICC_STANDARD_CLASSES, clientArea);

		auto list = createControl(dialogBox, setLanguageList, WC_COMBOBOXW, {},
		                          WS_VISIBLE|CBS_DROPDOWNLIST, 0, listX, listY, listWidth, 0).release();
		for(auto i = std::size_t{0}; i < languages.size(); ++i) {
			SendMessageW(list, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(getLanguageName(languageData, languages[i]).c_str()))
			        >> checkWin32Result("SendMessageW CB_ADDSTRINGW", successValue, static_cast<LRESULT>(i));
		}
		SendMessageW(list, CB_SETCURSEL, 0, 0) >> checkWin32Result("SendMessageW CB_SETCURSEL", successValue, 0);

		createControl(dialogBox, setLanguageDescription, WC_STATICW, getText(languageData, setLanguageDescription).c_str(),
		              WS_VISIBLE|SS_LEFT|SS_CENTERIMAGE, 0, textX, textY, textWidth, textHeight).release();

		auto buttonOffset = buttonX;
		for(auto id : {setLanguageOK, setLanguageCancel}) {
			createControl(dialogBox, id, WC_BUTTONW, getText(languageData, id),
			              WS_VISIBLE, 0, buttonOffset, buttonY, buttonWidth, buttonHeight).release();
			buttonOffset += buttonWidth + buttonPadding;
		}

		for(auto id : {setLanguageList, setLanguageDescription, setLanguageOK, setLanguageCancel}) {
			SendMessageW(getControlByID(dialogBox, id), WM_SETFONT, reinterpret_cast<WPARAM>(font.get()), true);
		}
		return TRUE;
	};

	handlers[WM_PAINT] = [&languageData, &banner480Brush](HWND dialogBox, WPARAM, LPARAM lParam) {
		if(banner480Brush.get() == nullptr) {
			return FALSE;
		}
		auto painter = beginPaint(dialogBox);
		FillRect(painter->context, &bannerRect, banner480Brush.get()) >> checkWin32Result("FillRect", errorValue, false);
		return TRUE;
	};

	handlers[WM_COMMAND] = [](HWND dialogBox, WPARAM information, LPARAM childWindowHandle) {
		auto notificationCode = HIWORD(information);
		auto identifier = LOWORD(information);
		if(notificationCode == BN_CLICKED) {
			if(identifier == setLanguageOK) {
				auto id = SendMessageW(getControlByID(dialogBox, setLanguageList), CB_GETCURSEL, 0, 0)
				          >> checkWin32Result("SendMessageW CB_GETCURSEL", errorValue, CB_ERR);
				EndDialog(dialogBox, id) >> checkWin32Result("EndDialog", errorValue, false);
			}
			if(identifier == setLanguageCancel) {
				EndDialog(dialogBox, 0) >> checkWin32Result("EndDialog", errorValue, false);
			}
		}
		return TRUE;
	};

	handlers[WM_CLOSE] = [](HWND dialogBox, WPARAM, LPARAM) {
		EndDialog(dialogBox, 0) >> checkWin32Result("EndDialog", errorValue, false);
		return TRUE;
	};

	auto result = modalDialogBox(handlers, WS_VISIBLE|WS_SYSMENU, 0, controlCenter);
	return loadLanguageData(ra3Path, languages.at(result));
};


template<typename IDWidthList>
WindowHandle createListView(HWND parent, ID id, const IDWidthList& columns, RECT windowRect, const LanguageData& languageData) {
	auto listView = createControl(parent, id, WC_LISTVIEWW, {},
	                              LVS_REPORT|LVS_SHOWSELALWAYS|LVS_SINGLESEL, 0,
	                              windowRect.left, windowRect.top, rectWidth(windowRect), rectHeight(windowRect));
	SendMessageW(listView.get(), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	auto listViewWidth = rectWidth(getClientRect(listView.get())) - GetSystemMetrics(SM_CXVSCROLL); //minus width of scroll bar

	auto columnIndex = 0;
	for(auto [id, widthPercent] : columns) {
		auto buf = getText(languageData, id);
		auto column = LVCOLUMNW{LVCF_TEXT|LVCF_WIDTH};
		column.pszText = buf.data();
		column.cx = widthPercent * listViewWidth;
		SendMessageW(listView.get(), LVM_INSERTCOLUMNW, columnIndex, reinterpret_cast<LPARAM>(&column))
		        >> checkWin32Result("SendMessageW LVM_INSERTCOLUMNW", successValue, columnIndex);

		columnIndex += 1;
	}

	return listView;
}

void replaceListView (HWND listView, std::vector<std::vector<std::wstring>> items) {
	SendMessageW(listView, LVM_DELETEALLITEMS, 0, 0)
	        >> checkWin32Result("SendMessageW LVM_DELETEALLITEMS", errorValue, false);

	for(auto index = std::size_t{0}; index < items.size(); ++index) {
		for(auto subIndex = std::size_t{0}; subIndex < items[index].size(); ++subIndex) {
			auto listItem = LVITEMW{LVIF_TEXT};
			listItem.iItem = index;
			listItem.iSubItem = subIndex;
			listItem.pszText = items[index][subIndex].data();
			if(listItem.iSubItem == 0) {
				SendMessageW(listView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&listItem))
				        >> checkWin32Result("SendMessageW LVM_INSERTITEMW", errorValue, -1);
			}
			else {
				SendMessageW(listView, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&listItem))
				        >> checkWin32Result("SendMessageW LVM_INSERTITEMW", errorValue, false);
			}
		}
	}
}

struct ReplaysAndModsData {

	std::vector<std::vector<std::wstring>> replayDetailsToStrings() const {

		auto allocateFormat = [](auto function, DWORD flags, const SYSTEMTIME& time) {
			auto size = function(LOCALE_USER_DEFAULT, flags, &time, nullptr, nullptr, 0)
			            >> checkWin32Result("allocateFormat", errorValue, 0);
			auto string = std::wstring{static_cast<std::size_t>(size), {}, std::wstring::allocator_type{}};
			function(LOCALE_USER_DEFAULT, flags, &time, nullptr, string.data(), string.size())
			        >> checkWin32Result("allocateFormat", errorValue, 0);
			string.erase(string.find(L'\0'));
			return string;
		};

		auto strings = std::vector<std::vector<std::wstring>> {};
		for(const auto& replay : this->replayDetails) {
			auto fileTime = unixTimeToFileTime(replay.timeStamp);
			auto systemTime = SYSTEMTIME{};
			FileTimeToSystemTime(&fileTime, &systemTime)
			        >> checkWin32Result("FileTimeToSystemTime", errorValue, false);
			auto localTime = SYSTEMTIME{};
			SystemTimeToTzSpecificLocalTime(nullptr, &systemTime, &localTime)
			        >> checkWin32Result("SystemTimeToTzSpecificLocalTime", errorValue, false);

			auto date = allocateFormat(GetDateFormatW, DATE_LONGDATE, localTime);
			auto time = allocateFormat(GetTimeFormatW, TIME_NOSECONDS, localTime);

			auto gameVersion = std::to_wstring(replay.gameVersion.first) + L'.' + std::to_wstring(replay.gameVersion.second);
			strings.emplace_back(std::vector{replay.replayName, replay.modName, std::move(gameVersion)});
			strings.back().emplace_back(date + L' ' + time);
		}
		return strings;
	}

	std::vector<std::vector<std::wstring>> modDetailsToStrings() const {
		auto strings = std::vector<std::vector<std::wstring>> {};
		for(const auto& [fullPath, modName, modVersion] : this->modDetails) {
			strings.emplace_back(std::vector{modName, modVersion});
		}
		return strings;
	}

	std::wstring getReplayDescription(std::size_t index, const LanguageData& languageData) const {
		static constexpr auto endLine = L"\r\n";
		auto details = this->replayDetails.at(index);
		auto result = std::wstring{};

		result += getText(languageData, replayMatchInformation);
		result += L" [" + details.replayName + L"] [" + this->getReplayDuration(index).value_or(getText(languageData, replayNeedsToBeFixed)) + L"]" + endLine;
		result += details.title + endLine;
		result += getText(languageData, replayMap) + L' ' + details.map + endLine;
		result += getText(languageData, replayNumberOfPlayers);
		result += L' ' + std::to_wstring(details.players.size()) + endLine;

		for(const auto& player : details.players) {
			result += L' ' + player + L' ';
		}
		result += endLine;

		result += getText(languageData, replayDescription) + L' ' + details.description;
		result += endLine;
		return result;
	}

	std::optional<std::wstring> getReplayDuration(std::size_t index) const {
		const auto& timeCode = this->replayDetails.at(index).finalTimeCode;
		if(not timeCode.has_value()) {
			return std::nullopt;
		}
		auto seconds = timeCode.value() / 15;
		auto result = std::wstring{};
		for(const auto count : {seconds / 3600, (seconds % 3600) / 60, seconds % 60}) {
			auto portion = std::to_wstring(count);
			result += std::wstring{2 - portion.size(), L'0', std::wstring::allocator_type{}};
			result += portion;
			result += L':';
		}
		result.resize(std::min(result.size(), result.rfind(L':')));
		return result;
	}

	std::vector<ReplaysAndMods::ReplayDetails> replayDetails;
	std::vector<ReplaysAndMods::ModDetails> modDetails;
};

std::optional<LaunchOptions> runGameBrowser(HWND controlCenter, const std::wstring& ra3Path, HICON icon, const LanguageData& languageData) {
	using std::pair;
	static constexpr auto tabs = {replays, mods};
	static constexpr auto replaySubWindows = {replayList, replayDescription, fixReplay, replayFolder};
	static constexpr auto modSubWindows = {modList, modFolder};
	static constexpr auto tabSubWindows = {pair{replays, replaySubWindows}, pair{mods, modSubWindows}};
	static constexpr auto replayListColumns = {pair{replayListReplayName, 0.52}, pair{replayListModName, 0.14}, pair{replayListGameVersion, 0.11}, pair{replayListDate, 0.23}};
	static constexpr auto modListColumns = {pair{modListModName, 0.8}, pair{modListModVersion, 0.2}};
	static constexpr auto bottomButtons = {gameBrowserLaunchGame, gameBrowserCancel};

	constexpr auto buttonWidth = 110;
	constexpr auto buttonHeight = 30;
	constexpr auto buttonPadding = 5;
	constexpr auto replayDescriptionHeight = 110;

	static constexpr auto bannerRect = RECT{0, 0, 640, 100};
	static constexpr auto clientArea = RECT{0, 0, rectWidth(bannerRect), 540};
	static constexpr auto page = RECT{0, rectHeight(bannerRect), rectWidth(clientArea), rectHeight(clientArea) - buttonHeight - 2 * buttonPadding};

	auto bannerLoader = std::bind(loadImageFromLauncherPath, std::ref(ra3Path), std::placeholders::_1, std::ref(bannerRect));
	auto banner640 = tryWith(std::bind(bannerLoader, languageData.languageName + L"_640banner.bmp"),
	                         std::bind(bannerLoader, L"640banner.bmp"),
	                         nullptr);
	auto banner640Brush = tryWith(std::bind(createPatternBrush, banner640.get()), nullptr);
	auto font = getNormalFont();
	auto handlers = ModalDialogBox::HandlerTable{};

	handlers[WM_INITDIALOG] = [controlCenter, icon, &languageData, &font](HWND dialogBox, WPARAM wParam, LPARAM lParam) {
		myBeginDialog(dialogBox, getText(languageData, gameBrowser), icon, controlCenter,
		              ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES, clientArea);

		auto tab = createControl(dialogBox, gameBrowserTabs, WC_TABCONTROLW, {},
		                         WS_VISIBLE, 0, 0, page.top, rectWidth(page), rectHeight(page)).release();
		//font must be set before calculating width of tabs
		SendMessageW(tab, WM_SETFONT, reinterpret_cast<WPARAM>(font.get()), true);

		auto totaltabsWidth = 0;
		auto tabIndex = 0;
		for(auto id : tabs) {
			auto tabItem = TCITEMW{TCIF_TEXT};
			auto buf = getText(languageData, id);
			tabItem.pszText = buf.data();
			SendMessageW(tab, TCM_INSERTITEMW, tabIndex, reinterpret_cast<LPARAM>(&tabItem))
			        >> checkWin32Result("SendMessageW TCM_INSERTITEMW", successValue, tabIndex);

			auto tabRect = RECT{};
			SendMessageW(tab, TCM_GETITEMRECT, tabIndex, reinterpret_cast<LPARAM>(&tabRect))
			        >> checkWin32Result("SendMessageW TCM_GETITEMRECT", successValue, true);
			totaltabsWidth += rectWidth(tabRect);

			tabIndex += 1;
		}

		auto tabRect = RECT{page.left, page.top, page.left + totaltabsWidth, page.top};
		SendMessageW(tab, TCM_ADJUSTRECT, true, reinterpret_cast<LPARAM>(&tabRect));
		SetWindowPos(tab, nullptr, tabRect.left, tabRect.top, rectWidth(tabRect), rectHeight(tabRect), SWP_NOZORDER)
		        >> checkWin32Result("SetWindowPos", errorValue, false);

		auto buttonOffset = buttonPadding * 2;
		for(auto id : bottomButtons) {
			createControl(dialogBox, id, WC_BUTTONW, getText(languageData, id).c_str(),
			              WS_VISIBLE, 0, buttonOffset, page.bottom + buttonPadding, buttonWidth, buttonHeight).release();

			buttonOffset += buttonWidth + buttonPadding;
		}

		auto replayListRect = page;
		replayListRect.bottom = page.bottom - replayDescriptionHeight;
		createListView(dialogBox, replayList, replayListColumns, replayListRect, languageData).release();

		createControl(dialogBox, replayDescription, WC_EDITW, {},
		              WS_VSCROLL|ES_MULTILINE|ES_READONLY, WS_EX_CLIENTEDGE,
		              page.left, page.bottom - replayDescriptionHeight, rectWidth(page), replayDescriptionHeight).release();

		createControl(dialogBox, replayFolder, WC_BUTTONW, getText(languageData, replayFolder).c_str(),
		              0, 0, page.right - 1 * (buttonPadding + buttonWidth), page.bottom + buttonPadding, buttonWidth, buttonHeight).release();
		createControl(dialogBox, fixReplay, WC_BUTTONW, getText(languageData, fixReplay).c_str(),
		              0, 0, page.right - 2 * (buttonPadding + buttonWidth), page.bottom + buttonPadding, buttonWidth, buttonHeight).release();

		createListView(dialogBox, modList, modListColumns, page, languageData).release();

		createControl(dialogBox, modFolder, WC_BUTTONW, getText(languageData, modFolder).c_str(),
		              0, 0, page.right - 1 * (buttonPadding + buttonWidth), page.bottom + buttonPadding, buttonWidth, buttonHeight).release();

		for(auto id : {gameBrowserLaunchGame, gameBrowserCancel, replayDescription, fixReplay, replayFolder, modFolder}) {
			SendMessageW(getControlByID(dialogBox, id), WM_SETFONT, reinterpret_cast<WPARAM>(font.get()), true);
		}

		//When initialization finishes, set mod tab as the default tab
		auto modPosition = std::find(std::begin(tabs), std::end(tabs), mods) - std::begin(tabs);
		SendMessageW(tab, TCM_SETCURFOCUS, modPosition, 0);

		return TRUE;
	};

	handlers[WM_PAINT] = [&banner640Brush](HWND dialogBox, WPARAM, LPARAM lParam) {
		if(banner640Brush.get() == nullptr) {
			return FALSE;
		}
		auto painter = beginPaint(dialogBox);
		FillRect(painter->context, &bannerRect, banner640Brush.get()) >> checkWin32Result("FillRect", errorValue, false);
		return TRUE;
	};

	auto replaysAndMods = ReplaysAndModsData{};
	auto launchOptions = std::optional<LaunchOptions> {};

	auto getCurrentTabID = [](HWND dialogBox) {
		auto selected = SendMessageW(getControlByID(dialogBox, gameBrowserTabs), TCM_GETCURSEL, 0, 0)
		                >> checkWin32Result("SendMessageW TCM_GETCURSEL", errorValue, -1);
		if(static_cast<std::size_t>(selected) >= tabs.size()) {
			throw std::out_of_range("currentSelection > tabs.size()");
		}
		return std::begin(tabs)[selected];
	};

	auto updateListWindow = [&replaysAndMods](HWND dialogBox, ID id) {
		auto window = HWND{nullptr};
		auto strings = std::vector<std::vector<std::wstring>> {};
		if(id == replays) {
			window = getControlByID(dialogBox, replayList);
			strings = replaysAndMods.replayDetailsToStrings();
		}
		if(id == mods) {
			window = getControlByID(dialogBox, modList);
			strings = replaysAndMods.modDetailsToStrings();
		}
		replaceListView(window, strings);
	};

	auto sortCurrentList = [&replaysAndMods, getCurrentTabID, updateListWindow](HWND dialogBox, std::size_t columnIndex) {
		using namespace std::placeholders;
		auto toggleSort = [](auto& detailsVector, auto memberAddress, auto compare) {
			auto compareMember = std::bind(compare, std::bind(memberAddress, _1), std::bind(memberAddress, _2));
			if(std::is_sorted(std::begin(detailsVector), std::end(detailsVector), compareMember)) {
				std::sort(std::rbegin(detailsVector), std::rend(detailsVector), compareMember);
			}
			else {
				std::sort(std::begin(detailsVector), std::end(detailsVector), compareMember);
			}
		};

		auto currentID = getCurrentTabID(dialogBox);
		if(currentID == replays) {
			if(columnIndex >= replayListColumns.size()) { throw std::out_of_range("columnIndex > replayListColumns.size()"); }
			auto columnIs = [columnIndex](ID id) { return std::begin(replayListColumns)[columnIndex].first == id; };
			auto sortReplays = std::bind(toggleSort, std::ref(replaysAndMods.replayDetails), _1, _2);
			using Details = ReplaysAndMods::ReplayDetails;

			if(columnIs(replayListReplayName)) sortReplays(&Details::replayName, StringLess{});
			if(columnIs(replayListModName)) sortReplays(&Details::modName, StringLess{});
			if(columnIs(replayListGameVersion)) sortReplays(&Details::gameVersion, std::less{});
			if(columnIs(replayListDate)) sortReplays(&Details::timeStamp, std::less{});
		}

		if(currentID == mods) {
			if(columnIndex >= modListColumns.size()) { throw std::out_of_range("columnIndex > modListColumns.size()"); }
			auto columnIs = [columnIndex](ID id) { return std::begin(modListColumns)[columnIndex].first == id; };
			auto sortModStrings = std::bind(toggleSort, std::ref(replaysAndMods.modDetails), _1, StringLess{});
			using Details = ReplaysAndMods::ModDetails;

			if(columnIs(modListModName)) sortModStrings(&Details::modName);
			if(columnIs(modListModVersion)) sortModStrings(&Details::version);
		}

		updateListWindow(dialogBox, currentID);
	};

	auto setSelected = [&languageData, &replaysAndMods, &launchOptions, getCurrentTabID](HWND dialogBox, std::size_t index) {
		auto currentID = getCurrentTabID(dialogBox);

		if(currentID == replays and index < replaysAndMods.replayDetails.size()) {
			const auto& replay = replaysAndMods.replayDetails[index];
			launchOptions = {LaunchOptions::replay, replay.fullPath};
			//update replay description
			auto description = replaysAndMods.getReplayDescription(index, languageData);
			SetWindowTextW(getControlByID(dialogBox, replayDescription), description.c_str())
			        >> checkWin32Result("SetWindowTextW", successValue, true);
			auto needFix = not replay.finalTimeCode.has_value();
			EnableWindow(getControlByID(dialogBox, fixReplay), needFix);
		}
		if(currentID == mods and index < replaysAndMods.modDetails.size()) {
			const auto& mod = replaysAndMods.modDetails[index];
			launchOptions = {LaunchOptions::mod, mod.fullPath};
		}
		EnableWindow(getControlByID(dialogBox, gameBrowserLaunchGame), true);
	};

	auto initializeTab = [&replaysAndMods, &launchOptions, getCurrentTabID, updateListWindow](HWND dialogBox) {
		//disable launch game button
		EnableWindow(getControlByID(dialogBox, gameBrowserLaunchGame), false);
		EnableWindow(getControlByID(dialogBox, fixReplay), false);
		launchOptions.reset();
		//clear replay detail box
		SetWindowTextW(getControlByID(dialogBox, replayDescription), L"")
		        >> checkWin32Result("SetWindowTextW", successValue, true);

		auto currentID = getCurrentTabID(dialogBox);

		for(auto [tabID, windowList] : tabSubWindows) {
			auto activeFlag = tabID == currentID;
			for(auto windowID : windowList) {
				auto window = getControlByID(dialogBox, windowID);
				ShowWindow(window, activeFlag == true ? SW_SHOW : SW_HIDE);
				EnableWindow(window, activeFlag and (windowID != fixReplay));
			}
		}

		if(currentID == replays) {
			replaysAndMods.replayDetails = ReplaysAndMods::getAllReplayDetails();
		}
		if(currentID == mods) {
			replaysAndMods.modDetails = ReplaysAndMods::getModSkudefs();
		}

		updateListWindow(dialogBox, currentID);
		return TRUE;
	};

	auto fixReplayWorker = [&languageData, &launchOptions, initializeTab](HWND dialogBox) {
		if(not launchOptions.has_value() or launchOptions->loadFileType != LaunchOptions::replay) {
			return;
		}
		auto fix = MessageBoxW(dialogBox,
		                       getText(languageData, fixReplayWarning).c_str(),
		                       getText(languageData, fixReplay).c_str(),
		                       MB_ICONWARNING|MB_YESNO)
		           >> checkWin32Result("MessageBoxW", errorValue, 0);
		if(fix != IDYES) {
			return;
		}
		try {
			fixReplayByFileName(launchOptions->fileToBeLoaded);
			notifyFixReplaySucceeded(languageData);
		}
		catch(const std::exception& error) {
			notifyFixReplayFailed(error, languageData);
		}

		initializeTab(dialogBox);
	};

	auto launchGame = [&launchOptions](HWND dialogBox) {
		if(not launchOptions.has_value()) {
			return;
		}
		EndDialog(dialogBox, 1) >> checkWin32Result("EndDialog", errorValue, false);
	};

	auto cancel = [](HWND dialogBox) {
		EndDialog(dialogBox, 0) >> checkWin32Result("EndDialog", errorValue, false);
		return TRUE;
	};

	handlers[WM_NOTIFY] = [sortCurrentList, setSelected, launchGame, initializeTab](HWND dialogBox, WPARAM, LPARAM dataHeaderAddress) {
		auto dataHeader = reinterpret_cast<const NMHDR*>(dataHeaderAddress);

		if(dataHeader->idFrom == gameBrowserTabs) {
			if(dataHeader->code == TCN_SELCHANGE) {
				initializeTab(dialogBox);
				return TRUE;
			}
		}

		if(dataHeader->idFrom == replayList or dataHeader->idFrom == modList) {
			if(dataHeader->code == LVN_COLUMNCLICK) {
				auto listViewData = reinterpret_cast<const NMLISTVIEW*>(dataHeader);
				sortCurrentList(dialogBox, listViewData->iSubItem);
				return TRUE;
			}
			if(dataHeader->code == LVN_ITEMCHANGED) {
				auto listViewData = reinterpret_cast<const NMLISTVIEW*>(dataHeader);
				if(listViewData->uNewState bitand LVIS_FOCUSED) {
					setSelected(dialogBox, listViewData->iItem);
					return TRUE;
				}
				return TRUE;
			}
			if(dataHeader->code == NM_DBLCLK) {
				if(dataHeader->idFrom == replayList or dataHeader->idFrom == modList) {
					launchGame(dialogBox);
				}
				return TRUE;
			}
		}
		return FALSE;
	};

	handlers[WM_COMMAND] = [fixReplayWorker, launchGame, cancel](HWND dialogBox, WPARAM codeAndIdentifier, LPARAM controlHandle) {
		auto notificationCode = HIWORD(codeAndIdentifier);
		auto identifier = LOWORD(codeAndIdentifier);
		if(notificationCode == BN_CLICKED) {
			if(identifier == gameBrowserLaunchGame) {
				launchGame(dialogBox);
				return TRUE;
			}
			if(identifier == gameBrowserCancel) {
				cancel(dialogBox);
				return TRUE;
			}


			if(identifier == replayFolder) {
				shellExecute(dialogBox, L"explore", ReplaysAndMods::concatenateWithReplayFolder({}).c_str());
				return TRUE;
			}
			if(identifier == fixReplay) {
				fixReplayWorker(dialogBox);
				return TRUE;
			}
			if(identifier == modFolder) {
				shellExecute(dialogBox, L"explore", ReplaysAndMods::concatenateWithModRootFolder({}).c_str());
				return TRUE;
			}
		}
		return FALSE;
	};

	handlers[WM_CLOSE] = std::bind(cancel, std::placeholders::_1);

	auto launch = (modalDialogBox(handlers, WS_VISIBLE|WS_SYSMENU, 0, controlCenter) == 1);

	if(launch == false) {
		launchOptions.reset();
	}

	return launchOptions;
}

void aboutWindow(HWND parent, HICON icon, const LanguageData& languageData) {

	static constexpr auto clientArea = RECT{0, 0, 330, 420};
	static constexpr auto logoRect = RECT{15, 310, 315, 410};
	auto handlers = ModalDialogBox::HandlerTable{};
	auto font = getNormalFont();
	auto ra3barData = loadBinaryDataResource<DWORD>(GetModuleHandle(nullptr), MAKEINTRESOURCEW(RA3BAR_LOGO), RT_RCDATA);
	auto ra3barBitmap = BitmapHandle{};
	auto maskBitmap = BitmapHandle{};
	auto initializeLogo = [&ra3barData, &ra3barBitmap, &maskBitmap](HDC context, int width, int height) {
		ra3barBitmap = createCompatibleBitmap(context, width, height);
		{
			auto imageBits = getBitmapBits(context, ra3barBitmap.get(), {{width, -height}});
			auto i = std::size_t{0};
			for(auto color : ra3barData) {
				auto alpha = (color >> 24) bitand 0xFF;
				auto red = (color >> 16) bitand 0xFF;
				auto green = (color >> 8) bitand 0xFF;
				auto blue = (color >> 0) bitand 0xFF;
				imageBits.buffer.at(i) = RGB(red, green, blue) * (alpha != 0);
				++i;
			}
			setBitmapBits(context, ra3barBitmap.get(), imageBits);
		}

		maskBitmap = createCompatibleBitmap(context, width, height);
		{
			constexpr auto white = DWORD{0x00FFFFFF};
			auto maskBits = getBitmapBits(context, maskBitmap.get(), {{width, -height}});
			auto i = std::size_t{0};
			for(auto color : ra3barData) {
				maskBits.buffer.at(i) = white * ((color >> 24) == 0);
				++i;
			}
			setBitmapBits(context, maskBitmap.get(), maskBits);
		}
	};

	handlers[WM_INITDIALOG] = [parent, icon, &languageData, &font, initializeLogo](HWND dialogBox, WPARAM, LPARAM) {
		myBeginDialog(dialogBox, getText(languageData, about), icon,
		              parent, ICC_STANDARD_CLASSES|ICC_LINK_CLASS, clientArea);
		auto text = getText(languageData, aboutText) + L"\r\n\r\n" + getText(languageData, resourceAuthors);
		auto control = createControl(dialogBox, aboutText, WC_LINK, text.c_str(), WS_VISIBLE, 0, 10, 10, 310, 300).release();
		SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font.get()), true);
		initializeLogo(getDeviceContext(dialogBox)->context, 300, 100);
		return TRUE;
	};
	handlers[WM_PAINT] = [&ra3barBitmap, &maskBitmap](HWND dialogBox, WPARAM, LPARAM) {
		auto paint = beginPaint(dialogBox);

		auto mask = createCompatibleDeviceContext(paint->context);
		auto previousMaskBitmap = selectObject(mask.get(), maskBitmap.get());
		auto [x, y, ex, ey] = logoRect;
		BitBlt(paint->context, x, y, ex - x, ey - y, mask.get(), 0, 0, SRCAND)
		        >> checkWin32Result("BitBlt", errorValue, false);

		auto logo = createCompatibleDeviceContext(paint->context);
		auto previousLogoBitmap = selectObject(logo.get(), ra3barBitmap.get());
		BitBlt(paint->context, x, y, ex - x, ey - y, logo.get(), 0, 0, SRCPAINT)
		        >> checkWin32Result("BitBlt", errorValue, false);
		return TRUE;
	};
	handlers[WM_NOTIFY] = [](HWND dialogBox, WPARAM, LPARAM dataHeaderAddress) {
		const auto& dataHeader = *reinterpret_cast<const NMHDR*>(dataHeaderAddress);
		if(dataHeader.idFrom == aboutText and dataHeader.code == NM_CLICK) {
			const auto& linkInformation = *reinterpret_cast<const NMLINK*>(dataHeaderAddress);
			shellExecute(dialogBox, L"open", linkInformation.item.szUrl);
			return TRUE;
		}
		return FALSE;
	};
	handlers[WM_CLOSE] = [](HWND dialogBox, WPARAM, LPARAM) {
		EndDialog(dialogBox, 0) >> checkWin32Result("EndDialog", errorValue, false);
		return TRUE;
	};
	modalDialogBox(handlers, WS_VISIBLE|WS_SYSMENU, 0, parent);
}
