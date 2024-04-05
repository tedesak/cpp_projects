#pragma once

#include <cstdint>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

struct big_integer {
private:
  const uint64_t SHIFT = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1;
  const uint32_t MAX_UNIT_VAL = std::numeric_limits<uint32_t>::max();
  const uint32_t SHIFT_MAX_SIZE = 9;
  const uint32_t SHIFT_MAX = 1000000000;
  std::vector<uint32_t> data;
  bool isNegative;

public:
  big_integer() noexcept;
  big_integer(const big_integer& other) noexcept;
  big_integer(int a);
  big_integer(long long a);
  big_integer(long a);
  big_integer(short a);
  big_integer(unsigned long long a);
  big_integer(unsigned long a);
  big_integer(unsigned int a);
  big_integer(unsigned short a);
  explicit big_integer(const std::string& str);
  ~big_integer();
  big_integer& operator=(const big_integer& other);

private:
  void checkZero() noexcept;
  big_integer(size_t size, uint32_t initValue);
  void equalizeSize(const big_integer& rhs);
  void changeSize(size_t newSize);
  size_t dataSize() const;
  uint32_t& getUnit(size_t pos);
  uint32_t getUnit(size_t pos) const;
  void add(const int32_t shift);
  void addWithShift(uint64_t delta, const size_t shift);
  void swap(big_integer& swapper) noexcept;
  uint32_t firstData() const;
  uint32_t& firstData();
  uint32_t lastData() const;
  uint32_t& lastData();
  friend big_integer mul(const big_integer& lhs, const uint32_t scalar);
  big_integer& mulChange(const uint32_t scalar);
  uint32_t scalarDivMod(uint32_t scalar);
  big_integer& logicOperator(const big_integer& rhs, uint32_t (*f)(uint32_t, uint32_t));
  big_integer operatorDivMod(const big_integer& rhs, bool returnQuot);

public:
  big_integer& operator+=(const big_integer& rhs);
  big_integer& operator-=(const big_integer& rhs);
  big_integer& operator*=(const big_integer& rhs);
  big_integer& operator/=(const big_integer& rhs);
  big_integer& operator%=(const big_integer& rhs);

  big_integer& operator&=(const big_integer& rhs);
  big_integer& operator|=(const big_integer& rhs);
  big_integer& operator^=(const big_integer& rhs);
  big_integer& operator<<=(int rhs);

  big_integer& operator>>=(int rhs);
  big_integer operator+() const;

  big_integer operator-() const;
  big_integer operator~() const;
  big_integer& operator++();

  big_integer operator++(int);
  big_integer& operator--();

  big_integer operator--(int);
  bool operator==(const big_integer& b) const;

  friend bool operator!=(const big_integer& a, const big_integer& b);
  friend bool operator<(const big_integer& a, const big_integer& b);
  friend bool operator>(const big_integer& a, const big_integer& b);
  friend bool operator<=(const big_integer& a, const big_integer& b);
  friend bool operator>=(const big_integer& a, const big_integer& b);
  friend std::string to_string(const big_integer& a);
};

big_integer operator+(const big_integer& a, const big_integer& b);
big_integer operator-(const big_integer& a, const big_integer& b);
big_integer operator*(const big_integer& a, const big_integer& b);
big_integer operator/(const big_integer& a, const big_integer& b);
big_integer operator%(const big_integer& a, const big_integer& b);

big_integer operator&(const big_integer& a, const big_integer& b);
big_integer operator|(const big_integer& a, const big_integer& b);
big_integer operator^(const big_integer& a, const big_integer& b);

big_integer operator<<(const big_integer& a, int b);
big_integer operator>>(const big_integer& a, int b);

//bool operator==(const big_integer& a, const big_integer& b);
bool operator!=(const big_integer& a, const big_integer& b);
bool operator<(const big_integer& a, const big_integer& b);
bool operator>(const big_integer& a, const big_integer& b);
bool operator<=(const big_integer& a, const big_integer& b);
bool operator>=(const big_integer& a, const big_integer& b);

std::string to_string(const big_integer& a);
std::ostream& operator<<(std::ostream& s, const big_integer& a);
