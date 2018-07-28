#ifndef CONNECTED_COMPONENT_UTIL_HPP
#define CONNECTED_COMPONENT_UTIL_HPP

#include <stdexcept>
#include <queue>
#include <utility>
#include <vector>
#include <cmath>
#include <limits>
#include <set>
#include <iterator>

using IntSizeType = int;
using IntPair = std::pair<IntSizeType, IntSizeType>;

template<typename ImgDT>
struct BinaryPredicate {
	// true indicates foreground, false indicates background
	bool operator()(const ImgDT &val) const {
		throw std::runtime_error{ "unsupported type" };
	}
};

template<>
struct BinaryPredicate<bool> {
	// true indicates foreground, false indicates background
	bool operator()(bool val) const {
		return val;
	}
};

template<>
struct BinaryPredicate<char> {
	// true indicates foreground, false indicates background
	bool operator()(char val) const {
		return val;
	}
};

template<typename ImgDT>
struct RoundBracketAccess {
	template<typename ImgType>
	const ImgDT & operator()(const ImgType &img, size_t row, size_t col) const {
		return img(row, col);
	}
};

template<typename ImgDT>
struct SquareBracketAccess {
	template<typename ImgType>
	const ImgDT & operator()(const ImgType &img, size_t row, size_t col) const {
		return img[row][col];
	}
};

struct Quick2DSizeT {
	using size_type = IntSizeType;

	static constexpr const size_type NOLABEL = std::numeric_limits<size_type>::min();

	// filled with zero
	Quick2DSizeT(size_type rows, size_type cols) {
		this->data.resize(rows * cols, NOLABEL);
		this->cols = cols;
	}

	size_type &operator()(size_type row, size_type col) {
		return this->data[row * this->cols + col];
	}

private:
	size_type cols;
	std::vector<size_type> data;
};

struct FourConnect {
	using size_type = IntSizeType;

	std::vector<IntPair> GetNeighbour(const IntPair &pos) const {
		std::vector<IntPair> result(4);
		result[0] = IntPair{pos.first + 1, pos.second};
		result[1] = IntPair{ pos.first, pos.second + 1};
		result[2] = IntPair{ pos.first - 1, pos.second };
		result[3] = IntPair{ pos.first, pos.second -1 };
		return result;
	}

};

struct EightConnect {
	using size_type = IntSizeType;

	std::vector<IntPair> GetNeighbour(const IntPair &pos) const {
		std::vector<IntPair> result;
		result.reserve(8);
		for (size_type i = -1; i < 2; ++i) {
			for (size_type j = -1; j < 2; ++j) {
				if (i == j && i == 0) {
					continue;
				}
				result.push_back(IntPair{pos.first + i, pos.second + j});
			}
		}
		return result;
	}

};

template<typename ImgDT, // the image data type (bool, char, int...)
	typename Connectivity = FourConnect, // determines the connectivity
	typename Access = SquareBracketAccess<ImgDT>, // defines the access method of pixels
	typename BinaryPred = BinaryPredicate<ImgDT>> // determines whether a pixel is foreground/background
	struct ConnectComponentFinder {
	
	using size_type = IntSizeType;
	using value_type = ImgDT;
	using cc_result_type = std::set<IntPair>;
	using result_type = std::vector<cc_result_type>;

	// size: rows, cols
	template<typename ImgType>
	result_type operator()(const ImgType &img, const IntPair &size) const {

		if (size.first < 1 || size.second < 1) {
			throw std::runtime_error{ "invalid size" };
		}

		Quick2DSizeT labelStorage{size.first, size.second};

		size_type currentLabel = labelStart;

		Access access;
		BinaryPred pred;
		Connectivity connect;

		queue<IntPair> myQueue;

		// mark connected components
		for (size_type i = 0; i < size.first; ++i) {
			for (size_type j = 0; j < size.second; ++j) {
				const auto &pixelVal = access(img, i, j);
				auto isForeground = pred(pixelVal);
				bool enteredQueue = false;

				if (isForeground) {
					// check if it has a label
					if (labelStorage(i, j) == Quick2DSizeT::NOLABEL) {
						// does not have a label
						myQueue.push(IntPair{i,j});
						// assign it a label
						labelStorage(i, j) = currentLabel;
						// controls the increment of currentLabel
						enteredQueue = true;
					}

					// repeat while the queue is not empty
					while (!myQueue.empty()) {
						// front has been labeled
						auto front = myQueue.front();
						myQueue.pop();

						// if it is foreground and not labeled
						auto neighbour = move(connect.GetNeighbour(front));
						for (const auto &pixelPos : neighbour) {
							// check if it is valid
							if (this->IsPixelPosValid(size, pixelPos)) {
								
								const auto &neighbourPixelVal = access(img, pixelPos.first, pixelPos.second);
								if (pred(neighbourPixelVal) // check if it is foreground
									&& labelStorage(pixelPos.first, pixelPos.second) == Quick2DSizeT::NOLABEL) {
									// if it has no label, add it into the queue
									myQueue.push(pixelPos);
									// assign it a label
									labelStorage(pixelPos.first, pixelPos.second) = currentLabel;
								}
							}
						}
					}

					if (enteredQueue) {
						currentLabel += 1;
					}
				}

			}
		}

		// compute the number of connected components
		size_type numOfCC = currentLabel - labelStart;
		result_type result;
		result.resize(numOfCC);

		// scan labelStorage for result
		for (size_type i = 0; i < size.first; ++i) {
			for (size_type j = 0; j < size.second; ++j) {
				if (labelStorage(i, j) != Quick2DSizeT::NOLABEL) {
					auto index = labelStorage(i, j) - labelStart;
					result[index].insert(IntPair{i,j});
				}
			}
		}

		return result;
	}

private:

	static constexpr const size_type labelStart = Quick2DSizeT::NOLABEL + 1;

	bool IsPixelPosValid(const IntPair &size, const IntPair &pos) const {
		return pos.first >= 0 && pos.first < size.first && pos.second >= 0 && pos.second < size.second;
	}


};



#endif