#pragma once
#include <utility>
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <system_error>
#include <functional>
#include <exception>
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Shlwapi.h>

namespace Windows {
	template<typename T>
	struct LocalMemoryDeleter {
		using pointer = std::decay_t<T>;
		void operator()(pointer memory) const noexcept {
			LocalFree(static_cast<HLOCAL>(memory));
		}
	};

	template<typename T>
	using LocalMemory = std::unique_ptr<T, LocalMemoryDeleter<T>>;

	inline std::string toBytes(std::wstring_view wideString, UINT codePage = CP_UTF8);
	inline std::wstring toWide(std::string_view byteString, UINT codePage = CP_UTF8);

	class WindowsErrorCategory : public std::error_category {
			static constexpr char selfName[] = "MyWindowsErrorCategory";
		public:
			virtual const char* name() const noexcept {
				return selfName;
			}
			virtual std::string message(int messageID) const {
				auto getErrorMessage = [](DWORD messageID, DWORD completeLanguageID) {
					auto messageBuffer = LPWSTR{};
					auto size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					                           nullptr, messageID, completeLanguageID, reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);
					auto message = std::string {"<Cannot retrieve error message>"};
					if(size > 0) {
						try {
							auto buf = LocalMemory<LPWSTR> {messageBuffer};
							message.assign(toBytes({buf.get(), size}));
						}
						catch(...) { }
					}
					return message;
				};
				auto message = getErrorMessage(messageID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				message += '\n';
				message += getErrorMessage(messageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
				return message;
			}
	};

	inline const WindowsErrorCategory& GetWindowsCategory() {
		auto lastError = GetLastError();
		static thread_local auto windowsErrorCategory = WindowsErrorCategory {};
		SetLastError(lastError);
		return windowsErrorCategory;
	}

	template<typename Predicate, typename ErrorCodeType>
	struct CheckWin32ResultProxy {
		const char* name;
		Predicate predicate;
		ErrorCodeType lastError;
	};
	template<typename Predicate, typename ErrorCodeType>
	CheckWin32ResultProxy(const char*, Predicate, ErrorCodeType) -> CheckWin32ResultProxy<Predicate, ErrorCodeType>;

	inline constexpr std::equal_to<> successValue;
	inline constexpr std::not_equal_to<> errorValue;
	inline constexpr struct SuccessIf { } successIf;
	inline constexpr struct ResultIsErrorCode { } resultIsErrorCode;

	template<typename ReturnType, typename Predicate, typename ErrorCodeType>
	ReturnType operator>>(ReturnType result, const CheckWin32ResultProxy<Predicate, ErrorCodeType>& proxy) {
		if(not proxy.predicate(result)) {
			if constexpr (std::is_same_v<ErrorCodeType, ResultIsErrorCode>) {
				throw std::system_error(result, WindowsErrorCategory(), std::string{proxy.name} + " failed");
			}
			else {
				throw std::system_error(proxy.lastError, WindowsErrorCategory(), std::string{proxy.name} + " failed");
			}
		}
		return result;
	}

	template<typename Expected, typename ExpectKind, typename ErrorCodeType = DWORD>
	auto checkWin32Result(const char* actionName, ExpectKind resultChecker, const Expected& expected, ErrorCodeType lastError = GetLastError()) {
		if constexpr(std::is_same_v<ExpectKind, SuccessIf>) {
			return CheckWin32ResultProxy{actionName, expected, lastError};
		}
		else {
			return CheckWin32ResultProxy{actionName, std::bind(resultChecker, expected, std::placeholders::_1), lastError};
		}
	}

	inline std::wstring toWide(std::string_view byteString, UINT codePage /* = CP_UTF8 */) {
		if(byteString.empty()) {
			return std::wstring {};
		}

		auto neededCharacters = MultiByteToWideChar(codePage, 0, byteString.data(), byteString.size(), nullptr, 0);
		neededCharacters >> checkWin32Result("MultiByteToWideChar", errorValue, 0);

		auto buf = std::make_unique<wchar_t[]>(static_cast<size_t>(neededCharacters));

		auto convertedCharacters = MultiByteToWideChar(codePage, 0, byteString.data(), byteString.size(), buf.get(), neededCharacters);
		convertedCharacters >> checkWin32Result("MultiByteToWideChar", successValue, neededCharacters);

		return std::wstring {buf.get(), static_cast<size_t>(convertedCharacters)};
	}

	inline std::string toBytes(std::wstring_view wideString, UINT codePage /* = CP_UTF8 */) {
		if(wideString.empty()) {
			return std::string {};
		}

		auto neededCharacters = WideCharToMultiByte(codePage, 0, wideString.data(), wideString.size(), nullptr, 0, nullptr, nullptr);
		neededCharacters >> checkWin32Result("MultiByteToWideChar", errorValue, 0);

		auto buf = std::make_unique<char[]>(static_cast<size_t>(neededCharacters));

		auto convertedCharacters = WideCharToMultiByte(codePage, 0, wideString.data(), wideString.size(), buf.get(), neededCharacters, nullptr, nullptr);
		convertedCharacters >> checkWin32Result("MultiByteToWideChar", successValue, neededCharacters);

		return std::string {buf.get(), static_cast<size_t>(convertedCharacters)};
	}

	template<typename T> struct NullableStruct {
		constexpr NullableStruct(std::nullptr_t = nullptr) noexcept { }
		constexpr NullableStruct(std::nullopt_t) noexcept { }
		constexpr NullableStruct(const std::optional<T>& data) : data{data} { }
		constexpr NullableStruct(std::optional<T>&& data) : data{std::move(data)} { }
		constexpr T& operator*() { return this->data.value(); }
		constexpr const T& operator*() const { return this->data.value(); }
		constexpr std::optional<T>& operator->() { return this->data; }
		constexpr const std::optional<T>& operator->() const { return this->data; }
		constexpr explicit operator bool() { return static_cast<bool>(this->data); }
		friend constexpr bool operator==(const NullableStruct& a, const NullableStruct& b) noexcept { return a.data == b.data; }
		friend constexpr bool operator!=(const NullableStruct& a, const NullableStruct& b) noexcept { return not (a.data == b.data); }
		std::optional<T> data;
	};

	/*struct WindowClassData {
		bool operator==(const WindowClassData& b) const noexcept { return this->atom == b.atom and this->module == b.module; }
		LPCWSTR atom;
		HINSTANCE module;
	};

	struct WindowClassDeleter {
		using pointer = NullableStruct<WindowClassData>;
		void operator()(const pointer& windowClass) const noexcept {
			try {
				UnregisterClassW(windowClass->atom, windowClass->module)
				        >> checkWin32Result("UnregisterClassW", errorValue, FALSE);
			}
			catch(const std::exception& e) {
				MessageBoxA(nullptr, e.what(), 0, MB_OK | MB_ICONERROR);
			}
			catch(...) {
				MessageBoxA(nullptr, "UnregisterClassW failed", 0, MB_OK | MB_ICONERROR);
			}
		}
	};

	using WindowClassHolder = std::unique_ptr<WindowClassData, WindowClassDeleter>;

	template<typename WindowType>
	WindowClassHolder registerWindowClass(const std::wstring& className, HICON icon,
	                                      HINSTANCE moduleInstance = GetModuleHandle(nullptr)) {
		auto windowClassExtended = WNDCLASSEXW {};
		windowClassExtended.cbSize        = sizeof(WNDCLASSEX);
		windowClassExtended.style         = CS_HREDRAW | CS_VREDRAW;
		windowClassExtended.lpfnWndProc   = WindowType::windowCallbacks;
		windowClassExtended.cbClsExtra    = 0;
		windowClassExtended.cbWndExtra    = sizeof(WindowType*);
		windowClassExtended.hInstance     = moduleInstance;
		windowClassExtended.hIcon         = icon;
		windowClassExtended.hCursor       = LoadCursorW(NULL, reinterpret_cast<LPCWSTR>(IDC_ARROW));
		windowClassExtended.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		windowClassExtended.lpszMenuName  = nullptr;
		windowClassExtended.lpszClassName = className.c_str();
		windowClassExtended.hIconSm       = icon;

		auto result = ULONG_PTR{RegisterClassExW(&windowClassExtended)}
		              >> checkWin32Result("RegisterClassExW", errorValue, ULONG_PTR{0});
		return WindowClassHolder{{WindowClassData{reinterpret_cast<LPCWSTR>(result), moduleInstance}}};
	}*/

	struct WindowDestroyer {
		using pointer = HWND;
		void operator()(pointer window) const noexcept {
			DestroyWindow(window);
		}
	};

	using WindowHandle = std::unique_ptr<HWND, WindowDestroyer>;

	inline WindowHandle createChildWindow(HWND parent, LPCWSTR windowClass,
	                                      const std::wstring& windowName, DWORD style, DWORD extendedStyle,
	                                      int x, int y, int width, int height,
	                                      HMENU menuHandle = nullptr, HINSTANCE moduleInstance = nullptr, LPVOID parameters = nullptr) {
		auto rawHandle = CreateWindowExW(extendedStyle, windowClass, windowName.c_str(), style,
		                                 x, y, width, height, parent, menuHandle, moduleInstance, parameters);
		rawHandle >> checkWin32Result("CreateWindowExW", errorValue, nullptr);
		return WindowHandle { rawHandle };
	}

	inline WindowHandle createControl(HWND parent, int childIdentifier, LPCWSTR windowClass,
	                                  const std::wstring& windowName, DWORD style, DWORD extendedStyle,
	                                  int x, int y, int width, int height,
	                                  HINSTANCE moduleInstance = nullptr, LPVOID parameters = nullptr) {
		auto rawHandle = CreateWindowExW(extendedStyle, windowClass, windowName.c_str(), style | WS_CHILD,
		                                 x, y, width, height, parent, reinterpret_cast<HMENU>(childIdentifier), moduleInstance, parameters)
		                 >> checkWin32Result("CreateWindowExW", errorValue, nullptr);
		return WindowHandle { rawHandle };
	}

	inline HWND getControlByID(HWND parent, int childIdentifier) {
		return GetDlgItem(parent, childIdentifier) >> checkWin32Result("GetDlgItem", errorValue, nullptr);
	}

	inline int getControlID(HWND childWindow) {
		return GetDlgCtrlID(childWindow) >> checkWin32Result("GetDlgCtrlID", errorValue, 0);
	}

	struct ModalDialogBox {
		using MessageHandler = std::function<INT_PTR(HWND, WPARAM, LPARAM)>;
		using HandlerTable = std::unordered_map<UINT, MessageHandler>;

		static INT_PTR CALLBACK dialogCallbacks(HWND windowHandle, UINT messageID, WPARAM wParam, LPARAM lParam) noexcept {
			if(messageID == WM_INITDIALOG) {
				SetLastError(ERROR_SUCCESS);
				//If messageID is WM_INITDIALOG, then set long ptr
				SetWindowLongPtrW(windowHandle, DWLP_USER, lParam);
				auto lastError = GetLastError();
				try {
					lastError >> checkWin32Result("SetWindowLongPtrW", successValue, DWORD{ERROR_SUCCESS});
				}
				catch(...) {
					//If there errors occurred when seeting DWLP_USER...
					reinterpret_cast<ModalDialogBox*>(lParam)->exceptionLocation = std::current_exception();
					EndDialog(windowHandle, -1);
				}
			}

			auto dataAddress = reinterpret_cast<ModalDialogBox*>(GetWindowLongPtrW(windowHandle, DWLP_USER));
			if(dataAddress == nullptr) {
				//If there is no dwlp_user, then return
				return FALSE;
			}

			auto& data = *dataAddress;

			if((data.callbacksReference.get().count(messageID) == 0) or (data.exceptionLocation != nullptr)) {
				//If there is no callbacks, then return
				return FALSE;
			}

			try {
				return data.callbacksReference.get().at(messageID)(windowHandle, wParam, lParam);
			}
			catch(...) {
				data.exceptionLocation = std::current_exception();
				EndDialog(windowHandle, -1);
				return FALSE;
			}
		}

		std::reference_wrapper<const HandlerTable> callbacksReference;
		std::exception_ptr exceptionLocation;
	};

	inline INT_PTR modalDialogBox(const ModalDialogBox::HandlerTable& callbacks, DWORD styles, DWORD extendedStyles, HWND parent = nullptr, HINSTANCE moduleHandle = nullptr) {
		auto buffer = std::aligned_storage_t<sizeof(DLGTEMPLATE) + 4 * sizeof(WORD), alignof(DLGTEMPLATE)> {};
		auto& dialogBoxTemplate = *reinterpret_cast<DLGTEMPLATE*>(&buffer);
		dialogBoxTemplate = DLGTEMPLATE{styles, extendedStyles};
		auto data = ModalDialogBox{callbacks};
		auto returnValue = DialogBoxIndirectParam(moduleHandle, &dialogBoxTemplate, parent, &ModalDialogBox::dialogCallbacks, reinterpret_cast<LPARAM>(&data));
		if(data.exceptionLocation) {
			std::rethrow_exception(data.exceptionLocation);
		}
		returnValue >> checkWin32Result("DialogBoxIndirectParam", errorValue, -1);
		return returnValue;
	}

	inline constexpr LONG rectWidth(const RECT& rect) {
		return rect.right - rect.left;
	}

	inline constexpr LONG rectHeight(const RECT& rect) {
		return rect.bottom - rect.top;
	}

	inline RECT getClientRect(HWND windowHandle) {
		auto clientRect = RECT{};
		GetClientRect(windowHandle, &clientRect) >> checkWin32Result("GetClientRect", errorValue, false);
		return clientRect;
	}

	inline RECT getWindowRect(HWND windowHandle) {
		auto windowRect = RECT{};
		GetWindowRect(windowHandle, &windowRect) >> checkWin32Result("GetClientRect", errorValue, false);
		return windowRect;
	}

	inline RECT setWindowRectByClientRect(HWND windowHandle, int width, int height) {
		auto expectedClientRect = RECT {0, 0, width, height};
		auto realClientRect = getClientRect(windowHandle);
		auto windowRect = getWindowRect(windowHandle);
		windowRect.left += expectedClientRect.left - realClientRect.left;
		windowRect.top += expectedClientRect.top - realClientRect.top;
		windowRect.right += expectedClientRect.right - realClientRect.right;
		windowRect.bottom += expectedClientRect.bottom - realClientRect.bottom;
		return windowRect;
	}

	struct DeviceContextDeleter {
		using pointer = HDC;
		void operator()(pointer deviceContext) const noexcept {
			DeleteDC(deviceContext) >> checkWin32Result("DeleteDC", errorValue, false);
		}
	};

	using DeviceContextOwner = std::unique_ptr<HDC, DeviceContextDeleter>;

	inline DeviceContextOwner createCompatibleDeviceContext(HDC existingContext) {
		return DeviceContextOwner{CreateCompatibleDC(existingContext)
		                          >> checkWin32Result("CreateCompatibleDC", errorValue, nullptr)};
	}

	struct WindowDeviceContext {
		bool operator==(const WindowDeviceContext& b) const noexcept { return this->context == b.context and this->window == b.window; }
		HWND window;
		HDC context;
	};

	struct WindowDeviceContextReleaser {
		using pointer = NullableStruct<WindowDeviceContext>;
		void operator()(pointer deviceContext) const noexcept {
			ReleaseDC(deviceContext->window, deviceContext->context);
		}
	};

	using RetrievedWindowDeviceContext = std::unique_ptr<WindowDeviceContext, WindowDeviceContextReleaser>;

	inline RetrievedWindowDeviceContext getDeviceContext(HWND windowHandle) {
		return RetrievedWindowDeviceContext{{
				WindowDeviceContext{
					windowHandle,
					GetDC(windowHandle) >> checkWin32Result("GetDC", errorValue, nullptr)
				}
			}};
	}

	struct PaintStruct {
		bool operator==(const PaintStruct& b) const noexcept { return this->window == b.window and this->context == b.context; }
		HWND window;
		PAINTSTRUCT paintStruct;
		HDC context;
	};

	struct PaintStructDeleter {
		using pointer = NullableStruct<PaintStruct>;
		void operator()(const pointer& paintStruct) const noexcept {
			EndPaint(paintStruct->window, &(paintStruct->paintStruct));
		}
	};

	using PaintStructHolder = std::unique_ptr<PaintStruct, PaintStructDeleter>;

	inline PaintStructHolder beginPaint(HWND windowHandle) {
		auto paintStruct = PAINTSTRUCT{};
		auto context = BeginPaint(windowHandle, &paintStruct)
		               >> checkWin32Result("BeginPaint", errorValue, nullptr);
		return PaintStructHolder{{PaintStruct{windowHandle, paintStruct, context}}};
	}

	template<typename T>
	struct GDIPreviousObject {
		bool operator==(const GDIPreviousObject& b) const noexcept { return this->context == b.context and this->object == b.object; }
		HDC context;
		T object;
	};
	template<typename T>
	GDIPreviousObject(HDC, T) -> GDIPreviousObject<T>;

	template<typename T, typename SelectObjectFunctor>
	struct GDISelectObjectRestorer {
		using pointer = NullableStruct<GDIPreviousObject<T>>;
		void operator()(const pointer& previous) const noexcept {
			try {
				restore(previous->context, previous->object).release();
			}
			catch(const std::exception& e) {
				MessageBoxW(nullptr, toWide(e.what()).c_str(), 0, MB_OK | MB_ICONERROR);
			}
		}
		SelectObjectFunctor restore;
	};


	template<typename ObjectRestorer>
	using GDISelectObjectRestoreData = std::unique_ptr<typename ObjectRestorer::pointer, ObjectRestorer>;

	struct SelectObjectFunctor {
		auto operator()(HDC context, HGDIOBJ newObject) const {
			auto previous = SelectObject(context, newObject)
			>> checkWin32Result("SelectObject", successIf, [](HGDIOBJ result) {
				return (result != nullptr) and (result != HGDI_ERROR);
			});
			using Restorer = GDISelectObjectRestorer<HGDIOBJ, SelectObjectFunctor>;
			return GDISelectObjectRestoreData<Restorer> {{GDIPreviousObject{context, previous}}, Restorer{*this}};
		}
	};
	inline constexpr SelectObjectFunctor selectObject;

	struct SelectBackgroundModeFunctor {
		auto operator()(HDC context, int newBackgroundMode) const {
			auto previous = SetBkMode(context, newBackgroundMode) >> checkWin32Result("SetBkMode", errorValue, 0);
			using Restorer = GDISelectObjectRestorer<int, SelectBackgroundModeFunctor>;
			return GDISelectObjectRestoreData<Restorer> {{GDIPreviousObject{context, previous}}, Restorer{*this}};
		}
	};
	inline constexpr SelectBackgroundModeFunctor setBackgroundMode;

	struct SelectTextColorFunctor {
		auto operator()(HDC context, COLORREF newTextColor) const {
			auto previous = SetTextColor(context, newTextColor) >> checkWin32Result("SetTextColor", errorValue, CLR_INVALID);
			using Restorer = GDISelectObjectRestorer<COLORREF, SelectTextColorFunctor>;
			return GDISelectObjectRestoreData<Restorer> {{GDIPreviousObject{context, previous}}, Restorer{*this}};
		}
	};
	inline constexpr SelectTextColorFunctor setTextColor;

	template<auto tag, typename Data, typename Deleter>
	struct TaggedHandle {
			using Handle = typename std::unique_ptr<Data, Deleter>::pointer;
			static constexpr auto typeID = tag;
			constexpr TaggedHandle() noexcept = default;
			explicit TaggedHandle(Handle handle) noexcept : handle { handle } { }
			TaggedHandle(TaggedHandle&& other) noexcept : handle { std::move(other.handle) } { }
			template<typename... Args> TaggedHandle(Args&&... args) noexcept : handle { std::forward<Args>(args)... } { }
			TaggedHandle& operator=(TaggedHandle&& other) noexcept { this->handle = std::move(other.handle); return *this; };
			Handle get() const noexcept { return this->handle.get(); }
		private:
			std::unique_ptr<Data, Deleter> handle;
	};

	template<typename HandleType>
	struct GDIObjectDeleter {
		using pointer = HandleType;
		void operator()(pointer objectHandle) const noexcept {
			DeleteObject(objectHandle);
		}
	};

	struct IconDeleter {
		using pointer = HICON;
		void operator()(pointer bitmap) const noexcept {
			DestroyIcon(bitmap);
		}
	};

	using BitmapHandle = TaggedHandle<IMAGE_BITMAP, HBITMAP, GDIObjectDeleter<HBITMAP>>;
	using BrushHandle = std::unique_ptr<HBRUSH, GDIObjectDeleter<HBRUSH>>;
	using FontHandle = std::unique_ptr<HFONT, GDIObjectDeleter<HFONT>>;
	using IconHandle = TaggedHandle<IMAGE_ICON, HICON, IconDeleter>;

	inline BitmapHandle createCompatibleBitmap(HDC deviceContext, int width, int height) {
		return BitmapHandle{CreateCompatibleBitmap(deviceContext, width, height)
		                    >> checkWin32Result("CreateComspatibleBitmap", errorValue, nullptr)};
	}

	template<typename ImageHandle>
	ImageHandle loadImage(const std::wstring& fileName, std::pair<int, int> xy = {0, 0}) {
		auto result = LoadImageW(nullptr, fileName.c_str(), ImageHandle::typeID, xy.first, xy.second, LR_LOADFROMFILE|LR_DEFAULTSIZE)
		              >> checkWin32Result("LoadImageW", errorValue, nullptr);
		return ImageHandle { reinterpret_cast<typename ImageHandle::Handle>(result) };
	}

	template<typename ImageHandle>
	ImageHandle loadImage(HINSTANCE module, WORD resourceID, std::pair<int, int> xy = {0, 0}) {
		auto result = LoadImageW(module, MAKEINTRESOURCEW(resourceID), ImageHandle::typeID, xy.first, xy.second, LR_DEFAULTSIZE)
		              >> checkWin32Result("LoadImageW", errorValue, nullptr);
		return ImageHandle { reinterpret_cast<typename ImageHandle::Handle>(result) };
	}

	inline BrushHandle createPatternBrush(HBITMAP bitmap) {
		return BrushHandle { CreatePatternBrush(bitmap)
		                     >> checkWin32Result("CreatePatternBrush", errorValue, nullptr) };
	}

	inline FontHandle createFontIndirect(const LOGFONTW& fontAttributes) {
		return FontHandle { CreateFontIndirectW(&fontAttributes)
		                    >> checkWin32Result("CreateFontIndirect", errorValue, nullptr) };
	}

	struct BitmapBits {
		std::pair<long, long> imageSize() const noexcept {
			return {this->bitmapInfo.bmiHeader.biWidth, std::abs(this->bitmapInfo.bmiHeader.biHeight)};
		}
		BITMAPINFO bitmapInfo;
		std::vector<DWORD> buffer;
	};

	inline BitmapBits getBitmapBits(HDC deviceContext, HBITMAP bitmapHandle, std::optional<std::pair<int, int>> requiredWidthHeight = std::nullopt) {
		auto bitmapInfo = BITMAPINFO{};
		auto& header = bitmapInfo.bmiHeader;
		header.biSize = sizeof(BITMAPINFOHEADER);

		GetDIBits(deviceContext, bitmapHandle, 0, /*requiredAbsoluteHeight*/0, nullptr, &bitmapInfo, DIB_RGB_COLORS)
		        >> checkWin32Result("GetDIBIts (retrieving bitmapinfo)", errorValue, false);

		if(header.biBitCount != 32) {
			throw std::runtime_error("Cannot get 32 bit pixels, got " + std::to_string(header.biBitCount) + " instead.");
		}

		auto absoluteHeight = std::abs(header.biHeight);
		if(requiredWidthHeight.has_value()) {
			auto [requiredWidth, requiredHeight] = requiredWidthHeight.value();
			if(requiredWidth != header.biWidth) {
				throw std::runtime_error("Image width != required width");
			}

			auto requiredAbsoluteHeight = std::abs(requiredHeight);

			if(absoluteHeight != requiredAbsoluteHeight) {
				throw std::runtime_error("Image height != required height");
			}
			header.biHeight = requiredHeight;
		}

		header.biCompression = BI_RGB;
		auto buffer = std::vector<DWORD> {};
		buffer.resize(header.biWidth * absoluteHeight);
		GetDIBits(deviceContext, bitmapHandle, 0, absoluteHeight, buffer.data(), &bitmapInfo, DIB_RGB_COLORS)
		        >> checkWin32Result("GetDIBIts", errorValue, false);

		return {bitmapInfo, std::move(buffer)};
	}

	inline void setBitmapBits(HDC deviceContext, HBITMAP bitmapHandle, const BitmapBits& bits) {
		SetDIBits(deviceContext, bitmapHandle, 0, bits.imageSize().second,
		          bits.buffer.data(), &(bits.bitmapInfo), DIB_RGB_COLORS)
		        >> checkWin32Result("SetDIBIts", errorValue, false);
	}

	template<typename ValueType>
	std::vector<ValueType> loadBinaryDataResource(HMODULE module, LPCWSTR resource, LPCWSTR type) {
		auto resourceInformation = FindResourceW(module, resource, type)
		                           >> checkWin32Result("FindResourceW", errorValue, nullptr);
		auto resourceHandle = LoadResource(module, resourceInformation)
		                      >> checkWin32Result("LoadResource", errorValue, nullptr);
		auto data = static_cast<const ValueType*>(LockResource(resourceHandle))
		            >> checkWin32Result("LockResource", errorValue, nullptr);
		auto bytes = SizeofResource(module, resourceInformation)
		            >> checkWin32Result("SizeofResource", errorValue, 0);

		auto buffer = std::vector<ValueType> {};
		auto count = bytes / sizeof(ValueType);
		buffer.resize(count);
		std::copy_n(data, count, std::begin(buffer));
		return buffer;
	}

	struct HandleDeleter {
		using pointer = HANDLE;
		void operator()(pointer handle) const noexcept {
			CloseHandle(handle);
		}
	};

	using Handle = std::unique_ptr<HANDLE, HandleDeleter>;

	inline Handle createFile(const std::wstring& fileName, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition,
	                         DWORD flagAndAttributes = FILE_ATTRIBUTE_NORMAL, LPSECURITY_ATTRIBUTES securityAttributes = nullptr, HANDLE fileTemplate = nullptr) {
		return Handle { CreateFileW(fileName.c_str(), desiredAccess, shareMode, securityAttributes,
		                            creationDisposition, flagAndAttributes, fileTemplate)
		                >> checkWin32Result(("CreateFileW [" + toBytes(fileName) + ']').c_str(), errorValue, INVALID_HANDLE_VALUE) };
	}

	inline LONGLONG getFileSize(HANDLE fileHandle) {
		auto fileSize = LARGE_INTEGER{};
		GetFileSizeEx(fileHandle, &fileSize) >> checkWin32Result("GetFileSizeEx", errorValue, false);
		return fileSize.QuadPart;
	}

	inline LONGLONG setFilePointer(HANDLE fileHandle, LONGLONG distanceToMove, DWORD moveMethod) {
		auto distanceAsLargeInteger = LARGE_INTEGER{};
		distanceAsLargeInteger.QuadPart = distanceToMove;
		auto result = LARGE_INTEGER{};
		SetFilePointerEx(fileHandle, distanceAsLargeInteger, &result, moveMethod) >> checkWin32Result("SetFilePointerEx", errorValue, false);
		return result.QuadPart;
	}

	inline bool isDirectory(const std::wstring& path) {
		auto attributes = GetFileAttributesW(path.c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES) and (attributes bitand FILE_ATTRIBUTE_DIRECTORY);
	}

	inline bool fileExists(const std::wstring& fileName) {
		auto attributes = GetFileAttributesW(fileName.c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES) and not (attributes bitand FILE_ATTRIBUTE_DIRECTORY);
	}

	struct FindCloser {
		using pointer = HANDLE;
		void operator()(pointer handle) const noexcept {
			FindClose(handle);
		}
	};

	using FindHandle = std::unique_ptr<HANDLE, FindCloser>;

	template<typename Predicate>
	inline std::vector<std::wstring> findAllMatching(const std::wstring& path, Predicate predicate) {
		auto data = WIN32_FIND_DATAW{};
		auto rawHandle = FindFirstFileW(path.c_str(), &data);
		auto lastError = GetLastError();
		if(rawHandle == INVALID_HANDLE_VALUE and lastError == ERROR_FILE_NOT_FOUND) {
			return {};
		}
		auto handle = FindHandle {rawHandle >> checkWin32Result(("FindFirstFile [" + toBytes(path) + ']').c_str(), errorValue, INVALID_HANDLE_VALUE, lastError)};

		auto fileNames = std::vector<std::wstring> {};
		auto nextFileExist = false;
		do {
			if(predicate(data)) {
				fileNames.emplace_back(data.cFileName);
			}

			nextFileExist = FindNextFileW(handle.get(), &data);
			lastError = GetLastError();
			if(lastError != ERROR_NO_MORE_FILES) {
				nextFileExist >> checkWin32Result("FindNextFile", errorValue, false, lastError);
			}
		}
		while(nextFileExist);
		return fileNames;
	}

	inline std::vector<std::wstring> findAllMatchingFiles(const std::wstring& path) {
		return findAllMatching(path, [](const WIN32_FIND_DATAW& data) { return data.dwFileAttributes xor FILE_ATTRIBUTE_DIRECTORY; });
	}

	inline std::vector<std::wstring> findAllMatchingDirectories(const std::wstring& path) {
		return findAllMatching(path, [](const WIN32_FIND_DATAW& data) { return data.dwFileAttributes bitand FILE_ATTRIBUTE_DIRECTORY; });
	}

	inline void appendToFolder(std::wstring& folder, std::wstring_view subPath) {
		if(not folder.empty() and PathGetCharType(folder.back()) != GCT_SEPARATOR) {
			folder += L'\\';
		}
		folder += subPath;
	}

	inline std::wstring concatenatePath(std::wstring folder, std::wstring_view subPath) {
		appendToFolder(folder, subPath);
		return folder;
	}

	inline void shellExecute(HWND associatedWindow, LPCWSTR operation, const std::wstring& file, int showMode = SW_SHOWNORMAL,
	                         LPCWSTR parameters = nullptr, LPCWSTR directory = nullptr) {

		auto info = SHELLEXECUTEINFOW{sizeof(SHELLEXECUTEINFOW)};
		info.fMask = SEE_MASK_DEFAULT;
		info.hwnd = associatedWindow;
		info.lpVerb = operation;
		info.lpFile = file.c_str();
		info.lpParameters = parameters;
		info.lpDirectory = directory;
		info.nShow = showMode;
		ShellExecuteExW(&info) >> checkWin32Result("ShellExecuteExW", errorValue, false);
	}

	inline Handle createProcess(std::wstring commandLine, LPCWSTR currentDirectory,
	                            DWORD creationFlags = 0, bool inheritHandles = false, LPVOID environments = nullptr, LPSTARTUPINFOW startUpInfo = nullptr,
	                            LPSECURITY_ATTRIBUTES processAttributes = nullptr, LPSECURITY_ATTRIBUTES threadAttribtues = nullptr) {
		auto defaultStartUpInfo = STARTUPINFOW{sizeof(STARTUPINFOW)};
		if(startUpInfo == nullptr) {
			startUpInfo = &defaultStartUpInfo;
		}
		auto processInformation = PROCESS_INFORMATION{};
		CreateProcessW(nullptr, commandLine.data(), processAttributes, threadAttribtues,
		               inheritHandles, creationFlags, environments, currentDirectory, startUpInfo, &processInformation)
		        >> checkWin32Result("CreateProcessW", errorValue, false);
		CloseHandle(processInformation.hThread);
		return Handle{processInformation.hProcess};
	}

	struct RegistryKeyCloser {
		using pointer = HKEY;
		void operator()(pointer keyHandle) const noexcept {
			RegCloseKey(keyHandle);
		}
	};

	using RegistryKey = std::unique_ptr<HKEY, RegistryKeyCloser>;

	inline RegistryKey openRegistryKey(HKEY root, const std::wstring& path, REGSAM securityAccessMask = KEY_READ) {
		auto rawHandle = HKEY{nullptr};
		RegOpenKeyExW(root, path.c_str(), 0, securityAccessMask, &rawHandle)
		        >> checkWin32Result("RegOpenKeyExW", successValue, ERROR_SUCCESS, resultIsErrorCode);
		return RegistryKey{rawHandle};
	}

	inline std::wstring getRegistryString(HKEY registryKey, const std::wstring& valueName) {
		auto valueType = DWORD{};
		auto bufferBytes = DWORD{};
		RegQueryValueExW(registryKey, valueName.c_str(), nullptr, &valueType, nullptr, &bufferBytes)
		        >> checkWin32Result("RegGetValue (retrieving buffer size)", successValue, ERROR_SUCCESS, resultIsErrorCode);

		if(valueType != REG_SZ) {
			throw std::invalid_argument("getRegistryString() failed: value type may not be string");
		}

		if(bufferBytes % sizeof(wchar_t) != 0) {
			throw std::invalid_argument("getRegistryString() failed: value type may not be string, bufferBytes % sizeof(wchar_t) != 0");
		}

		auto result = std::wstring{bufferBytes / sizeof(wchar_t), {}, std::wstring::allocator_type{}};

		RegQueryValueExW(registryKey, valueName.c_str(), nullptr, &valueType, reinterpret_cast<LPBYTE>(result.data()), &bufferBytes)
		        >> checkWin32Result("RegGetValue", successValue, ERROR_SUCCESS, resultIsErrorCode);

		if(valueType != REG_SZ) {
			throw std::invalid_argument("getRegistryString() failed: value type may not be string");
		}

		if(bufferBytes != (result.size() * sizeof(wchar_t))) {
			throw std::runtime_error("getRegistryString() failed, bufferBytes != result.size() * sizeof(wchar_t)");
		}

		constexpr auto nulLength = 1;
		if(result.size() >= nulLength and result.back() == '\0') {
			result.pop_back();
		}

		return result;
	}

	inline void setRegistryString(HKEY registryKey, const std::wstring& valueName, const std::wstring& value) {
		RegSetValueExW(registryKey, valueName.c_str(), 0, REG_SZ,
		               reinterpret_cast<const BYTE*>(value.c_str()), value.size() * sizeof(wchar_t))
		        >> checkWin32Result("RegSetValueEx", successValue, ERROR_SUCCESS, resultIsErrorCode);
	}

	template<typename T>
	FILETIME unixTimeToFileTime(T unixTimeStamp) {
		auto windowsTimeStamp = static_cast<LONGLONG>(unixTimeStamp) * 10000000 + 116444736000000000;
		auto fileTime = FILETIME{};
		fileTime.dwLowDateTime = static_cast<DWORD>(windowsTimeStamp);
		fileTime.dwHighDateTime = windowsTimeStamp >> 32;
		return fileTime;
	}
}

