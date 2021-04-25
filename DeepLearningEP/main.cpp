#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <math.h>

void check(bool condition) { if (!condition) throw; }

struct uint32_HE_t {
   uint32_t value;
   operator uint32_t() {
      return _byteswap_ulong(value);
   }
};

typedef double Scalar;

struct Size {
   size_t size[4];
   size_t count;
   size_t dim;
   Size(size_t dim, size_t s1 = 1, size_t s2 = 1, size_t s3 = 1, size_t s4 = 1) {
      check(dim < 5);
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
      check(values.size() == n);
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
   std::vector<Scalar> values;
};


Scalar randomScalar(Scalar min, Scalar max) {
   return min + (Scalar(rand()) / (Scalar(RAND_MAX) / (max - min)));
}

struct Neuron {
   struct Link {
      Neuron& from;
      Scalar weight = randomScalar(-1.0, 1.0);
      Link(Neuron& from) : from(from) {}
   };
   Scalar value = 0.0;
   Scalar error = 0.0;
   Scalar bias = randomScalar(-1.0, 1.0);
   std::vector<Link> links;

   void compute() {
      Scalar acc = 0;
      for (auto& link : this->links) {
         acc = link.weight * link.from.value;
      }
      this->value = acc + this->bias;
   }
   void connect(Neuron& neuron) {
      this->links.push_back(Link(neuron));
   }
};

struct Layer {
   std::vector<Neuron*> neurons;
   Size size;
   Layer(Size size) : size(size) {
      for (int i = 0; i < size.count; i++) {
         neurons.push_back(new Neuron());
      }
   }
   void feed(Tensor* data) {
      check(data->values.size() == neurons.size());
      for (int i = 0; i < data->values.size(); i++) {
         neurons[i]->value = data->values[i];
      }
   }
   Scalar feedback(Tensor* data) {
      Scalar error = 0;
      for (int i = 0; i < data->size.count; i++) {
         Scalar e = neurons[i]->error = data->values[i] - neurons[i]->value;
         error += e * e;
      }
      return error / Scalar(size.count);
   }
   void compute() {
      for (auto v : neurons) {
         v->compute();
      }
   }
   void learn() {

   }
};

typedef Layer Layer1D;
typedef Layer Layer2D;

void connectFull(Layer& in, Layer& out) {
   for (auto vout : out.neurons) {
      for (auto vin : in.neurons) {
         vout->connect(*vin);
      }
   }
}

void connectFullFrom2DTo2D(Layer2D& in, Layer2D& out) {
   connectFull(in, out);
}

void connectFullFrom2DTo1D(Layer2D& in, Layer1D& out) {
   connectFull(in, out);
}

namespace MNIST {

   struct UByteTensorFile {

      struct tHeader {
         uint8_t magic[3];
         uint8_t dim;
         uint32_HE_t count; // count defined by dim
         uint32_HE_t sizes[4]; // count defined by dim
      };

      uint8_t dim = 0;
      uint32_t sizes[4] = {};
      uint32_t datacount = 0;
      uint32_t datasize = 0;
      uint8_t* datas = 0;

      UByteTensorFile(std::string path) {
         std::ifstream f(path, std::ios_base::in | std::ios_base::binary);
         check(f.is_open());

         tHeader header;
         f.read((char*)&header, sizeof(tHeader));
         check(header.dim <= 4);
         this->dim = header.dim - 1;
         this->datacount = header.count;
         this->datasize = 1;
         for (int i = 0; i < this->dim; i++) {
            this->sizes[i] = header.sizes[i];
            this->datasize *= this->sizes[i];
         }

         int bufsz = this->datacount * this->datasize;
         f.seekg(sizeof(uint32_t) * (2 + this->dim), std::ios_base::beg);
         this->datas = (uint8_t*)malloc(bufsz);
         f.read((char*)this->datas, bufsz);
      }
      Tensor1D* getOrdinalTensor1D(int index, int range) {
         if (dim != 0) return 0;
         if (index >= datacount) return 0;
         auto data = new Tensor1D(range);
         data->values[index] = 1.0;
         return data;
      }
      Tensor1D* getTensor1D(int index) {
         if (dim != 1) return 0;
         if (index >= sizes[0]) return 0;
         auto data = new Tensor1D(sizes[0]);
         data->fill(&this->datas[index * this->datasize], this->datasize);
         return data;
      }
      Tensor2D* getTensor2D(int index) {
         if (dim != 2) return 0;
         if (index >= sizes[0]) return 0;
         auto data = new Tensor2D(sizes[0], sizes[1]);
         data->fill(&this->datas[index * this->datasize], this->datasize);
         return data;
      }
   };
}

void main() {
   MNIST::UByteTensorFile images("D:/data/MNIST/train-images.idx3-ubyte");
   MNIST::UByteTensorFile labels("D:/data/MNIST/train-labels.idx1-ubyte");


   auto layer0 = *new Layer2D(Size(2, 28, 28));

   auto layer1 = *new Layer2D(Size(2, 28, 28));
   connectFullFrom2DTo2D(layer0, layer1);

   auto layer2 = *new Layer1D(Size(1, 10));
   connectFullFrom2DTo1D(layer1, layer2);

   Tensor2D* inData = images.getTensor2D(0);
   Tensor1D* outData = labels.getOrdinalTensor1D(0, 10);
   layer0.feed(inData);
   layer1.compute();
   layer2.compute();
   Scalar err = layer2.feedback(outData);
   layer2.learn();
   layer1.learn();
}

