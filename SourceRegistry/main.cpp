#include <vector>
#include <string>
#include <fstream>
#include "./chrono.h"
#include "./SourcePersistantState.h"

typedef std::string IDEName;

using namespace SourceState;

struct Point : SourceState::Object {
  char x;
  char y;
  UniqueRef<String> any;
  Point() {
    x = 'x';
    y = 'y';
    any = String::New("hello world");
  }
};

struct PointList : SourceState::Object {
  List<UniqueRef<Point>> points;
};


void test_perf() {
  Chrono c;
  int count = 1000000;

  c.Start();
  for (int i = 0; i < count; i++) {
    struct BufTest : SourceState::Object { char data[122]; };
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
  Object* root = SourceState::GetRootObject();
  if (root) {
    auto pointList = (PointList*)root;
    _ASSERT(pointList->points.size() == 100000);
    SourceState::ResetHeap();
    root = SourceState::GetRootObject();
  }
  _ASSERT(root == 0);
  auto pointList = new PointList();
  for (int i = 0; i < 10000; i++) {
    auto point = new Point();
    point->x = 'v';
    //pointList->points.push_back(point);
  }
  SourceState::SetRootObject(pointList);
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

#include <iostream>
#include <Windows.h>

struct EncodingBuffer {
  char* buffer;
  size_t size;
  EncodingBuffer();
  EncodingBuffer(char* buffer, size_t size);
  void Encode_UTF8_From_DefaultCP(EncodingBuffer& src);
};

EncodingBuffer::EncodingBuffer() {
  this->buffer = 0;
  this->size = 0;
}

EncodingBuffer::EncodingBuffer(char* buffer, size_t size) {
  this->buffer = buffer;
  this->size = size;
}


void EncodingBuffer::Encode_UTF8_From_DefaultCP(EncodingBuffer& src) {
  int src_encoding = CP_ACP;
  int dst_encoding = CP_UTF8;

  // Decode into codepoint values
  wchar_t* codes_buf = new wchar_t[src.size];
  int codes_size = MultiByteToWideChar(src_encoding, 0, src.buffer, src.size, codes_buf, src.size);

  // Encoding codepoint values into destination
  this->size = codes_size*2;
  this->buffer = (char*)::malloc(this->size);
  this->size = WideCharToMultiByte(dst_encoding, 0, codes_buf, codes_size, this->buffer, this->size, 0, false);

  delete codes_buf;
}
void main() {

  std::string tout = "héllo world";
  EncodingBuffer dst, src((char*)tout.c_str(), tout.size());
  dst.Encode_UTF8_From_DefaultCP(src);
  std::string sout(dst.buffer, dst.size);
  ::free(dst.buffer);
  std::cout<<sout;

  test_perf();
  test_persistance();
  test_map();


}
