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
  XY(T cx, T cy) : x(cx), y(cy) {} 
  XY(const XY &other) : x(other.x), y(other.y) {} 

  XY& operator +=(const XY &rhs) { x += rhs.x; y += rhs.y; return *this; }
  friend XY& operator + (XY &lhs, const XY &rhs) { return lhs += rhs; }

  XY& operator -=(const XY &rhs) { x -= rhs.x; y -= rhs.y; return *this; }
  friend XY& operator - (XY &lhs, const XY &rhs) { return lhs -= rhs; }

  XY& operator *=(const XY &rhs) { x *= rhs.x; y *= rhs.y; return *this; }
  friend XY& operator * (XY &lhs, const XY &rhs) { return lhs *= rhs; }

  XY& operator /=(const XY &rhs) { x /= rhs.x; y /= rhs.y; return *this; }
  friend XY& operator / (XY &lhs, const XY &rhs) { return lhs /= rhs; }

  friend bool operator ==(const XY &lhs, const XY &rhs) { return (lhs.x == rhs.x) && (lhs.y == rhs.y); }
  friend bool operator !=(const XY &lhs, const XY &rhs) { return !(lhs == rhs); }

  T x;
  T y;

private:
  XY();
};

typedef XY<int32_t> iXY;
typedef XY<double> fXY;


template <class T> class Rect {
public:
  Rect(T o, T l) : org(o), len(l) {} 
  Rect(const Rect &other) : org(other.org), len(other.len) {} 

  friend bool operator ==(const Rect &lhs, const Rect &rhs) { return (lhs.org == rhs.org) && (lhs.len == rhs.len); }
  friend bool operator !=(const Rect &lhs, const Rect &rhs) { return !(lhs == rhs); }

  T org;
  T len;

private:
  Rect();
};

typedef Rect<iXY> iRect;
typedef Rect<fXY> fRect;


template <class T> class Col {
public:
  Col(T cy, T cu, T cv) : y(cy), u(cu), v(cv) {} 
  Col(const Col &other) : y(other.y), u(other.u), v(other.v) {} 

  friend bool operator ==(const Col &lhs, const Col &rhs) { return (lhs.y == rhs.y) && (lhs.u == rhs.u) && (lhs.v == rhs.v); }
  friend bool operator !=(const Col &lhs, const Col &rhs) { return !(lhs == rhs); }

  T y;
  T u;
  T v;

private:
  Col();
};

typedef Col<int16_t> iCol;
typedef Col<double> fCol;

} // namespace streampunk

#endif
