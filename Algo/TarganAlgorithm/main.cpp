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
   tNode(int id)
      : id(id) {
   }
   void add(tNode* dependency) {
      this->dependencies.push_back(dependency);
   }
   struct GraphHandler {
      static std::string toString(tNode* node) {
         char tmp[8];
         return std::string("#") + itoa(node->id, tmp, 10);
      }
      static void successors(tNode* node, std::function<void(tNode*)>&& callback) {
         for (auto x : node->dependencies) callback(x);
      }
   };
private:
   int id;
   std::vector<tNode*> dependencies;
}*Node;


template <class TNode>
struct GraphDefaultHandler {
   typedef TNode* Node;
   static void successors(Node node, std::function<void(Node)>&& callback) { throw; }
};

template <class TNode, class TNodeHandler = GraphDefaultHandler<TNode>>
struct DependencyOrderingAlgorithm {

   typedef TNode* Node;
   typedef TNodeHandler At;

   enum class status_t : uint8_t {
      NotProcessed,
      Processing,
      Completed,
   };

   struct data_t {
      status_t status = status_t::NotProcessed;
      uint32_t index;
      uint32_t lowlink;
      data_t* connected;
      Node node;
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

   void addNode(Node node) {
      this->nodes.insert({ node, data_t(node) });
   }
   void process() {

      // Compute strongly connected components with Targan Algorithm
      // Note: the order in which the strongly connected components are identified constitutes
      //       a reverse topological sort of the DAG formed by the strongly connected components
      this->stack.clear();
      for (auto& entry : this->nodes) {
         if (entry.second.status == status_t::NotProcessed) {
            this->processNode(&entry.second);
         }
      }
   }
   void print() {
      for (auto& component : this->components) {
         for (auto v = component.connecteds; v; v = v->connected) {
            std::cout << "group: " << component.groupId << ", node: " << At::toString(v->node) << std::endl;
         }
      }
   }

private:
   int counter = 0;
   std::vector<data_t*> stack;

   void processNode(data_t* v) {
      // Set the depth index for v to the smallest unused index
      v->status = status_t::Processing;
      v->index = this->counter;
      v->lowlink = this->counter;
      stack.push_back(v);
      this->counter++;

      // Consider successors of v
      At::successors(v->node, [this, v](Node succ) {
         data_t* w = &this->nodes[succ];
         if (w->status == status_t::NotProcessed) {
            // Successor w has not yet been visited; recurse on it
            this->processNode(w);
            v->lowlink = std::min(v->lowlink, w->lowlink);
         }
         else if (w->status == status_t::Processing) {
            // Successor w is in stack S and hence in the current SCC
            // If w is not on stack, then (v, w) is an edge pointing to an SCC already found and must be ignored
            // Note: The next line may look odd - but is correct.
            // It says w.index not w.lowlink; that is deliberate and from the 
            v->lowlink = std::min(v->lowlink, w->index);
         }
      });

      // If v is a root node, pop the stack and generate an SCC
      if (v->lowlink == v->index) {
         component_t component;
         component.groupId = this->components.size();
         component.connecteds = nullptr;
         do {
            data_t* w = stack.back();
            w->status = status_t::Completed;
            w->connected = component.connecteds;
            component.connecteds = w;
            stack.pop_back();
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

void reorderVector(std::vector<Node>& vec, std::vector<int> orders) {
   _ASSERT(vec.size() == orders.size());
   std::vector<Node> result;
   for (int i : orders) {
      result.push_back(vec[i]);
   }
   result.swap(vec);
}

int main() {
   std::vector<Node> n;
   for (int i = 0; i < 9; i++) {
      n.push_back(new tNode(i));
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

   for (int i = 0; i < 1; i++) {
      shuffleVector(n);
      //reorderVector(n, { 5, 4, 3, 7, 1, 8, 6, 0, 2 });

      DependencyOrderingAlgorithm<tNode, tNode::GraphHandler> algo;
      // Register node/data pair for computing
      for (int i = 0; i < n.size(); i++) algo.addNode(n[i]);
      algo.process();
      algo.print();

      _ASSERT(algo.components.size() == 5);
   }
   return 0;
}

