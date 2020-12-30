#include <vector>
#include <string>
#include <fstream>
#include "./chrono.h"
#include "./PersistentState.h"

typedef std::string IDEName;

using namespace wFS;

enum PersistantObjectID {
   Point1D_ID = 1,
   Point2D_ID,
};
struct Point : Persistent {
   UniqueRef<String> any;
   Point() {
      any = String::New("hello world");
   }
   Point(Persistent::Descriptor& infos) {
   }
   virtual void print() = 0;
};

struct Point1D : Point {
   int x;
   Point1D(int x) 
      : x(x) {
   }
   Point1D(Persistent::Descriptor& infos)
      : Point(infos) {
   }
   virtual int GetTypeID() override {
      return Point1D_ID;
   }
   virtual void print() override {
      printf("> x = %d\n", x);
   }   
};

struct Point2D: Point1D {
   int y;
   Point2D(int x, int y) 
      : Point1D(x), y(y) {
   }
   Point2D(Persistent::Descriptor& infos)
      : Point1D(infos) {
   }
   virtual int GetTypeID() override {
      return Point2D_ID;
   }
   virtual void print() override {
      printf("> x = %d, y = %d\n", x, y);
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
      _ASSERT(pointList->points.size() == 1000);
      wFS::ResetHeap();
      root = wFS::GetRootObject();
   }
   _ASSERT(root == 0);
   auto pointList = new PointList();
   for (int i = 0; i < 1000; i++) {
      Ref<Point> point = new Point2D(i, i);
      pointList->points.push_back(&*point);
      pointList->points[0]->print();
   }
   wFS::SetRootObject(pointList);


   for (auto& x : pointList->points) {
      x->print();
   }
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
   wFS::Persistent::RegisterInfos<Point1D>();
   wFS::Persistent::RegisterInfos<Point2D>();

   wFS::OpenHeap(TEST_STATE_PATH);
   wFS::ResetHeap();

   test_perf();
   test_persistance();
   test_map();
}
