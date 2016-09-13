/* Copyright 2016 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

namespace streampunk {

template <class T> class XY {
public:
  XY(T x, T y) : X(x), Y(y) {} 
  XY(const XY &other) : X(other.X), Y(other.Y) {} 

  XY& operator +=(const XY &rhs) { X += rhs.X; Y += rhs.Y; return *this; }
  friend XY& operator + (XY &lhs, const XY &rhs) { return lhs += rhs; }

  XY& operator -=(const XY &rhs) { X -= rhs.X; Y -= rhs.Y; return *this; }
  friend XY& operator - (XY &lhs, const XY &rhs) { return lhs -= rhs; }

  XY& operator *=(const XY &rhs) { X *= rhs.X; Y *= rhs.Y; return *this; }
  friend XY& operator * (XY &lhs, const XY &rhs) { return lhs *= rhs; }

  XY& operator /=(const XY &rhs) { X /= rhs.X; Y /= rhs.Y; return *this; }
  friend XY& operator / (XY &lhs, const XY &rhs) { return lhs /= rhs; }

  friend bool operator ==(const XY &lhs, const XY &rhs) { return (lhs.X == rhs.X) && (lhs.Y == rhs.Y); }
  friend bool operator !=(const XY &lhs, const XY &rhs) { return !(lhs == rhs); }

  T X;
  T Y;

private:
  XY();
};

typedef XY<int32_t> iXY;
typedef XY<double> fXY;

} // namespace streampunk

#endif
