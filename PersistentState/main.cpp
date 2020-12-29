#include <vector>
#include <string>
#include <fstream>
#include "./chrono.h"
#include "./PersistentState.h"

typedef std::string IDEName;

using namespace wFS;

struct Point : Persistent {
   char x;
   char y;
   UniqueRef<String> any;
   Point() {
      x = 'x';
      y = 'y';
      any = String::New("hello world");
   }
};

struct PointList : Persistent {
   List<UniqueRef<Point>> points;
};


void test_perf() {
   Chrono c;
   int count = 1000000;

   c.Start();
   for (int i = 0; i < count; i++) {
      struct BufTest : Persistent { char data[122]; };
      auto x = new BufTest();
      delete x;
   }
   printf("> Time persistant alloc: %g s\n", c.GetDiffFloat(Chrono::S));

   c.Start();
   for (int i = 0; i < count; i++) {
      struct BufTest { char data[122]; };
      auto x = new BufTest();
      delete x;
   }
   printf("> Time malloc: %g s\n", c.GetDiffFloat(Chrono::S));
}

void test_persistance() {
   Persistent* root = wFS::GetRootObject();
   if (root) {
      auto pointList = (PointList*)root;
      _ASSERT(pointList->points.size() == 100000);
      wFS::ResetHeap();
      root = wFS::GetRootObject();
   }
   _ASSERT(root == 0);
   auto pointList = new PointList();
   for (int i = 0; i < 10000; i++) {
      auto point = new Point();
      point->x = 'v';
      //pointList->points.push_back(point);
   }
   wFS::SetRootObject(pointList);
}

void test_map() {
   struct tId {
      int x;
      int operator - (tId& other) {
         return this->x - other.x;
      }
      tId(int _x) : x(_x) {}
   };

   struct tValue {
      char bits[512];
   };

   Map<tId, tValue> m;
   m.insert(tId(10), tValue());
   auto r1 = m.find(tId(1));
   auto r2 = m.find(tId(2));
   for (int i = 0; i < 10000; i++) {
      m.insert(tId(rand()), tValue());
   }
   for (auto x : m) {
      printf("> x %d\n", x->key.x);
   }
}

void main() {

   wFS::OpenHeap(TEST_STATE_PATH);

   test_perf();
   test_persistance();
   test_map();
}
