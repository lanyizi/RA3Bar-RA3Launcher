//Some utility functions used to
//read binary data from a pair of iterators.
#pragma once

#include <cstddef>
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Input {
	template<typename Iterator>
	struct Range {
		using Category = typename std::iterator_traits<Iterator>::iterator_category;
		Range(const Iterator& current, const Iterator& end) : current{current}, end{end} { }
		Iterator current;
		Iterator end;
	};

	struct RangeException : std::out_of_range {
		using std::out_of_range::out_of_range;
	};

	//return the [begin + magicSize, end] on success
	template<typename InputIterator>
	void readAndCheckMagic(Range<InputIterator>& range, std::string_view magic) {
		auto [magicEnd, current] = std::mismatch(magic.begin(), magic.end(), range.current, range.end);
		if(magicEnd != magic.end()) {
			if(current == range.end) {
				throw RangeException("Iterator reached end before compare finishes.");
			}
			constexpr auto toHex = [](unsigned char byte) -> char { byte = byte bitand 0xF; return byte < 0xA ? byte + '0' : byte - 0xA + 'A'; };
			auto printable = std::string{};
			for(auto byte : magic) { printable += isprint(byte) ? std::string{byte} : std::string{'\\', 'x', toHex(byte >> 4), toHex(byte)}; }
			throw std::invalid_argument("May not be able to parse file correctly, \"" + printable + "\" not found.");
		}
		range.current = current;
	}

	template<typename InputIterator, typename OutputIterator>
	void copyFixed(Range<InputIterator>& range, OutputIterator outputBegin, typename std::iterator_traits<InputIterator>::difference_type count) {
		if constexpr(std::is_base_of_v<std::forward_iterator_tag, typename Range<InputIterator>::Category>) {
			if(std::distance(range.current, range.end) < count) {
				throw RangeException("ForwardIterator will reach end before copy finishes");
			}
			auto afterCopy = std::next(range.current, count);
			std::copy(range.current, afterCopy, outputBegin);
			range.current = afterCopy;
		}
		else {
			for(auto k = count; k != 0; --k) {
				if(range.current == range.end) {
					throw RangeException("InputIterator reached end before copy finishes");
				}
				*outputBegin = *range.current;
				++range.current, ++outputBegin;
			}
		}
	}

	template<typename InputIterator>
	void ignore(Range<InputIterator>& range, std::size_t numberOfBytes) {
		if constexpr(std::is_base_of_v<std::forward_iterator_tag, typename Range<InputIterator>::Category>) {
			auto distance = std::distance(range.current, range.end);
			if((distance < 0) or (static_cast<std::size_t>(distance) < numberOfBytes)) {
				throw RangeException("ForwardIterator will reach end before ignore finishes");
			}
			range.current = std::next(range.current, numberOfBytes);
		}
		else {
			for(auto k = numberOfBytes; k != 0; --k) {
				if(range.current == range.end) {
					throw RangeException("InputIterator reached end before ignore finishes");
				}
				++range.current;
			}
		}
	}

	template<typename T, typename InputIterator>
	void ignore(Range<InputIterator>& range) {
		return ignore(range, sizeof(T));
	}

	template<typename T, typename InputIterator>
	T copyBytes(Range<InputIterator>& input) {
		static_assert(sizeof(char) == sizeof(std::uint8_t) and sizeof(char) == sizeof(std::byte));
		using Type = std::decay_t<typename std::iterator_traits<InputIterator>::value_type>;
		using std::is_same_v;
		static_assert((is_same_v<Type, unsigned char> or is_same_v<Type, char> or is_same_v<Type, std::byte>) and (sizeof(Type) == 1),
		              "cannot use iterator's value_type because of strict aliasing rule!");
		auto value = T{};
		auto valueBegin = reinterpret_cast<Type*>(&value);
		copyFixed(input, valueBegin, sizeof(value));
		return value;
	}
}
