#include "./tensor.h"
#include "./MNIST.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <math.h>

struct Neuron;
struct Link;

struct Value {
   union {
      struct {
         uint8_t disabled : 1;
         uint8_t saturated : 1;
         uint8_t activated : 1;
      };
      uint8_t flags = 1;
   };
   Scalar value = 0.0;

   __forceinline operator Scalar() {
      return this->value;
   }
   __forceinline Value& operator = (Scalar value) {
      if (value <= 0.0) {
         this->flags = 1;
         this->value = 0.0;
      }
      else if (value > 1.0) {
         this->flags = 6;
         this->value = 1.0;
      }
      else {
         this->flags = 4;
         this->value = value;
      }
      return *this;
   }
   void check() {
      if (this->disabled) checkAssert(this->value == 0.0);
      else if (this->saturated) checkAssert(this->value == 1.0);
      else checkAssert(this->value >= 0.0 && this->value <= 1.0);
   }
};

struct NeuronId {
   int32_t index;
   uint32_t layer;
   NeuronId(int32_t index, uint32_t layer)
      : index(index), layer(layer) {
   }
};

struct NeuronState : Value {
   NeuronId id;
   Value computed;
   Value expected;

   Scalar error = 0.0;
   Scalar innerError = 0.0;

   NeuronState(NeuronId id) : id(id) {
   }
   void setInnerValue(Scalar value) {
      this->expected = this->computed = value;
   }
   void check() {
      this->computed.check();
      this->expected.check();
   }
};

struct NeuronLink {
   NeuronState& input;
   Scalar weight;
   bool inhibitory : 1;
   bool excitatory : 1;
   NeuronLink(NeuronState& input, Scalar weight)
      : input(input), weight(weight) {
      if (weight > 0.0) {
         this->inhibitory = 0;
         this->excitatory = 1;
      }
      else {
         this->inhibitory = 1;
         this->excitatory = 0;
      }
   }
   void check() {
      checkAssert(this->excitatory ? this->weight >= 0.0 : this->weight <= 0.0);
   }
};

struct NeuronLinks : std::vector<NeuronLink> {
   Scalar value() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         acc += link.weight * link.input.computed;
      }
      return acc;
   }
   Scalar expectedValue() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         acc += link.weight * link.input.expected;
      }
      return acc;
   }
   Scalar weightPower() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (link.input.activated) {
            acc += link.weight * link.weight;
         }
      }
      return acc;
   }
   Scalar unsaturatedErrorPower() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (!link.input.expected.saturated) {
            acc += link.input.error * link.input.error;
         }
      }
      return acc;
   }
   Scalar unsaturatedExpectedValue() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (!link.input.expected.saturated) {
            acc += link.weight * link.input.expected;
         }
      }
      return acc;
   }
   Scalar unsaturatedWeightPower() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (!link.input.expected.saturated) {
            acc += link.weight * link.weight;
         }
      }
      return acc;
   }
   Scalar undisabledErrorPower() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (!link.input.expected.disabled) {
            acc += link.input.error * link.input.error;
         }
      }
      return acc;
   }
   Scalar undisabledExpectedValue() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (!link.input.expected.disabled) {
            acc += link.weight * link.input.expected;
         }
      }
      return acc;
   }
   Scalar undisabledWeightPower() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (!link.input.expected.disabled) {
            acc += link.weight * link.weight;
         }
      }
      return acc;
   }

};

struct Neuron : NeuronState {
   NeuronLinks links;
   NeuronState bias; // Permanent inhibiter

   Neuron(NeuronId id) : NeuronState(id), bias(NeuronId(-id.index, id.layer)) {
      this->links.push_back(NeuronLink(this->bias, 0.0));
   }

   void check() {
      this->NeuronState::check();
      this->bias.check();
      for (auto& link : this->links) {
         link.check();
      }
   }

   Scalar forwardError() {
      return this->expected - this->links.expectedValue();
   }

   void backpropagateImpulse(Scalar impulseFactor) {
      this->check();
      Scalar forwardExpected = this->links.expectedValue();
      if (this->expected > forwardExpected) {
         Scalar weightPower = this->links.unsaturatedWeightPower();
         Scalar impulse = impulseFactor * (this->expected - forwardExpected) / weightPower;
         for (auto& link : this->links) {
            if (!link.input.expected.saturated) {
               link.input.expected = link.input.expected + impulse * link.weight;
            }
            else {
               _ASSERT(link.input.expected == 1.0);
            }
         }
      }
      else {
         Scalar weightPower = this->links.undisabledWeightPower();
         Scalar impulse = impulseFactor * (this->expected - forwardExpected) / weightPower;
         for (auto& link : this->links) {
            if (!link.input.expected.disabled) {
               link.input.expected = link.input.expected + impulse * link.weight;
            }
            else {
               _ASSERT(link.input.expected == 0.0);
            }
         }
      }
   }

   void updateError() {
      this->error = this->computed - this->expected;
   }

   void learn(Scalar learningRate) {
      if (this->computed < this->expected) {
         Scalar errorPower = this->links.unsaturatedErrorPower();
         if (errorPower > 0.0) {
            Scalar impulse = learningRate * (this->expected - value) / errorPower;
            for (auto& link : this->links) {
               if (!link.input.expected.saturated) {
                  link.weight -= impulse * link.input.error;
               }
            }
         }
      }
      else {
         Scalar errorPower = this->links.undisabledErrorPower();
         if (errorPower > 0.0) {
            Scalar impulse = learningRate * (this->expected - value) / errorPower;
            for (auto& link : this->links) {
               if (!link.input.expected.disabled) {
                  link.weight -= impulse * link.input.error;
               }
            }
         }
      }
      Scalar newValue = this->links.expectedValue();
      if (abs(newValue - this->expected) > abs(value - this->expected)) {
         //printf("learn issue\n");
      }
   }

   void compute() {
      this->bias.setInnerValue(1.0);
      this->setInnerValue(this->links.value());
   }
   void connect(Neuron& neuron, Scalar weight) {
      this->links.push_back(NeuronLink(neuron, weight));
   }
};

struct Layer {
   uint32_t layerId = 0;
   std::vector<Neuron*> neurons;
   Size size;
   Layer(Size size) : size(size) {
      for (int i = 0; i < size.count; i++) {
         neurons.push_back(new Neuron(NeuronId(i, this->layerId)));
      }
   }
   void check() {
      for (auto v : this->neurons) {
         v->check();
      }
   }
   void feed(Tensor* data) {
      checkAssert(data->values.size() == neurons.size());
      for (int i = 0; i < data->values.size(); i++) {
         neurons[i]->setInnerValue(data->values[i]);
      }
   }
   void feedback(Tensor* data) {
      for (int i = 0; i < data->size.count; i++) {
         neurons[i]->expected = data->values[i];
      }
   }
   Scalar error() {
      Scalar acc = 0.0;
      for (auto v : this->neurons) {
         auto err = v->computed - v->expected;
         acc += err * err;
      }
      return acc;
   }
   void backpropagate() {
      //this->printFixedError();
      for (auto v : this->neurons) v->backpropagateImpulse(1);
      for (auto v : this->neurons) v->backpropagateImpulse(0.5);
      for (auto v : this->neurons) v->backpropagateImpulse(0.25);
      for (auto v : this->neurons) v->backpropagateImpulse(0.125);
      for (auto v : this->neurons) v->backpropagateImpulse(0.0625);
      for (auto v : this->neurons) v->updateError();
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
   void printState() {
      int activated = 0;
      int saturated = 0;
      int disabled = 0;
      for (int i = 0; i < this->size.count; i++) {
         auto& neuron = *this->neurons[i];
         if (neuron.saturated) saturated++;
         if (neuron.activated) activated++;
         else disabled++;
      }
      printf("| Layer(%d): activated = %d, saturated = %d, disabled = %d / %d\n", this->layerId, activated, saturated, disabled, this->size.count);
   }
   void printErrors() {
      printf("\n--------------------------\n");
      for (int i = 0; i < this->size.count; i++) {
         auto& neuron = *this->neurons[i];
         if (neuron.activated) {
            printf("%d: %.3lg \t-> %.3lg\n", i, neuron.error, neuron.forwardError());
         }
      }
      printf("> error: %lg\n", this->error());
   }
   void printResults() {
      printf("\n--------------------------\n");
      for (int i = 0; i < this->size.count; i++) {
         auto& neuron = *this->neurons[i];
         printf("%d: (%.3lg)\t%.3lg\n", i, Scalar(neuron.expected), Scalar(neuron.computed));
      }
      printf("> error: %lg\n", this->error());
   }
};

struct NeuralNetwork {
   std::vector<Layer*> layers;
   void addLayer(Size size) {
      Layer* layer = new Layer(size);
      layer->layerId = this->layers.size();
      if (this->layers.size()) {
         Layer* input = this->layers.back();
         connectFull(*input, *layer);
      }
      this->layers.push_back(layer);
   }
   void check() {
      for (auto layer : this->layers) {
         layer->check();
      }
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
      for (Layer* layer : this->layers) {
         layer->printState();
      }
   }
private:
   void connectFull(Layer& in, Layer& out) {
      Scalar dispersion = 1.0 / in.neurons.size();
      for (auto vout : out.neurons) {
         for (auto vin : in.neurons) {
            vout->connect(*vin, dispersion * randomScalar(-1.0, 1.0));
         }
      }
   }
};


void testMNIST() {
   MNIST::UByteTensorFile images("D:/data/MNIST/train-images.idx3-ubyte");
   MNIST::UByteTensorFile labels("D:/data/MNIST/train-labels.idx1-ubyte");

   NeuralNetwork network;

   network.addLayer(Size(2, 28, 28));
   //network.addLayer(Size(1, 28 * 28));
   //network.addLayer(Size(1, 28 * 28 / 2));
   //network.addLayer(Size(1, 28 * 28 / 4));
   network.addLayer(Size(1, 10));

   if (0) {
      Scalar learningRate = 0.1;
      int count = images.datacount;
      int index = 1;
      while (index < count) {
         for (int k = 0; k < 100 && index < count; k++) {
            Tensor2D* inData = images.getTensor2D(index);
            Tensor1D* outData = labels.getOrdinalTensor1D(index, 10);
            network.feed(inData);
            network.feedback(outData);
            network.learn(learningRate);
            index++;
         }
         Tensor2D* inData = images.getTensor2D(0);
         Tensor1D* outData = labels.getOrdinalTensor1D(0, 10);
         network.feed(inData);
         network.feedback(outData);
         network.printResults();
      }
   }
   else {
      Scalar learningRate = 0.1;
      Tensor2D* inData = images.getTensor2D(1);
      Tensor1D* outData = labels.getOrdinalTensor1D(0, 10);
      saveBitmap("test.bmp", inData);
      for (int k = 0; k < 1000; k++) {
         network.check();
         network.feed(inData);
         network.feedback(outData);
         network.learn(learningRate);
         network.printResults();
         printf("> error: %lg\n", network.layers.back()->error());
      }
   }
}


void main() {
   testMNIST();
}
