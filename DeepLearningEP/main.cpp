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

struct Neuron {

   struct Link {
      Neuron& input;
      Scalar weight;
      Link(Neuron& input, Scalar weight) : input(input), weight(weight) {}
   };

   // Connection
   std::vector<Link> links;
   Scalar bias = 0.5;

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
         link.input.error += impulse * link.weight;
      }
      Scalar newForwardError = this->forwardError();
      if (abs(newForwardError - this->error) > abs(forwardError - this->error)) {
         printf("impulse issue\n");
      }
   }

   void learn(Scalar learningRate) {
      Scalar forwardError = this->forwardError();
      Scalar forwardPower = this->errorPower();
      if (forwardPower == 0.0) {
         return;
      }
      if (this->activate) {
         Scalar impulse = learningRate * (this->error - forwardError) / forwardPower;
         for (auto& link : this->links) {
            link.weight += impulse * link.input.error;
         }
         Scalar newForwardError = this->forwardError();
         if (abs(newForwardError) > abs(forwardError)) {
            printf("impulse issue\n");
         }
      }
      else {
         if (this->value > 0.5) {
            this->bias -= this->bias * learningRate * 0.01;
         }
         else {
            this->bias -= this->bias * learningRate * 0.01;
         }
      }
      this->bias = 0.5;
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

      // Simplified Sigmoide activation
      if (this->value < 0.0) {
         this->activate = false;
         this->value = 0.0;
      }
      else if (this->value > 1.0) {
         this->activate = false;
         this->value = 1.0;
      }
      else this->activate = true;
   }
   void connect(Neuron& neuron, Scalar weight) {
      this->links.push_back(Link(neuron, weight));
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
   void printState() {
      int activated = 0;
      int saturated = 0;
      int disabled = 0;
      for (int i = 0; i < this->size.count; i++) {
         auto& neuron = *this->neurons[i];
         if (neuron.activate) activated++;
         else if (neuron.value > 0.5) saturated++;
         else disabled++;
      }
      printf("| Layer(%d): activated = %d, saturated = %d, disabled = %d / %d\n", this->name, activated, saturated, disabled, this->size.count);
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
         Scalar expectedValue = neuron.value - neuron.error;
         printf("%d: (%.3lg)\t%.3lg\n", i, expectedValue, neuron.value);
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
            vout->connect(*vin, dispersion * randomScalar(0.0, 1.0));
         }
      }
   }
};

void main() {
   MNIST::UByteTensorFile images("D:/data/MNIST/train-images.idx3-ubyte");
   MNIST::UByteTensorFile labels("D:/data/MNIST/train-labels.idx1-ubyte");

   NeuralNetwork network;

   network.addLayer(Size(2, 28, 28));
   network.addLayer(Size(2, 28, 28));
   network.addLayer(Size(2, 28, 28));
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
         network.feed(inData);
         network.feedback(outData);
         network.learn(learningRate);
         network.printResults();
         printf("> error: %lg\n", network.layers.back()->error());
      }
   }
}

