#include "./handlers.h"

using namespace sat;

extern void test_perf_alloc();
extern void test_buddy_bitmap();
extern void test_unit_span_alloc();
extern void test_unit_fragment_alloc();
extern void test_page_access();

extern void test_objects_allocate();

sat::MemoryContext* sat_malloc_handler::context = 0;

int main() {

   //test_buddy_bitmap();
   //test_unit_span_alloc();
   //test_unit_fragment_alloc();
   //test_page_access();
   test_perf_alloc();
   //test_objects_allocate();

   sat_malloc_handler::context->scavenge();
   sat_malloc_handler::context->getStats();

   return 0;
}

