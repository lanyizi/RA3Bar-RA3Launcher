#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include "WindowsWrapper.hpp"
#include <Shlobj.h>
#include <Shlwapi.h>
#include "Input.hpp"
#include "Common.hpp"

//Common.hpp
template<typename T>
void readFile(HANDLE file, std::vector<T>& buffer, std::size_t count);
template<typename T>
std::vector<T> readFile(HANDLE file, std::size_t count);
template<typename InputIterator1, typename InputIterator2>
bool insensitiveEqual(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2);

namespace ReplaysAndMods {

	struct ReplayDetails {
		std::wstring fullPath;
		std::wstring replayName;
		std::optional<std::uint32_t> finalTimeCode;
		std::wstring modName;
		std::wstring modVersion;
		std::pair<std::uint32_t, std::uint32_t> gameVersion;
		std::uint32_t timeStamp;
		std::wstring title;
		std::wstring map;
		std::vector<std::wstring> players;
		std::wstring description;
		bool hasCommentator;
	};

	struct ModDetails {
		std::wstring fullPath;
		std::wstring modName;
		std::wstring version;
	};


	template<typename Range>
	ReplayDetails parseReplayHeader(Range&& replay);
	template<typename Range>
	std::vector<char> fixReplay(Range&& replay);
	template<typename Range>
	std::optional<std::uint32_t> getFinalTimeCodeFromLastBytes(Range&& terminatorAndFooter);
	inline ReplayDetails getReplayDetails(const std::wstring& replayFullPath);
	inline std::vector<ReplayDetails> getAllReplayDetails();
	inline std::vector<ModDetails> getModSkudefs();
	inline std::wstring concatenateWithReplayFolder(std::wstring_view replay);
	inline std::wstring concatenateWithModRootFolder(std::wstring_view mod);
	inline std::optional<std::vector<std::wstring>> getModSkudefPathsFromReplay(const ReplayDetails& replay);

	inline const auto replayExtension = std::wstring {L".ra3replay"};
	inline const auto skudefExtension = std::wstring {L".skudef"};
	inline constexpr auto replayHeaderMagic = std::string_view {"RA3 REPLAY HEADER"};
	namespace Internal {
		using namespace Windows;
		using namespace Input;
		using namespace std::string_view_literals;

		inline constexpr auto cncMagic = std::string_view {"CNC3RPL\0"sv};
		inline constexpr auto terminator = std::string_view {"\xFF\xFF\xFF\x7F"sv};
		inline constexpr auto footerMagic = std::string_view {"RA3 REPLAY FOOTER"sv};
		inline constexpr auto prePlainTextPadding = 31;

		inline const std::wstring wildcardAny = L"*";


		inline std::wstring getRA3UserFolder(std::wstring_view subDirectory) {
			auto userDataLeafName = std::wstring{L"Red Alert 3"};

			try {
				auto redAlert3Registry = openRegistryKey(HKEY_LOCAL_MACHINE, L"Software\\Electronic Arts\\Electronic Arts\\Red Alert 3");
				userDataLeafName = getRegistryString(redAlert3Registry.get(), L"UserDataLeafName");
			}
			catch(...) { }

			auto ra3UserPathName = std::wstring{MAX_PATH, {}, std::wstring::allocator_type{}};
			auto getPathResult =
			    SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS|CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, ra3UserPathName.data());
			if(getPathResult != S_OK) {
				throw std::runtime_error("SHGetFolderPathAndSubDir failed, error code " + std::to_string(getPathResult));
			}
			ra3UserPathName.resize(std::min(ra3UserPathName.size(), ra3UserPathName.find(L'\0')));

			appendToFolder(ra3UserPathName, userDataLeafName);
			if(not isDirectory(ra3UserPathName)) {
				CreateDirectoryW(ra3UserPathName.c_str(), nullptr) >> checkWin32Result("CreateDirectoryW", errorValue, false);
			}

			appendToFolder(ra3UserPathName, subDirectory);
			if(not isDirectory(ra3UserPathName)) {
				CreateDirectoryW(ra3UserPathName.c_str(), nullptr) >> checkWin32Result("CreateDirectoryW", errorValue, false);
			}

			return ra3UserPathName;
		}

		template<typename InputIterator>
		std::wstring readNullTerminatedWideString(Range<InputIterator>& input) {
			auto result = std::wstring {};
			do {
				result += copyBytes<wchar_t>(input);
			}
			while(result.back() != L'\0');
			result.pop_back();
			return result;
		}
	}

	std::wstring concatenateWithReplayFolder(std::wstring_view replay) {
		using namespace Internal;
		auto replaysFolder = std::wstring{L"Replays"};

		try {
			auto redAlert3Registry = openRegistryKey(HKEY_LOCAL_MACHINE, L"Software\\Electronic Arts\\Electronic Arts\\Red Alert 3");
			replaysFolder = getRegistryString(redAlert3Registry.get(), L"ReplayFolderName");
		}
		catch(...) { }

		return concatenatePath(getRA3UserFolder(replaysFolder), replay);
	}

	std::wstring concatenateWithModRootFolder(std::wstring_view mod) {
		using namespace Internal;
		return concatenatePath(getRA3UserFolder(L"Mods"), mod);
	}

	template<typename Range>
	ReplayDetails parseReplayHeader(Range&& replay) {
		using namespace Internal;

		readAndCheckMagic(replay, replayHeaderMagic);

		auto hNumber = std::to_integer<unsigned>(copyBytes<std::byte>(replay));

		auto majorVersion = copyBytes<std::uint32_t>(replay);
		auto minorVersion = copyBytes<std::uint32_t>(replay);
		auto gameVersion = std::pair{majorVersion, minorVersion};

		ignore<std::uint32_t>(replay); //build major
		ignore<std::uint32_t>(replay); //build minor
		ignore<std::byte>(replay);  //commentary track flag?
		ignore<std::byte>(replay);  //zero

		auto title = readNullTerminatedWideString(replay);
		auto description = readNullTerminatedWideString(replay);
		auto mapName = readNullTerminatedWideString(replay);
		auto mapID = readNullTerminatedWideString(replay);

		auto numberOfPlayers = std::to_integer<std::size_t>(copyBytes<std::byte>(replay));

		auto playerNames = std::vector<std::wstring> {numberOfPlayers + 1, {}, std::vector<std::wstring>::allocator_type{}};
		for(auto& playerName : playerNames) {
			ignore<std::uint32_t>(replay); //skip player id
			playerName = readNullTerminatedWideString(replay);
			if(hNumber == 0x05u) {
				ignore<std::byte>(replay); //skip team number
			}
		}
		playerNames.pop_back();

		auto offset = copyBytes<std::uint32_t>(replay);

		if(copyBytes<std::uint32_t>(replay) != cncMagic.size()) {
			throw std::invalid_argument("incorrect CNC3RPL magic length");
		}
		readAndCheckMagic(replay, cncMagic);

		constexpr auto modInfoSize = std::size_t{22};
		auto modInfo = std::string{modInfoSize, {}, std::string::allocator_type{}};
		copyFixed(replay, modInfo.begin(), modInfo.size());
		auto modVersion = modInfo.substr(modInfo.find_last_of(L'\0', modInfo.find_last_not_of(L'\0')) + 1);
		modInfo.resize(std::min(modInfo.size(), modInfo.find(L'\0')));
		modVersion.resize(std::min(modVersion.size(), modVersion.find(L'\0')));

		auto timeStamp = copyBytes<std::uint32_t>(replay);

		ignore(replay, prePlainTextPadding);
		auto plainTextLength = copyBytes<std::uint32_t>(replay);
		auto plainText = std::string{plainTextLength, {}, std::string::allocator_type{}};
		copyFixed(replay, plainText.begin(), plainTextLength);

		auto hasCommentator = (plainText.rfind(":Hpost Commentator") != plainText.npos);

		auto afterOffset = cncMagic.size() + modInfo.size() + sizeof(timeStamp)
		                   + prePlainTextPadding + sizeof(plainTextLength) + plainText.size();
		ignore(replay, offset - afterOffset);

		using std::move;
		return {{}, {}, std::nullopt, toWide(modInfo), toWide(modVersion), gameVersion, timeStamp,
			move(title), move(mapName), move(playerNames), move(description), hasCommentator};
	}

	template<typename Range>
	std::vector<char> fixReplay(Range&& replay) {
		using namespace Internal;
		auto enlargeAndGetIterator = [](std::vector<char>& buffer, std::size_t numberOfNewBytesToAdd) {
			std::size_t oldSize = buffer.size();
			buffer.resize(oldSize + numberOfNewBytesToAdd);
			return std::next(std::begin(buffer), oldSize);
		};

		auto replayData = std::vector<char> {};

		readAndCheckMagic(replay, replayHeaderMagic);
		std::copy(std::begin(replayHeaderMagic), std::end(replayHeaderMagic),
		          enlargeAndGetIterator(replayData, replayHeaderMagic.size()));

		auto hNumber = copyBytes<char>(replay);
		replayData.emplace_back(hNumber);

		auto versionNumbersAndFlagsSize = sizeof(std::uint32_t) * 4 + sizeof(char) * 2;
		copyFixed(replay, enlargeAndGetIterator(replayData, versionNumbersAndFlagsSize), versionNumbersAndFlagsSize);

		auto copyWideStringAsCharArray = [enlargeAndGetIterator](std::wstring_view string, std::vector<char>& buffer) {
			auto begin = reinterpret_cast<const char*>(string.data());
			auto bytesToCopy = string.size() * sizeof(*string.data());
			return std::copy_n(begin, bytesToCopy, enlargeAndGetIterator(buffer, bytesToCopy));
		};

		for(auto i = 0; i < 4; ++i) {
			copyWideStringAsCharArray(readNullTerminatedWideString(replay) + L'\0', replayData);
		}

		auto numberOfPlayers = copyBytes<unsigned char>(replay);
		replayData.emplace_back(static_cast<char>(numberOfPlayers));

		for(auto i = 0; i < numberOfPlayers + 1; ++i) {
			copyFixed(replay, enlargeAndGetIterator(replayData, sizeof(std::uint32_t)), sizeof(std::uint32_t));
			copyWideStringAsCharArray(readNullTerminatedWideString(replay) + L'\0', replayData);
			if(hNumber == 0x05) {
				replayData.emplace_back(copyBytes<char>(replay));
			}
		}

		auto offset = copyBytes<std::uint32_t>(replay);
		std::copy_n(reinterpret_cast<const char*>(&offset), sizeof(offset),
		            enlargeAndGetIterator(replayData, sizeof(offset)));

		copyFixed(replay, enlargeAndGetIterator(replayData, sizeof(std::uint32_t)), sizeof(std::uint32_t));
		readAndCheckMagic(replay, cncMagic);
		std::copy(std::begin(cncMagic), std::end(cncMagic), enlargeAndGetIterator(replayData, cncMagic.size()));

		copyFixed(replay, enlargeAndGetIterator(replayData, offset - cncMagic.size()), offset - cncMagic.size());

		auto lastTimeCode = std::array<char, sizeof(uint32_t)> {};
		try {

			while(true) {
				auto chunk = std::vector<char> {};

				auto chunkTimeCode = decltype(lastTimeCode) {};
				copyFixed(replay, std::begin(chunkTimeCode), chunkTimeCode.size());
				if(std::equal(std::begin(chunkTimeCode), std::end(chunkTimeCode),
				              std::begin(terminator), std::end(terminator))) {
					break;
				}

				std::copy(std::begin(chunkTimeCode), std::end(chunkTimeCode),
				          enlargeAndGetIterator(chunk, chunkTimeCode.size()));

				chunk.emplace_back(copyBytes<char>(replay)); //chunk type

				auto chunkSize = copyBytes<std::uint32_t>(replay);
				std::copy_n(reinterpret_cast<const char*>(&chunkSize), sizeof(chunkSize),
				            enlargeAndGetIterator(chunk, sizeof(chunkSize)));
				copyFixed(replay, enlargeAndGetIterator(chunk, chunkSize), chunkSize);

				constexpr auto zeroes = std::string_view{"\0\0\0\0"sv};
				readAndCheckMagic(replay, zeroes);
				std::copy(std::begin(zeroes), std::end(zeroes), enlargeAndGetIterator(chunk, zeroes.size()));

				lastTimeCode = chunkTimeCode;
				std::copy(std::begin(chunk), std::end(chunk), enlargeAndGetIterator(replayData, chunk.size()));
			}

			auto footer = std::vector<char> {};

			std::copy(std::begin(terminator), std::end(terminator), enlargeAndGetIterator(footer, terminator.size()));
			std::copy(replay.current, replay.end, std::back_inserter(footer));

			auto finalTimeCode = getFinalTimeCodeFromLastBytes(Range{std::begin(footer), std::end(footer)}).value();
			std::copy_n(reinterpret_cast<const char*>(&finalTimeCode), lastTimeCode.size(), std::begin(lastTimeCode));

			std::copy(std::begin(footer), std::end(footer), enlargeAndGetIterator(replayData, footer.size()));
		}
		catch(...) {
			auto myFooter = std::vector<char> {};

			std::copy(std::begin(terminator), std::end(terminator), enlargeAndGetIterator(myFooter, terminator.size()));
			std::copy(std::begin(footerMagic), std::end(footerMagic), enlargeAndGetIterator(myFooter, footerMagic.size()));
			std::copy(std::begin(lastTimeCode), std::end(lastTimeCode), enlargeAndGetIterator(myFooter, lastTimeCode.size()));
			constexpr auto finalData = std::string_view{"\x02\x1A\x00\x00\x00"sv};
			std::copy(std::begin(finalData), std::end(finalData), enlargeAndGetIterator(myFooter, finalData.size()));
			auto footerLength = myFooter.size();
			std::copy_n(reinterpret_cast<const char*>(&footerLength), sizeof(footerLength), enlargeAndGetIterator(myFooter, sizeof(footerLength)));

			std::copy(std::begin(myFooter), std::end(myFooter), enlargeAndGetIterator(replayData, myFooter.size()));
		}

		return replayData;
	}

	template<typename Range>
	std::optional<std::uint32_t> getFinalTimeCodeFromLastBytes(Range&& terminatorAndFooter) {
		using namespace Internal;
		try {
			readAndCheckMagic(terminatorAndFooter, terminator);
			readAndCheckMagic(terminatorAndFooter, footerMagic);
			auto finalTimeCode = copyBytes<std::uint32_t>(terminatorAndFooter);

			auto remainedBytes = std::vector<char> {};
			std::copy(terminatorAndFooter.current, terminatorAndFooter.end, std::back_inserter(remainedBytes));
			if(remainedBytes.size() < sizeof(std::uint32_t)) {
				throw std::out_of_range("remainedBytes.size() < sizeof(std::uint32_t)");
			}
			auto footerLengthRange = Range{std::end(remainedBytes) - sizeof(std::uint32_t), std::end(remainedBytes)};
			auto footerLength = copyBytes<std::uint32_t>(footerLengthRange);
			if((footerLength - footerMagic.size() - sizeof(finalTimeCode)) != remainedBytes.size()) {
				throw std::invalid_argument("Incorrect footer length");
			}
			return finalTimeCode;
		}
		catch(...) { }
		return std::nullopt;
	}

	ReplayDetails getReplayDetails(const std::wstring& replayFullPath) {
		using namespace Internal;
		auto file = createFile(replayFullPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
		auto buffer = std::vector<char> {};
		auto replayDetails = ReplayDetails{};
		while(true) {
			auto fileSize = getFileSize(file.get());
			readFile(file.get(), buffer, std::clamp<std::size_t>(buffer.size() * 2, 1024, fileSize - buffer.size()));
			try {
				replayDetails = parseReplayHeader(Range{std::begin(buffer), std::end(buffer)});
				break;
			}
			catch(const RangeException&) {
				if(buffer.size() >= fileSize) {
					throw;
				}
			}
		}
		replayDetails.fullPath = std::move(replayFullPath);

		auto footerLength = std::uint32_t{};
		setFilePointer(file.get(), -static_cast<LONGLONG>(sizeof(footerLength)), FILE_END);
		auto lengthBuffer = readFile<char>(file.get(), sizeof(std::uint32_t));
		std::copy(std::begin(lengthBuffer), std::end(lengthBuffer), reinterpret_cast<char*>(&footerLength));

		try {
			auto terminatorAndFooterLength = footerLength + sizeof(std::uint32_t);
			setFilePointer(file.get(), -static_cast<LONGLONG>(terminatorAndFooterLength), FILE_END);
			auto lastBytes = readFile<char>(file.get(), terminatorAndFooterLength);
			replayDetails.finalTimeCode = getFinalTimeCodeFromLastBytes(Range{std::begin(lastBytes), std::end(lastBytes)});
		}
		catch(...) { }

		return replayDetails;
	}

	std::vector<ReplayDetails> getAllReplayDetails() {
		using namespace Internal;
		auto replayPath = concatenateWithReplayFolder({});
		auto allReplays = findAllMatchingFiles(concatenatePath(replayPath, wildcardAny + replayExtension));
		auto replayDetails = std::vector<ReplayDetails> {};
		for(auto& fileName : allReplays) {
			try {
				replayDetails.emplace_back(getReplayDetails(concatenatePath(replayPath, fileName)));
				replayDetails.back().replayName = std::move(fileName);
			}
			catch(...) { /* simply skip unparsable replays */ }
		}
		return replayDetails;
	}

	std::vector<ModDetails> getModSkudefs() {
		using namespace Internal;
		auto modRoot = concatenateWithModRootFolder({});
		auto allModFolders = findAllMatchingDirectories(concatenatePath(modRoot, wildcardAny));
		auto modDetails = std::vector<ModDetails> {};
		for(const auto& mod : allModFolders) {
			auto modFolder = concatenatePath(modRoot, mod);
			auto skudefs = findAllMatchingFiles(concatenatePath(modFolder, wildcardAny + skudefExtension));
			for(const auto& skudef : skudefs) {
				auto modName = skudef.substr(0, skudef.find(L'_'));
				auto version = skudef.substr(skudef.find(L'_') + 1);
				version.erase(version.find_last_of(L'.'));
				modDetails.emplace_back(ModDetails{concatenatePath(modFolder, skudef), std::move(modName), std::move(version)});
			}
		}
		return modDetails;
	}

	std::optional<std::vector<std::wstring>> getModSkudefPathsFromReplay(const ReplayDetails& replay) {
		using namespace Internal;
		constexpr auto ra3 = std::wstring_view{L"ra3"};
		const auto& modName = replay.modName;
		if(insensitiveEqual(std::begin(modName), std::end(modName), std::begin(ra3), std::end(ra3))) {
			return std::nullopt;
		}
		auto skudefFileName = modName + L"_" + replay.modVersion + skudefExtension;
		auto modRoot = concatenateWithModRootFolder({});
		auto modFolders = findAllMatchingDirectories(concatenatePath(modRoot, wildcardAny));
		auto mods = std::vector<std::wstring> {};
		for(const auto& modFolder : modFolders) {
			auto requiredFileName = concatenatePath(concatenatePath(modRoot, modFolder), skudefFileName);
			if(fileExists(requiredFileName)) {
				mods.emplace_back(std::move(requiredFileName));
			}
		}
		return mods;
	}
}
