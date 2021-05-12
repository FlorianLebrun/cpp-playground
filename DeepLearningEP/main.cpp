#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <math.h>
#include "./tensor.h"
#include "./MNIST.h"

struct Neuron;
struct Link;

struct NeuronState {
   bool activated : 1;
   bool saturated : 1;
   Scalar computed = 0.0;
   Scalar expected = 0.0;

   Scalar error = 0.0;
   Scalar innerError = 0.0;

   void setInnerValue(Scalar value) {
      this->expected = 0.0; // TODO: Maybe not useful
      if (value > 0.0) {
         this->activated = 1;
         if (value > 1.0) {
            this->saturated = 1;
            this->computed = 1.0;
         }
         else {
            this->saturated = 0;
            this->computed = value;
         }
      }
      else {
         this->activated = 0;
         this->computed = 0.0;
      }
   }
   void check() {
      checkAssert(this->computed >= 0.0 && this->computed <= 1.0);
   }
};

struct NeuronLink {
   NeuronState& input;
   Scalar weight;
   bool inhibitory : 1;
   bool excitatory : 1;
   NeuronLink(NeuronState& input, Scalar weight)
      : input(input), weight(weight) {
      if (this->weight > 0.0) {
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
   Scalar forwardValue() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         acc += link.weight * link.input.computed;
      }
      return acc;
   }
   Scalar forwardExpectedValue() {
      Scalar acc = 0.0;
      for (auto& link : *this) {
         if (link.input.activated) {
            acc += link.input.expected * link.weight;
         }
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

};

struct Neuron : NeuronState {
   NeuronLinks links;
   NeuronState bias; // Permanent inhibiter

   Neuron() {
      this->bias.setInnerValue(1.0);
      this->links.push_back(NeuronLink(this->bias, -0.5));
   }

   void check() {
      this->NeuronState::check();
      this->bias.check();
      for (auto& link : this->links) {
         link.check();
      }
   }

   Scalar forwardError() {
      return this->expected - this->links.forwardExpectedValue();
   }

   void backpropagateImpulse(Scalar impulseFactor) {
      Scalar expectedValue = this->links.forwardExpectedValue();
      Scalar weightPower = this->links.weightPower();
      if (weightPower == 0.0) {
         return;
      }
      Scalar impulse = impulseFactor * (this->expected - expectedValue) / weightPower;
      for (auto& link : this->links) {
         auto value = link.input.expected + impulse * link.weight;
         if (value < 0.0) value = 0.0;
         else if (value > 1.0) value = 1.0;
         link.input.expected = value;
      }
   }

   void updateError() {
      this->error = this->links.forwardExpectedValue() - this->expected;

      Scalar prev_error = abs(this->computed - this->expected);
      Scalar new_error = abs(this->links.forwardExpectedValue() - this->expected);
      if (new_error > prev_error) {
         printf("impulse issue\n");
      }
   }

   void learn(Scalar learningRate) {
      if (this->activated) {
         Scalar value = this->links.forwardExpectedValue();
         Scalar errorPower = 0.0;
         int activatedLink = 0;
         for (auto& link : this->links) {
            if (link.input.activated) {
               errorPower += link.input.error * link.input.error;
               activatedLink++;
            }
         }
         if (errorPower > 0.0) {
            Scalar impulse = learningRate * (this->expected - value) / errorPower;
            for (auto& link : this->links) {
               if (link.input.activated) {
                  link.weight -= impulse * link.input.error;
               }
            }
         }
         Scalar newValue = this->links.forwardExpectedValue();
         if (abs(newValue - this->expected) > abs(value - this->expected)) {
            //printf("learn issue\n");
         }
      }
   }

   void compute() {
      this->setInnerValue(this->links.forwardValue());
   }
   void connect(Neuron& neuron, Scalar weight) {
      this->links.push_back(NeuronLink(neuron, weight));
   }
};

struct Layer {
   int name = 0;
   std::vector<Neuron*> neurons;
   Size size;
   Layer(Size size) : size(size) {
      for (int i = 0; i < size.count; i++) {
         neurons.push_back(new Neuron());
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
      printf("| Layer(%d): activated = %d, saturated = %d, disabled = %d / %d\n", this->name, activated, saturated, disabled, this->size.count);
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
         printf("%d: (%.3lg)\t%.3lg\n", i, neuron.expected, neuron.computed);
      }
      printf("> error: %lg\n", this->error());
   }
};

struct NeuralNetwork {
   std::vector<Layer*> layers;
   void addLayer(Size size) {
      Layer* layer = new Layer(size);
      layer->name = this->layers.size();
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
   network.addLayer(Size(1, 28 * 28));
   network.addLayer(Size(1, 28 * 28 / 2));
   network.addLayer(Size(1, 28 * 28 / 4));
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
      Tensor2D* inData = images.getTensor2D(0);
      Tensor1D* outData = labels.getOrdinalTensor1D(0, 10);
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
