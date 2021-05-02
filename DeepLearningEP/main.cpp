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
      Neuron& input;
      Scalar weight;
      Link(Neuron& input, Scalar weight) : input(input), weight(weight) {}
   };

   // Connection
   std::vector<Link> links;
   Scalar bias = randomScalar(-1.0, 1.0);

   // State
   bool activate = false;
   Scalar value = 0.0;
   Scalar error = 0.0;

   Scalar weightPower() {
      Scalar acc = 0.0;
      for (auto& link : this->links) {
         if (link.input.activate) {
            acc += link.weight * link.weight;
         }
      }
      return acc;
   }
   Scalar errorPower() {
      Scalar acc = 0.0;
      for (auto& link : this->links) {
         if (link.input.activate) {
            acc += link.input.error * link.input.error;
         }
      }
      return acc;
   }
   Scalar forwardError() {
      Scalar acc = 0.0;
      for (auto& link : this->links) {
         if (link.input.activate) {
            acc += link.input.error * link.weight;
         }
      }
      return acc;
   }

   void impulse(Scalar impulseFactor) {
      Scalar forwardError = this->forwardError();
      Scalar forwardPower = this->weightPower();
      if (forwardPower == 0.0) {
         return;
      }
      Scalar impulse = impulseFactor * (this->error - forwardError) / forwardPower;
      for (auto& link : this->links) {
         if (link.input.activate) {
            link.input.error += impulse * link.weight;
         }
      }
   }

   void learn(Scalar impulseFactor) {
      Scalar forwardError = this->forwardError();
      Scalar forwardPower = this->errorPower();
      if (forwardPower == 0.0) {
         return;
      }
      Scalar impulse = impulseFactor * (this->error - forwardError) / forwardPower;
      for (auto& link : this->links) {
         if (link.input.activate) {
            link.weight -= impulse * link.input.error;
         }
      }
   }

   void feed(Scalar value) {
      this->activate = true;
      this->value = value;
   }

   void feedback(Scalar expectedValue) {
      this->error = this->value - expectedValue;
   }

   void compute() {

      // Clean error
      this->error = 0.0;

      // Vectorial projection
      Scalar acc = 0;
      for (auto& link : this->links) {
         acc += link.weight * link.input.value;
      }
      this->value = acc + this->bias;

      // Relu activation
      this->activate = (this->value > 0.0);
      if (!this->activate) this->value = 0.0;

   }
   void connect(Neuron& neuron, Scalar weight) {
      this->links.push_back(Link(neuron, weight));
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
         neurons[i]->feed(data->values[i]);
      }
   }
   void feedback(Tensor* data) {
      for (int i = 0; i < data->size.count; i++) {
         neurons[i]->feedback(data->values[i]);
      }
   }
   Scalar error() {
      Scalar acc = 0.0;
      for (auto v : this->neurons) {
         acc += v->error * v->error;
      }
      return acc;
   }
   void backpropagate() {
      //this->printFixedError();
      for (auto v : this->neurons) v->impulse(1);
      for (auto v : this->neurons) v->impulse(0.5);
      for (auto v : this->neurons) v->impulse(0.25);
      for (auto v : this->neurons) v->impulse(0.125);
      for (auto v : this->neurons) v->impulse(0.0625);
      //this->printFixedError();
   }
   void learn(Scalar learningRate) {
      for (auto v : this->neurons) {
         v->learn(learningRate);
      }
   }
   void compute() {
      for (auto v : this->neurons) {
         v->compute();
      }
   }
   void printErrors() {
      printf("\n--------------------------\n");
      for (int i = 0; i < this->size.count; i++) {
         auto& neuron = *this->neurons[i];
         if (neuron.activate) {
            printf("%d: %.3lg \t-> %.3lg\n", i, neuron.error, neuron.forwardError());
         }
      }
      printf("> error: %lg\n", this->error());
   }
   void printResults() {
      printf("\n--------------------------\n");
      for (int i = 0; i < this->size.count; i++) {
         auto& neuron = *this->neurons[i];
         Scalar expectedValue = neuron.value + neuron.error;
         printf("%d: (%.3lg)\t%.3lg\n", i, expectedValue, neuron.value);
      }
      printf("> error: %lg\n", this->error());
   }
};

struct NeuralNetwork {
   std::vector<Layer*> layers;
   void addLayer(Size size) {
      Layer* layer = new Layer(size);
      if (this->layers.size()) {
         Layer* input = this->layers.back();
         connectFull(*input, *layer);
      }
      this->layers.push_back(layer);
   }
   void feed(Tensor2D* inData) {
      this->layers[0]->feed(inData);
      for (int i = 1; i < layers.size(); i++) {
         this->layers[i]->compute();
      }
   }
   void feedback(Tensor1D* outData) {
      this->layers.back()->feedback(outData);
      for (int i = layers.size() - 1; i > 0; i--) {
         this->layers[i]->backpropagate();
      }
   }
   void learn(Scalar learningRate) {
      for (Layer* layer : this->layers) {
         layer->learn(learningRate);
      }
   }
   void printErrors() {
      this->layers.back()->printErrors();
   }
   void printResults() {
      this->layers.back()->printResults();
   }
private:
   void connectFull(Layer& in, Layer& out) {
      Scalar dispersion = 1.0 / in.neurons.size();
      for (auto vout : out.neurons) {
         for (auto vin : in.neurons) {
            vout->connect(*vin, dispersion * randomScalar(0.0, 1.0));
         }
      }
   }
};

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
         if (index >= this->datacount) return 0;
         uint8_t ordinal = this->datas[index * this->datasize];
         auto data = new Tensor1D(range);
         data->values[ordinal] = 1.0;
         return data;
      }
      Tensor1D* getTensor1D(int index) {
         if (dim != 1) return 0;
         if (index >= this->datacount) return 0;
         auto data = new Tensor1D(sizes[0]);
         data->fill(&this->datas[index * this->datasize], this->datasize);
         return data;
      }
      Tensor2D* getTensor2D(int index) {
         if (dim != 2) return 0;
         if (index >= this->datacount) return 0;
         auto data = new Tensor2D(sizes[0], sizes[1]);
         data->fill(&this->datas[index * this->datasize], this->datasize);
         return data;
      }
   };
}

void main() {
   MNIST::UByteTensorFile images("D:/data/MNIST/train-images.idx3-ubyte");
   MNIST::UByteTensorFile labels("D:/data/MNIST/train-labels.idx1-ubyte");

   NeuralNetwork network;

   network.addLayer(Size(2, 28, 28));
   network.addLayer(Size(2, 28, 28));
   network.addLayer(Size(1, 10));

   if (0) {
      Scalar learningRate = 0.01;
      int count = images.datacount;
      for (int i = 1; i < count; i++) {
         for (int k = 0; k < 100 && i < count; k++, i++) {
            Tensor2D* inData = images.getTensor2D(i);
            Tensor1D* outData = labels.getOrdinalTensor1D(i, 10);
            network.feed(inData);
            network.feedback(outData);
            network.learn(learningRate);
         }
         Tensor2D* inData = images.getTensor2D(0);
         Tensor1D* outData = labels.getOrdinalTensor1D(0, 10);
         network.feed(inData);
         network.feedback(outData);
         network.printResults();
      }
   }
   else {
      Scalar learningRate = 0.01;
      Tensor2D* inData = images.getTensor2D(0);
      Tensor1D* outData = labels.getOrdinalTensor1D(0, 10);
      for (int k = 0; k < 1000; k++) {
         network.feed(inData);
         network.feedback(outData);
         network.learn(learningRate);
         //network.printResults();
         printf("> error: %lg\n", network.layers.back()->error());
      }
   }
}

