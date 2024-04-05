#include "big_integer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <vector>

big_integer::big_integer() noexcept : data(), isNegative(false) {}

big_integer::big_integer(const big_integer& other) noexcept = default;

big_integer::big_integer(unsigned long long a) : data(a > MAX_UNIT_VAL ? 2 : 1), isNegative(false) {
  data[0] = static_cast<uint32_t>(a);
  a >>= 32;
  if (a > 0) {
    data[1] = a;
  }
}

big_integer::big_integer(long long a)
    : big_integer(static_cast<unsigned long long>(a >= 0 ? a : static_cast<unsigned long long>(-(a + 1)) + 1)) {
  isNegative = a < 0;
}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(int a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned short a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(short a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(const std::string& str) : big_integer(0) {
  isNegative = false;
  size_t index = str[0] == '-' ? 1 : 0;
  while (index < str.size() && str[index] >= '0' && str[index] <= '9') {
    size_t dif_index = 0;
    uint32_t addNum = std::stoi(str.substr(index, SHIFT_MAX_SIZE), &dif_index);
    index += dif_index;
    if (dif_index == 0) {
      break;
    }
    mulChange(dif_index == SHIFT_MAX_SIZE ? SHIFT_MAX : std::pow(10, dif_index));
    add(addNum);
  }
  if (index != str.size() || str.empty() || str[index - 1] < '0' || str[index - 1] > '9') {
    throw std::invalid_argument("incorrect format: " + str);
  }
  isNegative = str[0] == '-';
  checkZero();
}

big_integer::~big_integer() = default;

big_integer::big_integer(size_t size, uint32_t initValue) : data(size, initValue), isNegative(false) {}

big_integer& big_integer::operator=(const big_integer& other) {
  if (&other == this) {
    return *this;
  }
  big_integer newNumber(other);
  swap(newNumber);
  return *this;
}

void big_integer::changeSize(size_t newSize) {
  data.resize(newSize);
}

size_t big_integer::dataSize() const {
  return data.size();
}

void big_integer::equalizeSize(const big_integer& rhs) {
  if (dataSize() <= rhs.dataSize()) {
    changeSize(rhs.dataSize());
  }
}

uint32_t& big_integer::getUnit(size_t pos) {
  return data[pos];
}

uint32_t big_integer::getUnit(size_t pos) const {
  return data[pos];
}

uint32_t& big_integer::firstData() {
  return data[dataSize() - 1];
}

uint32_t big_integer::firstData() const {
  return data[dataSize() - 1];
}

uint32_t& big_integer::lastData() {
  return data[0];
}

uint32_t big_integer::lastData() const {
  return data[0];
}

void big_integer::swap(big_integer& swapper) noexcept {
  std::swap(isNegative, swapper.isNegative);
  std::swap(data, swapper.data);
}

void big_integer::checkZero() noexcept {
  size_t firstNotZero;
  for (firstNotZero = dataSize(); firstNotZero != 0 && getUnit(firstNotZero - 1) == 0; --firstNotZero) {}
  if (firstNotZero == 0) {
    firstNotZero = 1;
    isNegative = false;
  }
  changeSize(firstNotZero);
}

void big_integer::add(const int32_t shift) {
  int64_t carry = shift * (isNegative ? -1 : 1);
  if (firstData() == std::numeric_limits<uint32_t>::max()) {
    changeSize(dataSize() + 1);
  }
  for (size_t index = 0; carry != 0 && index != dataSize(); index++) {
    carry += getUnit(index);
    getUnit(index) = carry;
    carry >>= 32;
  }
  if (carry != 0) {
    firstData() = carry;
  }
  checkZero();
}

void big_integer::addWithShift(uint64_t delta, const size_t shift) {
  uint64_t carry = delta >> 32;
  delta %= SHIFT;
  delta += getUnit(shift);
  getUnit(shift) = delta;
  carry += delta >> 32;
  for (size_t index = shift + 1; carry != 0 && index != dataSize(); index++) {
    carry += getUnit(index);
    getUnit(index) = carry;
    carry >>= 32;
  }
  if (carry != 0) {
    firstData() = carry;
  }
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  equalizeSize(rhs);
  uint64_t carry = 0;
  isNegative = !isNegative;
  bool changeSign = ((!isNegative) && (rhs > *this)) || (isNegative && (rhs < *this));
  isNegative = !isNegative;
  if ((dataSize() == rhs.dataSize() && firstData() > MAX_UNIT_VAL - rhs.firstData()) || firstData() == MAX_UNIT_VAL) {
    changeSize(dataSize() + 1);
  }
  for (size_t index = 0; index != rhs.dataSize(); index++) {
    if (isNegative == rhs.isNegative) {
      carry = getUnit(index) + carry + rhs.getUnit(index);
    } else if (changeSign) {
      carry = rhs.getUnit(index) - carry - getUnit(index);
    } else {
      carry = getUnit(index) - carry - rhs.getUnit(index);
    }
    getUnit(index) = carry;
    carry = carry > MAX_UNIT_VAL ? 1 : 0;
  }
  for (size_t index = rhs.dataSize(); carry != 0 && index != dataSize(); index++) {
    if (isNegative == rhs.isNegative) {
      carry = getUnit(index) + carry;
    } else if (changeSign) {
      carry = -getUnit(index) - carry;
    } else {
      carry = getUnit(index) - carry;
    }
    getUnit(index) = carry;
    carry = carry > MAX_UNIT_VAL ? 1 : 0;
  }
  if (carry == 1) {
    firstData() = 1;
  }
  isNegative = isNegative ^ changeSign;
  checkZero();
  return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  isNegative = !isNegative;
  *this += rhs;
  isNegative = !isNegative;
  checkZero();
  return *this;
}

big_integer& big_integer::mulChange(const uint32_t scalar) {
  uint64_t carry = 0, res, scalar64 = scalar;
  for (size_t index = 0; index != dataSize(); ++index) {
    res = carry + getUnit(index) * scalar64;
    getUnit(index) = res;
    carry = res >> 32;
  }
  if (carry != 0) {
    changeSize(dataSize() + 1);
    getUnit(dataSize() - 1) = carry;
  }
  checkZero();
  return *this;
}

big_integer mul(const big_integer& lhs, const uint32_t scalar) {
  big_integer newNumber(lhs);
  return newNumber.mulChange(scalar);
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  size_t index1 = dataSize();
  changeSize(dataSize() + rhs.dataSize() -
             (static_cast<uint64_t>(firstData()) * rhs.firstData() > MAX_UNIT_VAL ? 0 : 1));
  for (; index1 != 0; --index1) {
    uint64_t val = getUnit(index1 - 1);
    getUnit(index1 - 1) = 0;
    for (size_t index2 = 0; index2 != rhs.dataSize(); ++index2) {
      addWithShift(val * rhs.getUnit(index2), index1 + index2 - 1);
    }
  }
  isNegative = isNegative ^ rhs.isNegative;
  checkZero();
  return *this;
}

uint32_t big_integer::scalarDivMod(uint32_t scalar) {
  uint64_t carry = 0;
  for (size_t index = dataSize(); index != 0; --index) {
    carry <<= 32;
    carry += getUnit(index - 1);
    getUnit(index - 1) = carry / scalar;
    carry %= scalar;
  }
  checkZero();
  return carry;
}

big_integer big_integer::operatorDivMod(const big_integer& rhs, bool returnQuot) {
  assert(rhs != 0);
  bool isOldNeg = isNegative;
  isNegative = rhs.isNegative;
  if ((rhs.isNegative && *this > rhs) || (!rhs.isNegative && *this < rhs)) {
    big_integer newNumber(0);
    isNegative = isOldNeg;
    if (returnQuot) {
      swap(newNumber);
    }
    checkZero();
    return newNumber;
  }
  isNegative = isOldNeg;
  uint64_t elementaryQuot, mainDivider;
  bool finalSign = isNegative ^ rhs.isNegative;
  isNegative = rhs.isNegative;
  size_t n, m;
  for (size_t index = rhs.dataSize() - 1;; --index) {
    if (rhs.getUnit(index) != 0) {
      mainDivider = rhs.getUnit(index);
      n = index + 1;
      m = dataSize() - n;
      break;
    }
  }
  big_integer quot(m + 1, 0);
  for (size_t index = m; dataSize() >= n; --index) {
    if (n + index >= dataSize()) {
      elementaryQuot = getUnit(n + index - 1) / mainDivider;
    } else {
      elementaryQuot = (getUnit(n + index) * SHIFT + getUnit(n + index - 1)) / mainDivider;
    }
    if (elementaryQuot == 0) {
      if (index == 0) {
        break;
      }
      continue;
    }
    uint32_t quotAtom = 0;
    while (true) {
      big_integer shiftBigInt(mul(rhs, elementaryQuot) << (index * 32));
      *this -= shiftBigInt;
      quotAtom += elementaryQuot;
      if (isNegative == rhs.isNegative || *this == 0) {
        break;
      }
      elementaryQuot = (elementaryQuot + 1) / 2;
      shiftBigInt = mul(rhs, elementaryQuot) << (index * 32);
      *this += shiftBigInt;
      quotAtom -= elementaryQuot;
      if (n + index - 1 >= dataSize() ||
          ((n + index >= dataSize() || getUnit(n + index) == 0) && getUnit(n + index - 1) < rhs.getUnit(n - 1))) {
        break;
      }
      if (n + index >= dataSize()) {
        elementaryQuot = getUnit(n + index - 1) / mainDivider;
      } else {
        elementaryQuot = (getUnit(n + index) * SHIFT + getUnit(n + index - 1)) / mainDivider;
      }
    }
    quot.getUnit(index) = quotAtom;
    if (index == 0) {
      break;
    }
  }
  quot.isNegative = finalSign;
  isNegative = isOldNeg;
  if (returnQuot) {
    swap(quot);
  }
  checkZero();
  quot.checkZero();
  return quot;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  operatorDivMod(rhs, true);
  return *this;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  operatorDivMod(rhs, false);
  return *this;
}

big_integer& big_integer::logicOperator(const big_integer& rhs, uint32_t (*f)(uint32_t, uint32_t)) {
  equalizeSize(rhs);
  bool finalSign = f(isNegative, rhs.isNegative), carryA = isNegative, carryB = rhs.isNegative;
  for (size_t index = 0; index != dataSize(); ++index) {
    uint32_t a = isNegative ? ~getUnit(index) : getUnit(index),
             b = rhs.isNegative ? (index < rhs.dataSize() ? ~rhs.getUnit(index) : MAX_UNIT_VAL)
                                : (index < rhs.dataSize() ? rhs.getUnit(index) : 0);
    if (carryA) {
      carryA = a == MAX_UNIT_VAL;
      a++;
    }
    if (carryB) {
      carryB = b == MAX_UNIT_VAL;
      b++;
    }
    a = f(a, b);
    if (finalSign) {
      getUnit(index) = ~a;
    } else {
      getUnit(index) = a;
    }
  }
  isNegative = false;
  if (finalSign) {
    (*this)++;
  }
  isNegative = finalSign;
  checkZero();
  return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  return logicOperator(rhs, [](uint32_t a, uint32_t b) { return a & b; });
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  return logicOperator(rhs, [](uint32_t a, uint32_t b) { return a | b; });
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  return logicOperator(rhs, [](uint32_t a, uint32_t b) { return a ^ b; });
}

big_integer& big_integer::operator<<=(int rhs) {
  assert(rhs >= 0);
  if (rhs == 0) {
    return *this;
  }
  const uint16_t SHIFT_LEFT = rhs % 32, SHIFT_RIGHT = 32 - rhs % 32, RIGHT_INDEX = (rhs + 31) / 32,
                 LEFT_INDEX = rhs / 32;
  changeSize(dataSize() + 1 + RIGHT_INDEX);
  size_t index;
  bool needClear = true;
  for (index = dataSize() - 1; index >= LEFT_INDEX; --index) {
    getUnit(index) = (getUnit(index - LEFT_INDEX) << SHIFT_LEFT);
    if (index >= RIGHT_INDEX && SHIFT_LEFT != 0) {
      getUnit(index) += (getUnit(index - RIGHT_INDEX) >> SHIFT_RIGHT);
    }
    if (index <= LEFT_INDEX) {
      needClear = (index != 0);
      index--;
      break;
    }
  }
  for (size_t clearIndex = 0; needClear && clearIndex <= index; ++clearIndex) {
    getUnit(clearIndex) = 0;
  }
  checkZero();
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  assert(rhs >= 0);
  if (rhs == 0) {
    return *this;
  }
  const uint16_t RIGHT_INDEX = (rhs + 31) / 32, LEFT_INDEX = rhs / 32, SHIFT = rhs % 32;
  const uint64_t SHIFT_LEFT = (static_cast<uint64_t>(UINT32_MAX) + 1) >> SHIFT;
  size_t index;
  bool needRound = false;
  for (index = 0; index < dataSize() && index < RIGHT_INDEX; ++index) {
    needRound |= (getUnit(index) != 0);
  }
  if (RIGHT_INDEX < dataSize()) {
    needRound |= ((getUnit(RIGHT_INDEX) * SHIFT_LEFT) != 0);
  }
  for (index = 0; index + LEFT_INDEX < dataSize(); ++index) {
    getUnit(index) = (getUnit(index + LEFT_INDEX) >> SHIFT);
    if (index + RIGHT_INDEX < dataSize()) {
      getUnit(index) += (getUnit(index + RIGHT_INDEX) * SHIFT_LEFT);
    }
  }
  for (size_t clearIndex = index; clearIndex < dataSize(); ++clearIndex) {
    getUnit(clearIndex) = 0;
  }
  checkZero();
  if (needRound & isNegative) {
    (*this)--;
  }
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  big_integer newNumber(*this);
  newNumber.isNegative = !newNumber.isNegative;
  newNumber.checkZero();
  return newNumber;
}

big_integer big_integer::operator~() const {
  big_integer newNumber(*this);
  newNumber.add(1);
  newNumber.isNegative = !newNumber.isNegative;
  return newNumber;
}

big_integer& big_integer::operator++() {
  if (dataSize() == 0) {
    changeSize(1);
    getUnit(0) = 1;
    return *this;
  }
  add(1);
  return *this;
}

big_integer big_integer::operator++(int) {
  big_integer newNumber(*this);
  ++*this;
  return newNumber;
}

big_integer& big_integer::operator--() {
  if (dataSize() == 0) {
    changeSize(1);
    getUnit(0) = 1;
    isNegative = true;
    return *this;
  }
  add(-1);
  return *this;
}

big_integer big_integer::operator--(int) {
  big_integer newNumber(*this);
  --*this;
  return newNumber;
}

big_integer operator+(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber += b;
}

big_integer operator-(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber -= b;
}

big_integer operator*(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber *= b;
}

big_integer operator/(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber /= b;
}

big_integer operator%(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber %= b;
}

big_integer operator&(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber &= b;
}

big_integer operator|(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber |= b;
}

big_integer operator^(const big_integer& a, const big_integer& b) {
  big_integer newNumber(a);
  return newNumber ^= b;
}

big_integer operator<<(const big_integer& a, int b) {
  big_integer newNumber(a);
  return newNumber <<= b;
}

big_integer operator>>(const big_integer& a, int b) {
  big_integer newNumber(a);
  return newNumber >>= b;
}

bool big_integer::operator==(const big_integer& b) const {
  return (dataSize() == 0 && b.dataSize() == 1 && b.lastData() == 0) ||
         (b.dataSize() == 0 && dataSize() == 1 && lastData() == 0) ||
         (isNegative == b.isNegative && data == b.data);
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  if (a == b) {
    return false;
  }
  if (a.isNegative ^ b.isNegative) {
    return a.isNegative;
  }
  return a.isNegative ^
         (a.dataSize() < b.dataSize() ||
          (a.dataSize() == b.dataSize() && std::lexicographical_compare(std::rbegin(a.data), std::rend(a.data),
                                                                        std::rbegin(b.data), std::rend(b.data))));
}

bool operator>(const big_integer& a, const big_integer& b) {
  return b < a;
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(a < b);
}

std::string to_string(const big_integer& a) {
  big_integer forStringInt(a);
  std::string s;
  while (true) {
    std::string add = std::to_string(forStringInt.scalarDivMod(a.SHIFT_MAX));
    std::reverse(add.begin(), add.end());
    s.append(add);
    if (forStringInt == 0) {
      break;
    } else {
      s.append(a.SHIFT_MAX_SIZE - add.size(), '0');
    }
  }
  if (a.isNegative) {
    s.push_back('-');
  }
  std::reverse(s.begin(), s.end());
  return s;
}

std::ostream& operator<<(std::ostream& s, const big_integer& a) {
  return s << to_string(a);
}
