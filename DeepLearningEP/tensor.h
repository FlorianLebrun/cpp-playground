#pragma once
#include <string>
#include <vector>
#include <math.h>

inline void checkAssert(bool condition) { if (!condition) throw; }

typedef double Scalar;

struct Size {
   size_t size[4];
   size_t count;
   size_t dim;
   Size(size_t dim, size_t s1 = 1, size_t s2 = 1, size_t s3 = 1, size_t s4 = 1) {
      checkAssert(dim < 5);
      this->dim = dim;
      this->count = s1 * s2 * s3 * s4;
      this->size[0] = s1;
      this->size[1] = s2;
      this->size[2] = s3;
      this->size[3] = s4;
   }
};

struct Tensor {
   std::vector<Scalar> values;
   Size size;
   Tensor(Size sz) : size(sz) {
      this->values.resize(size.count);
   }
   void fill(uint8_t* v, int n) {
      checkAssert(values.size() == n);
      for (int i = 0; i < n; i++) {
         values[i] = (Scalar(v[i]) / 256.0);
      }
   }
   void zeros() {

   }
};

struct Tensor1D : Tensor {
   Tensor1D(int sx) : Tensor(Size(1, sx)) {
   }
};

struct Tensor2D : Tensor {
   Tensor2D(int sx, int sy) : Tensor(Size(2, sx, sy)) {
   }
};

struct Tensor3D : Tensor {
   Tensor3D(int sx, int sy, int sz) : Tensor(Size(3, sx, sy, sz)) {
   }
};

void saveBitmap(std::string path, Tensor* data);
Scalar randomScalar(Scalar min, Scalar max);
