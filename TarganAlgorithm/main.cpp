#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <math.h>

typedef struct tNode {
   int id = -1;
   std::vector<tNode*> dependencies;
   void add(tNode* dependency) {
      this->dependencies.push_back(dependency);
   }
}*Node;

struct DependencyOrderingAlgorithm {

   enum class status_t : uint8_t {
      NotProcessed,
      Processing,
      Completed,
   };

   struct data_t {
      Node node;
      status_t status = status_t::NotProcessed;
      union {
         struct {
            uint32_t index;
            uint32_t lowIndex;
         } state;
         data_t* connected;
      };
      data_t(Node node = nullptr)
         : node(node), status(status_t::NotProcessed) {
      }
   };

   struct component_t {
      uint32_t groupId;
      data_t* connecteds;
   };

   std::unordered_map<Node, data_t> nodes;
   std::vector<component_t> components; // Strongly Connected Components (ie. SCC)

   void process(Node* nodes, int count) {

      // Register node/data pair for computing
      for (int i = 0; i < count; i++) {
         auto node = nodes[i];
         this->nodes.insert({ node, data_t(node) });
      }

      // Compute strongly connected components with Targan Algorithm
      // Note: the order in which the strongly connected components are identified constitutes
      //       a reverse topological sort of the DAG formed by the strongly connected components
      for (auto& entry : this->nodes) {
         if (entry.second.status == status_t::NotProcessed) {
            this->processNode(&entry.second);
         }
      }
   }
   void print() {
      for (auto& component : this->components) {
         for (auto v = component.connecteds; v; v = v->connected) {
            printf("group: %d, node: %d\n", component.groupId, v->node->id);
         }
      }
   }

private:
   int counter = 0;
   std::vector<data_t*> stack;

   void processNode(data_t* v) {
      // Set the depth index for v to the smallest unused index
      v->status = status_t::Processing;
      v->state.index = this->counter;
      v->state.lowIndex = this->counter;
      stack.push_back(v);
      this->counter++;

      // Consider successors of v
      for (Node succ : v->node->dependencies) {
         data_t* w = &this->nodes[succ];
         if (w->status == status_t::NotProcessed) {
            // Successor w has not yet been visited; recurse on it
            this->processNode(w);
            v->state.lowIndex = std::min(v->state.lowIndex, w->state.lowIndex);
         }
         else if (w->status == status_t::Processing) {
            // Successor w is in stack S and hence in the current SCC
            // If w is not on stack, then (v, w) is an edge pointing to an SCC already found and must be ignored
            // Note: The next line may look odd - but is correct.
            // It says w.index not w.lowlink; that is deliberate and from the 
            v->state.lowIndex = std::min(v->state.lowIndex, w->state.index);
         }
      }

      // If v is a root node, pop the stack and generate an SCC
      if (v->state.lowIndex == v->state.index) {
         component_t component;
         component.groupId = this->components.size();
         component.connecteds = nullptr;
         do {
            data_t* w = stack.back();
            stack.pop_back();
            w->status = status_t::Completed;
            w->connected = component.connecteds;
            component.connecteds = w;
         } while (component.connecteds != v);
         this->components.push_back(component);
      }
   }
};

void shuffleVector(std::vector<Node>& vec) {
   srand(clock());
   for (int i = 0; i < vec.size() * 2; i++) {
      int i1 = rand() % vec.size();
      int i2 = rand() % vec.size();
      Node t = vec[i1];
      vec[i1] = vec[i2];
      vec[i2] = t;
   }
}

int main() {
   std::vector<Node> n;
   for (int i = 0; i < 9; i++) {
      Node x = new tNode();
      x->id = i;
      n.push_back(x);
   }

   n[0]->add(n[8]);
   n[0]->add(n[4]);

   n[1]->add(n[0]);

   n[2]->add(n[1]);
   n[2]->add(n[3]);

   n[3]->add(n[2]);

   n[4]->add(n[1]);

   n[5]->add(n[1]);
   n[5]->add(n[4]);
   n[5]->add(n[6]);

   n[6]->add(n[2]);
   n[6]->add(n[5]);

   n[7]->add(n[3]);
   n[7]->add(n[6]);
   n[7]->add(n[7]);

   shuffleVector(n);

   DependencyOrderingAlgorithm algo;
   algo.process(n.data(), n.size());
   algo.print();
   return 0;
}

