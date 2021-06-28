//Simple CSF Parser
//In Red Alert 3, *.csf files are used to store strings.
//This header provides a function capable of parsing this kind of file.
//Some other C&C Games uses CSF files too, but I didn't investigate them,
//so I can't be sure that their formats are the same as RA3's one.
//This file requires another header 'Input.hpp',
//which provides some utility functions used to
//read binary data from a pair of iterators.
// - Lanyi, 2018

#pragma once

#include <cstdint>
#include <utility>
#include <string>
#include <string_view>
#include <stdexcept>
#include "Input.hpp"

namespace MyCSF {

	//Read CSF into a container using a user-provided CSFSringEmplacer.
	//CSFSringEmplacer accepts a reference to the destination container and
	//a std::pair<std::string, std::wstring>, which is the CSF label-string pair.
	//Example:
	//    using namespace std;
	//    using namespace placeholders;
	//    using ValueType = pair<string, wstring>;
	//    auto emplacer = bind(&vector<ValueType>::emplace_back<ValueType>, _1, _2);
	//    auto file = ifstream{fileName, ifstream::binary};
	//    auto strings = readCSF<vector<ValueType>>(Input::Range{istreambuf_iterator<char>{file}, istreambuf_iterator<char>{}}, emplacer);
	//    for(const auto& [label, string] : strings) {
	//        ...
	//    }
	//This function asssumes that machine's endianness is little-endian, and sizeof(wchar_t) == 2.
	template<typename Container, typename CSFStringEmplacer, typename Range>
	Container readCSF(Range&& csf, CSFStringEmplacer csfStringEmplacer = CSFStringEmplacer{});

	//Write CSF from a ForwardIterator Range with an OutputIterator.
	//ForwardIterator should be dereferencable into something that after structured binding,
	//could be converted to std::string (label) and std::wstring (text)
	//Example:
	//    writeCSF(Input::Range{vector.begin(), vector.end()}, std::ostreambuf_iterator{file});
	//This function asssumes that machine's endianness is little-endian, and sizeof(wchar_t) == 2.
	template<typename ForwardIteratorRange, typename OutputIterator>
	OutputIterator writeCSF(ForwardIteratorRange stringsToBeWritten, OutputIterator output);

	/*
		CSF file format

		struct CSF {
			char[4];                     //string " FSC"
			uint32_t;                    //unknown 0x00000003 in little endian, type / version number?
			uint32_t;                    //array size in little endian
			uint32_t;                    //array size in little endian
			uint32_t;                    //unknown zero, probably high-order bytes of array size?
			uint32_t;                    //unknown zero, probably high-order bytes of array size?
			LabbeledString[array size];  //array of strings with label
		}

		struct LabbeledString {
			char[4];                //string " LBL"
			uint32_t;               //unknown 0x00000001 in little endian, type / version number?
			uint32_t;               //label size in little endian
			char[label size];       //string label name
			char[4];                //string " RTS"
			uint32_t;               //string size
			char16_t[string size];  //masked UTF-16 LE string: realChar = 0xFFFF ^ sourceChar
		}
	*/

	//This output iterator can be used to calculate the required size of an output buffer
	struct OutputCounter {
		using value_type = void;
		using difference_type = void;
		using pointer = void;
		using reference = void;
		using iterator_category = std::output_iterator_tag;
		struct NoOpAssignee {
			template<typename T> constexpr void operator=(T&&) const noexcept { }
		};

		NoOpAssignee operator*() const noexcept { return this->assignee; }
		OutputCounter& operator++() { ++(this->counter); return *this; }
		OutputCounter operator++(int) { auto copy = *this; ++(*this); return copy; }

		int counter = 0;
		NoOpAssignee assignee;
	};

	namespace Details {
		using namespace std::string_view_literals;
		static constexpr auto header = std::string_view{ " FSC" "\x03\x00\x00\x00"sv };
		static constexpr auto zero32Bit = std::string_view{ "\x00\x00\x00\x00"sv };
		static constexpr auto lbl = std::string_view{ " LBL" "\x01\x00\x00\x00"sv };
		static constexpr auto rts = std::string_view{ " RTS"sv };

		using namespace Input;

		template<typename InputIterator>
		std::pair<std::string, std::wstring> nextString(Range<InputIterator>& input) {

			readAndCheckMagic(input, lbl);

			auto labelSize = copyBytes<std::uint32_t>(input);
			auto label = std::string{ labelSize, '\0', std::string::allocator_type{} };
			copyFixed(input, label.begin(), label.size());

			readAndCheckMagic(input, rts);

			auto wideCharCount = copyBytes<std::uint32_t>(input);
			auto string = std::wstring{ wideCharCount, L'\xFFFF', std::wstring::allocator_type{} };
			for (auto& wideChar : string) { wideChar = wideChar xor copyBytes<wchar_t>(input); }

			return { std::move(label), std::move(string) };
		}

		template<typename OutputIterator, typename T>
		OutputIterator writeAsBytes(OutputIterator out, const T& data, std::size_t bytes = sizeof(T)) {
			return std::copy_n(reinterpret_cast<const char*>(&data), bytes, out);
		}

		template<typename OutputIterator>
		OutputIterator writeString(const std::string& label, std::wstring string, OutputIterator out) {

			out = writeAsBytes(out, *(lbl.data()), lbl.size());
			out = writeAsBytes(out, label.size());
			out = writeAsBytes(out, *(label.data()), label.size());

			out = writeAsBytes(out, *(rts.data()), rts.size());
			out = writeAsBytes(out, string.size());
			for (auto& character : string) character = character xor L'\xFFFF';
			return writeAsBytes(out, *(string.data()), sizeof(*string.data()) * string.length());
		}
	}

	template<typename Container, typename CSFStringEmplacer, typename Range>
	Container readCSF(Range&& csfRange, CSFStringEmplacer csfStringEmplacer) {
		using namespace Details;

		readAndCheckMagic(csfRange, header);

		auto stringCount1 = copyBytes<std::uint32_t>(csfRange);
		auto stringCount2 = copyBytes<std::uint32_t>(csfRange);

		if (stringCount1 != stringCount2) {
			throw std::invalid_argument("May not be able to correctly parse CSF file");
		}

		readAndCheckMagic(csfRange, zero32Bit);
		readAndCheckMagic(csfRange, zero32Bit);

		auto map = Container{};

		for (auto i = std::uint32_t{ 0 }; i < stringCount1; ++i) {
			csfStringEmplacer(map, nextString(csfRange));
		}

		//ensure EOF is reached;
		if (csfRange.current != csfRange.end) {
			throw std::invalid_argument("CSF EOF not reached as expected");
		}

		return map;
	}

	template<typename ForwardIteratorRange, typename OutputIterator>
	OutputIterator writeCSF(ForwardIteratorRange stringsToBeWritten, OutputIterator out) {
		using namespace Details;

		auto count = std::distance(stringsToBeWritten.current, stringsToBeWritten.end);

		out = writeAsBytes(out, *(header.data()), header.size());
		out = writeAsBytes(out, count);
		out = writeAsBytes(out, count);
		out = writeAsBytes(out, *(zero32Bit.data()), zero32Bit.size());
		out = writeAsBytes(out, *(zero32Bit.data()), zero32Bit.size());

		for (const auto& [label, string] : stringsToBeWritten.rangeForLoop()) {
			out = writeString(label, string, out);
		}
		return out;
	}

}
