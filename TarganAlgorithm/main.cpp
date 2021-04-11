#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <math.h>

typedef struct tNode {
   int id = -1;
   int componentId = -1;
   int componentIndex = -1;
   int componentLowIndex = -1;
   std::vector<tNode*> successors;
}*Node;

struct TarganAlgorithm {
   int lastIndex = 0;
   std::vector<Node> stack;
   std::vector<std::vector<Node>> components; // Strongly Connected Components (ie. SCC)
   std::set<Node> seens;

   bool isOnStack(Node node) {
      for (Node x : this->stack) if (x == node) return true;
      return false;
   }
   bool isNotProcessed(Node node) {
      return node->componentIndex < 0;
   }
   void process(Node v) {
      // Set the depth index for v to the smallest unused index
      this->seens.insert(v);
      this->stack.push_back(v);
      v->componentIndex = this->lastIndex;
      v->componentLowIndex = this->lastIndex;
      this->lastIndex++;

      // Consider successors of v
      for (Node w : v->successors) {
         if (this->isNotProcessed(w)) {
            // Successor w has not yet been visited; recurse on it
            this->process(w);
            v->componentLowIndex = std::min(v->componentLowIndex, w->componentLowIndex);
         }
         else if (this->isOnStack(w)) {
            // Successor w is in stack S and hence in the current SCC
            // If w is not on stack, then (v, w) is an edge pointing to an SCC already found and must be ignored
            // Note: The next line may look odd - but is correct.
            // It says w.index not w.lowlink; that is deliberate and from the 
            v->componentLowIndex = std::min(v->componentLowIndex, w->componentIndex);
         }
      }

      // If v is a root node, pop the stack and generate an SCC
      if (v->componentLowIndex == v->componentIndex) {
         std::vector<Node> component;
         int componentId = this->components.size();
         Node w;
         do {
            w = this->stack.back();
            this->stack.pop_back();
            w->componentId = componentId;
            component.push_back(w);
         } while (w != v);
         this->components.push_back(component);
      }
   }
   void processGraph(Node* nodes, int count) {
      for (int i = 0; i < count; i++) {
         Node v = nodes[i];
         if (this->isNotProcessed(v)) {
            this->process(v);
         }
      }
   }
};


int main() {
   std::vector<Node> n;
   for (int i = 0; i < 8; i++) {
      Node x = new tNode();
      x->id = i;
      n.push_back(x);
   }

   n[0]->successors.push_back(n[4]);

   n[1]->successors.push_back(n[0]);

   n[2]->successors.push_back(n[1]);
   n[2]->successors.push_back(n[3]);

   n[3]->successors.push_back(n[2]);

   n[4]->successors.push_back(n[1]);

   n[5]->successors.push_back(n[1]);
   n[5]->successors.push_back(n[4]);
   n[5]->successors.push_back(n[6]);

   n[6]->successors.push_back(n[2]);
   n[6]->successors.push_back(n[5]);

   n[7]->successors.push_back(n[3]);
   n[7]->successors.push_back(n[6]);
   n[7]->successors.push_back(n[7]);

   TarganAlgorithm algo;
   algo.processGraph(n.data(), n.size());

   return 0;
}

