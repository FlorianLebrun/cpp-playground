#pragma once

#include <stdint.h>
#include "../../modules/AVLOperators.h"

namespace SourceState {

   typedef uint8_t* BytesPointer;

   struct Object {
      static void* operator new(size_t size);
      static void operator delete(void* ptr);
      static void* resize(Object* ptr, size_t newsize);
   };

   struct BaseRef {
      union {
         struct {
            uint32_t segment;
            uint32_t offset;
         };
         uint64_t _bits;
      };
      BaseRef() { this->_bits = 0; }
      operator bool() const { return !!this->_bits; }
      Object* get() const;
      void set(Object* ptr);
   };

   template <typename T>
   struct Ref : BaseRef {
      Ref() {}
      Ref(BaseRef& ref) { this->_bits = ref._bits; }
      Ref(T* ptr) { this->set(ptr); }
      T* operator ->() const { return (T*)this->get(); }
      T& operator *() const { return *(T*)this->get(); }
      T* operator = (T* ptr) { this->set(ptr); return ptr; }
      operator T* () const { return (T*)this->get(); }
   };

   template <typename T>
   struct UniqueRef : public Ref<T> {
      UniqueRef() {}
      UniqueRef(T* ptr) { this->set(ptr); }
      ~UniqueRef() { delete this->get(); }
      T* operator ->() const { return (T*)this->get(); }
      T& operator *() const { return *(T*)this->get(); }
      T* operator = (T* ptr) { this->set(ptr); return ptr; }
      operator T* () const { return (T*)this->get(); }
   };

   template <typename T>
   struct List {
      //typedef Ref<Object> T;
      struct t_buffer : Object {
         uint32_t count;
         T items[0];
         t_buffer() {
            this->count = 0;
         }
      };
      UniqueRef<t_buffer> content;
      size_t size() {
         return this->content.segment ? content->count : 0;
      }
      T& operator [](size_t index) const {
         t_buffer* content = this->content;
         if (content && index < content->count) return content->items[index];
         else throw "overflow";
      }
      void push_back(T& data) {
         size_t newIndex = this->size();
         t_buffer* content = this->content;
         content = (t_buffer*)Object::resize(content, sizeof(t_buffer) + sizeof(T) * (newIndex + 1));
         content->count++;
         content->items[newIndex] = data;
         this->content = content;
      }
   };

   template<typename KeyT, typename ValueT>
   struct Map {
      //typedef int KeyT; typedef int ValueT;

      struct t_node : Object {
         Ref<t_node> left;
         Ref<t_node> right;
         uint16_t height;
         KeyT key;
         ValueT value;
         t_node(KeyT& _key, ValueT& _value)
            :key(_key), value(_value) {
         }
      };

      struct t_node_handler {
         typedef Ref<t_node> Node;
         static Node& left(Node node) { return node->left; }
         static Node& right(Node node) { return node->right; }
         static uint16_t& height(Node node) { return node->height; }
      };

      typedef AVLOperators<t_node, t_node_handler> AVL;

      struct t_key_find : AVL::IInsertable {
         KeyT& key;
         t_key_find(KeyT& _key) : key(_key) {}
         virtual int compare(t_node* node) override { return key - node->key; };
         virtual t_node* create(t_node* overridden) override { return 0; };
      };
      struct t_key_insert : t_key_find {
         ValueT& value;
         t_key_insert(KeyT& _key, ValueT& _value) : t_key_find(_key), value(_value) {}
         virtual t_node* create(t_node* overridden) override { return overridden ? 0 : new t_node(key, value); };
      };

      Ref<t_node> root;
      int count;

      Map()
         : count(0) {
      }
      ~Map() {
         for (auto x : *this) delete x;
      }
      typename AVL::iterator begin() {
         return AVL::iterator(this->root);
      }
      typename AVL::iterator::end end() {
         return AVL::iterator::end();
      }
      size_t size() const {
         return this->count;
      }
      ValueT* operator [](KeyT& key) const {
         return this->find(key);
      }
      ValueT* find(KeyT& key) const {
         t_node* result;
         AVL::findAt(this->root, &t_key_find(key), result);
         if (result) return &result->value;
         else return 0;
      }
      bool insert(KeyT& key, ValueT& value) {
         t_node* result;
         this->root = AVL::insertAt(this->root, &t_key_insert(key, value), result);
         if (result) {
            this->count++;
            return true;
         }
         return false;
      }
      bool remove(KeyT& key) {
         t_node* result;
         this->root = AVL::removeAt(this->root, &t_key_find(key), result);
         if (result) {
            delete result;
            this->count--;
            return true;
         }
         return false;
      }
   };

   struct String : Object {
      uint32_t length;
      char chars[1];

      static void* operator new(size_t size, size_t count);
      static String* New(const char* chars, int32_t count = -1);
   };

   Ref<Object> GetRootObject();
   void SetRootObject(Ref<Object>);

   void ResetHeap();
}
