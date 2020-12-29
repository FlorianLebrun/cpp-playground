#ifndef __AVL_Tree_Operators__
#define __AVL_Tree_Operators__

#include <stdint.h>

template <class TNode>
struct AVLDefaultHandler {
   typedef TNode* Node;
   static Node& left(Node node) { return node->left; }
   static Node& right(Node node) { return node->right; }
   static int& height(Node node) { return node->height; }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
*
* AVL Tree Operators
*
* Tooling for handling of AVL tree with node class 'TNode' containing the fields:
*   - left: TNode*
*   - right: TNode*
*   - height: integer
*
* All functions take the tree root in firast argument, and can return the new root.
*
** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/
template <class TNode, class TNodeHandler = AVLDefaultHandler<TNode>>
class AVLOperators {

public:
   typedef TNode* Node;
   typedef TNodeHandler At;

   struct IComparable {
      virtual int compare(Node node) = 0;
   };
   struct IInsertable : IComparable {
      virtual Node create(Node overridden) = 0;
   };

   // Iterator on tree nodes
   struct cursor {
   protected:
      Node current;
      Node stack[32];
      int level;
   public:
      cursor(Node root) {
         this->current = 0;
         this->level = 0;
         for (Node node = root; node; node = At::left(node)) {
            this->stack[this->level++] = node;
         }
      }
      Node next() {
         if (this->level > 0) {
            this->current = this->stack[--this->level];
            for (Node node = At::right(this->current); node; node = At::left(node)) {
               this->stack[this->level++] = node;
            }
         }
         else {
            this->current = 0;
         }
         return this->current;
      }
      TNode& operator -> () {
         return *this->current;
      }
      Node operator * () {
         return this->current;
      }
   };
   struct iterator : cursor {
      iterator(Node root) : cursor(root) {
         this->next();
      }
      void operator ++() {
         this->next();
      }
      struct end {
         bool operator != (iterator& it) { return !!it.current; }
      };
      bool operator != (end& it) { return !!this->current; }
   };

   // Check Balance factor of sub tree
   static bool checkConsistency(Node root) {
      if (!root) return true;
      int height = getSubHeight(root) + 1;
      if (At::height(root) != height) return false;
      int balance = getBalance(root);
      if (balance < -1 || balance > 1) return false;
      return checkConsistency(At::left(root)) && checkConsistency(At::right(root));
   }

   static int count(Node root)
   {
      if (!root) return 0;
      int n = 1;
      n += count(At::left(root));
      n += count(At::right(root));
      return n;
   }

   static Node removeAtMinimum(Node root, Node& result) {
      if (At::left(root)) {
         At::left(root) = removeAtMinimum(At::left(root), result);
         return rebalance(root);
      }
      else {
         result = root;
         return At::right(root);
      }
   }

   static Node removeAtMaximum(Node root, Node& result) {
      if (At::right(root)) {
         At::right(root) = removeAtMaximum(At::right(root), result);
         return rebalance(root);
      }
      else {
         result = root;
         return At::left(root);
      }
   }

   static Node removeAt(Node root, IComparable* comparable, Node& result)
   {
      // Empty sub tree, nothing to remove
      if (root == 0) {
         result = 0;
         return 0;
      }

      // Remove key from sub tree
      int c = comparable->compare(root);
      if (c > 0) {
         At::right(root) = removeAt(At::right(root), comparable, result);
         return rebalance(root);
      }
      else if (c < 0) {
         At::left(root) = removeAt(At::left(root), comparable, result);
         return rebalance(root);
      }
      else {
         result = root;
         if (At::left(root) && At::right(root)) {
            Node new_root;
            At::right(root) = removeAtMinimum(At::right(root), new_root);
            At::left(new_root) = At::left(root);
            At::right(new_root) = At::right(root);
            return rebalance(new_root);
         }
         else {
            return reinterpret_cast<Node>(uintptr_t(At::right(root)) | uintptr_t(At::left(root)));
         }
      }
   }

   static void findAt(Node root, IComparable* comparable, Node& result) {

      // Empty sub tree, nothing to remove
      if (root == 0) {
         result = 0;
         return;
      }

      // Find key from sub tree
      int c = comparable->compare(root);
      if (c > 0) {
         findAt(At::right(root), comparable, result);
      }
      else if (c < 0) {
         findAt(At::left(root), comparable, result);
      }
      else {
         result = root;
      }
   }

   static Node insertAt(Node root, IInsertable* insertable, Node& result) {

      // New node location reached
      if (root == 0) {
         if (result = insertable->create(0)) {
            At::height(result) = 1;
            At::left(result) = 0;
            At::right(result) = 0;
         }
         return result;
      }

      // Perform the insertion
      int c = insertable->compare(root);
      if (c < 0) {
         At::left(root) = insertAt(At::left(root), insertable, result);
         return rebalance(root);
      }
      else if (c > 0) {
         At::right(root) = insertAt(At::right(root), insertable, result);
         return rebalance(root);
      }
      else {
         if (result = insertable->create(root)) {
            At::height(result) = At::height(root);
            At::left(result) = At::left(root);
            At::right(result) = At::right(root);
            return result;
         }
         return root;
      }
   }

private:

   static int getSubHeight(Node node) {
      Node right = At::right(node);
      if (Node left = At::left(node)) {
         if (right) {
            if (At::height(left) > At::height(right)) return At::height(left);
            else return At::height(right);
         }
         else return At::height(left);
      }
      else {
         if (right) return At::height(right);
         else return 0;
      }
   }
   static int getBalance(Node node) {
      int balance = At::left(node) ? At::height(At::left(node)) : 0;
      if (At::right(node)) return balance - At::height(At::right(node));
      else return balance;
   }
   static Node rightRotate(Node y) {
      Node x = At::left(y);

      // Perform rotation
      Node tmp = At::right(x);
      At::right(x) = y;
      At::left(y) = tmp;

      // Update heights
      At::height(y) = getSubHeight(y) + 1;
      At::height(x) = getSubHeight(x) + 1;

      // Return new root
      return x;
   }
   static Node leftRotate(Node x) {
      Node y = At::right(x);

      // Perform rotation
      Node tmp = At::left(y);
      At::left(y) = x;
      At::right(x) = tmp;

      // Update heights
      At::height(x) = getSubHeight(x) + 1;
      At::height(y) = getSubHeight(y) + 1;

      // Return new root
      return y;
   }
   static Node rebalance(Node node) {
      int balance = getBalance(node);
      if (balance < -1) {
         if (getBalance(At::right(node)) <= 0) {
            return leftRotate(node);
         }
         else {
            At::right(node) = rightRotate(At::right(node));
            return leftRotate(node);
         }
      }
      else if (balance > 1) {
         if (getBalance(At::left(node)) >= 0) {
            return rightRotate(node);
         }
         else {
            At::left(node) = leftRotate(At::left(node));
            return rightRotate(node);
         }
      }
      else {
         At::height(node) = getSubHeight(node) + 1;
         return node;
      }
   }
};

#endif
